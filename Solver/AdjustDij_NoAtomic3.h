#ifndef ADJUSTDIJ_NOATOMIC3_H_
#define ADJUSTDIJ_NOATOMIC3_H_

#include <assert.h>
#include <stdlib.h>
#include "../Utils/ColorUtils.h"

namespace AdjustDij_NoAtomic3Utils {
    static const int PUSH_RETRY_TIMES = 5;
    static const int CAPACITY = 256;
}

class AdjustDij_NoAtomic3: public Solver<AdjacentMatrix> {
    char name[1024];
    const int THREAD_NUM, NUM_RELAX_EACH_ITERATION;

    Heaps *heaps;

    typedef Utils::TwoDConcurrentQueue<AdjustDij_NoAtomic3Utils::CAPACITY> TwoDConcurrentQueueType;

    TwoDConcurrentQueueType *queues;

    const int *colors;

    enum State {
        STATE_ACTIVE, STATE_READY_TO_STOP
    };

    struct WorkerInfo {
        volatile State state;
        volatile unsigned int version;
    };

    volatile WorkerInfo *workerInfo;

    inline void incVersion(const int &pid) {
        ++workerInfo[pid].version;
    }

    /*
     * try to relax nodes belong to current thread according to current knowledge, and update new value.
     */
    void relax(const int &pid, AdjacentMatrix *matrix, int &totalScan) {
        AdjacentMatrix::Arch *&archArray = matrix->archArray;
        int *&g = matrix->g;
        int *&result = matrix->result;
        Utils::Item data;
        int &src_node = data.key, &src_result = data.value;
        int left = NUM_RELAX_EACH_ITERATION;
        while (left > 0 && !heaps->empty(pid)) {
            heaps->removeTop(pid, &data);
            if (result[src_node] > src_result) result[src_node] = src_result;
            /*
             * relax according to src_node
             */
            bool push_ret = true;
#ifdef PERF_SCAN
            ++totalScan;
#endif
            for (int i = g[src_node]; i < g[src_node + 1]; ++i) {
                int &dst_node = archArray[i].j, &weight = archArray[i].w, new_dst_result = src_result + weight;
                /*
                 * case colors[dst_node]==pid, we get right result[dst_node]
                 * case colors[dst_node]!=pid, current thread may have a cache value, but the cache value must be bigger than the actural one.
                 */
                if (result[dst_node] > new_dst_result) {
                    if (colors[dst_node] == pid) {
                        result[dst_node] = new_dst_result;
                        heaps->addOrUpdate(pid, dst_node, new_dst_result);
                    } else {
                        push_ret = false;
                        Utils::SmallConcurrentQueue<AdjustDij_NoAtomic3Utils::CAPACITY>* dst_queue = queues->get(colors[dst_node], pid);
                        for (int tmp = 0; tmp < AdjustDij_NoAtomic3Utils::PUSH_RETRY_TIMES; ++tmp) {
                            push_ret = dst_queue->push(dst_node, new_dst_result);
                            if (push_ret) break;
                        }
                        if (!push_ret) break;
                    }
                }
                if (!push_ret) break;
            }
            if (!push_ret) {
                //push fails, go to adjust and later retry relax
                heaps->add(pid, src_node, src_result);
                break;
            } else {
                --left;
            }
        }
    }

    void adjust(const int &pid, AdjacentMatrix *matrix) {
        int *&result = matrix->result;
        Utils::Item data;
        int &src_node = data.key, &src_result = data.value;
        for (int i = 0; i < THREAD_NUM; ++i) {
            while (queues->get(pid, i)->pop(data))
                if (result[src_node] > src_result) {
                    result[src_node] = src_result;
                    heaps->addOrUpdate(pid, src_node, src_result);
                }
        }
    }

    bool isUpdateQueueEmpty(const int &pid) {
        for (int i = 0; i < THREAD_NUM; ++i)
            if (!queues->get(pid, i)->empty() || !queues->get(i, pid)->empty()) return false;
        return true;
    }

    bool isHeapQueueEmpty(const int &pid) {
        return heaps->empty(pid) && isUpdateQueueEmpty(pid);
    }

