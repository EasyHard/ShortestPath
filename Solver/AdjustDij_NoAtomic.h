#ifndef ADJUSTDIJ_NOATOMIC_H_
#define ADJUSTDIJ_NOATOMIC_H_

#include <assert.h>
#include <stdlib.h>
#include "../Utils/ColorUtils.h"

namespace AdjustDij_NoAtomicUtils {
    static const int PUSH_RETRY_TIMES = 5;
    static const int CAPACITY = 256;
}

class AdjustDij_NoAtomic: public Solver<AdjacentMatrix> {
    char name[1024];
    const int THREAD_NUM, NUM_RELAX_EACH_ITERATION;

    Heaps *heaps;

    typedef Utils::TwoDConcurrentQueue<AdjustDij_NoAtomicUtils::CAPACITY> TwoDConcurrentQueueType;

    TwoDConcurrentQueueType *queues;

    const int *colors;

    enum State {
        STATE_ACTIVE, STATE_HEAP_EMPTY, STATE_HEAP_QUEUE_EMPTY, STATE_NO_MORE
    };
    volatile State *states;

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
                        Utils::SmallConcurrentQueue<AdjustDij_NoAtomicUtils::CAPACITY>* dst_queue = queues->get(colors[dst_node], pid);
                        for (int tmp = 0; tmp < AdjustDij_NoAtomicUtils::PUSH_RETRY_TIMES; ++tmp) {
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
        volatile State &myState = states[pid], &preState = states[(pid + THREAD_NUM - 1) % THREAD_NUM];
        bool more = true;
        while (more) {
            switch (myState) {
                case STATE_ACTIVE:
                    relax(pid, matrix, totalScan);
                    adjust(pid, matrix);
                    if (isHeapQueueEmpty(pid) && (pid == 0 || preState != STATE_ACTIVE)) myState = STATE_HEAP_EMPTY;
                    break;
                case STATE_HEAP_EMPTY:
                    if (isHeapQueueEmpty(pid)) {
                        if (pid == 0) {
                            if (preState == STATE_HEAP_EMPTY) myState = STATE_HEAP_QUEUE_EMPTY;
                        } else {
                            if (preState == STATE_HEAP_QUEUE_EMPTY) myState = STATE_HEAP_QUEUE_EMPTY;
                        }
                    } else
                        myState = STATE_ACTIVE;
                    break;
                case STATE_HEAP_QUEUE_EMPTY:
                    if (pid == 0) {
                        if (preState == STATE_HEAP_QUEUE_EMPTY) myState = STATE_NO_MORE;
                    } else {
                        if (preState == STATE_NO_MORE) myState = STATE_NO_MORE;
                    }
                    break;
                case STATE_NO_MORE:
                    more = false;
                    break;
            }
        }
    }

public:
    AdjustDij_NoAtomic(int thread_num, int num_relax_each_iteration) :
            THREAD_NUM(thread_num), NUM_RELAX_EACH_ITERATION(num_relax_each_iteration), heaps(NULL), queues(NULL), colors(NULL), states(NULL) {
        sprintf(name, "Dijstra + Adjustment + NoAtomic: THREAD_NUM=%d, NUM_RELAX_EACH_ITERATION=%d", THREAD_NUM, NUM_RELAX_EACH_ITERATION);
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
        states = (State *) malloc(sizeof(State) * THREAD_NUM);
        for (int i = 0; i < THREAD_NUM; ++i)
            states[i] = STATE_ACTIVE;

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
        free((void *) states);

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