    void worker(AdjacentMatrix *matrix, int &totalScan) {
        const int pid = omp_get_thread_num();
        volatile State &myState = workerInfo[pid].state;
        unsigned int localVersions[THREAD_NUM];
        bool more = true;
        while (more) {
            switch (myState) {
                case STATE_ACTIVE:
                    relax(pid, matrix, totalScan);
                    adjust(pid, matrix);
                    if (isHeapQueueEmpty(pid)) {
                        myState = STATE_READY_TO_STOP;
                        incVersion(pid);
                    }
                    break;
                case STATE_READY_TO_STOP:
                    if (!isHeapQueueEmpty(pid)) {
                        myState = STATE_ACTIVE;
                        break;
                    }
                    bool someStateIsActive = false;
                    for (int i = 0; i < THREAD_NUM && !someStateIsActive; ++i) {
                        if (workerInfo[i].state == STATE_ACTIVE) someStateIsActive = true;
                        localVersions[i] = workerInfo[i].version;
                    }
                    if (someStateIsActive) break;
                    bool isFinished = true;
                    for (int i = 0; i < THREAD_NUM && !someStateIsActive && isFinished; ++i) {
                        if (workerInfo[i].state == STATE_ACTIVE) someStateIsActive = true;
                        if (localVersions[i] != workerInfo[i].version) isFinished = false;
                    }
                    if (someStateIsActive) break;
                    if (isFinished) more = false;
                    break;
            }
        }
    }

public:
    AdjustDij_NoAtomic3(int thread_num, int num_relax_each_iteration) :
            THREAD_NUM(thread_num), NUM_RELAX_EACH_ITERATION(num_relax_each_iteration), heaps(NULL), queues(NULL), colors(NULL), workerInfo(NULL) {
        sprintf(name, "Dijstra + Adjustment + NoAtomic3: THREAD_NUM=%d, NUM_RELAX_EACH_ITERATION=%d", THREAD_NUM, NUM_RELAX_EACH_ITERATION);
    }

    const char * getName() {
        return name;
    }

    void compute(AdjacentMatrix &adjMatrix, Colors *colors, int source, double &prepareTimeS, double &runningTimeS, double &totalScan, double &avgScan) {
        omp_set_num_threads(THREAD_NUM);

        CycleTimer prepareTimer, runningTimer;
#ifdef PERF_SCAN
        totalScan = 0;
#endif

        // Step 1: prepare Environment
        prepareTimer.Start();

        int &n = adjMatrix.n;
        this->colors = colors->getColors();

        int *&result = adjMatrix.result;
        adjMatrix.resetResult();

        /*
         * create heaps and queues for each thread
         */
        heaps = new Heaps(n, THREAD_NUM, colors->getNodeNumberCount());
        queues = new TwoDConcurrentQueueType(THREAD_NUM);

        /*
         * create sync protocol for each thread
         */
        workerInfo = (WorkerInfo *) malloc(sizeof(WorkerInfo) * THREAD_NUM);
        for (int i = 0; i < THREAD_NUM; ++i) {
            workerInfo[i].state = STATE_ACTIVE;
            workerInfo[i].version = 0;
        }

        prepareTimer.Stop();

        // Step 2: start to compute
        runningTimer.Start();

        result[source] = 0;
        heaps->add(this->colors[source], source, 0);

#pragma omp parallel
        {
            int ts = 0;
            worker(&adjMatrix, ts);
#ifdef PERF_SCAN
#pragma omp atomic
            totalScan += ts;
#endif
        }

        runningTimer.Stop();

        // Step 3: destroy environment
        prepareTimer.Start();

        /*
         * destroy sync protocol for each thread
         */
        free((void *) workerInfo);

        /*
         * destroy heaps and queues for each thread
         */
        delete queues;
        delete heaps;

        prepareTimer.Stop();

        runningTimeS = runningTimer.GetSecond();
        prepareTimeS = prepareTimer.GetSecond();
#ifdef PERF_SCAN
        avgScan = totalScan / THREAD_NUM;
#endif
    }
};

#endif
