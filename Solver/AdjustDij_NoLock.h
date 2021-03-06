#ifndef ADJUSTDIJ_NOLOCK_H_
#define ADJUSTDIJ_NOLOCK_H_

#include <assert.h>
#include <stdlib.h>
#include "../Utils/ColorUtils.h"

namespace AdjustDij_NoLockUtils {

    struct NonBlockingSyncer {
        const int threadNum;
        volatile int globalVersion;
        volatile int * localVersion;

        NonBlockingSyncer(int threadNum) :
                threadNum(threadNum), globalVersion(0), localVersion(NULL) {
            localVersion = (int*) malloc(sizeof(int) * threadNum);
            assert(localVersion!=NULL);
        }

        void reset() {
            globalVersion = 0;
            memset((void*) localVersion, 0, sizeof(int) * threadNum);
        }

        ~NonBlockingSyncer() {
            if (localVersion != NULL) free((int*) localVersion);
        }

        void incGlobalVersion() {
#pragma omp atomic
            ++globalVersion;
        }

        void print(int pid) const {
#pragma omp critical
            {
                printf("pid=%d globalVersion=%d\n", pid, globalVersion);
                for (int i = 0; i < threadNum; ++i)
                    printf("%d ", localVersion[i]);
                printf("\n");
                fflush(stdout);
            }
        }
    };

    static const int PUSH_RETRY_TIMES = 5;

    static const int CAPACITY = 256;
}

class AdjustDij_NoLock: public Solver<AdjacentMatrix> {
    char name[1024];
    const int THREAD_NUM, NUM_RELAX_EACH_ITERATION;

    Heaps *heaps;

    typedef Utils::TwoDConcurrentQueue<AdjustDij_NoLockUtils::CAPACITY> TwoDConcurrentQueueType;

    TwoDConcurrentQueueType *queues;

    AdjustDij_NoLockUtils::NonBlockingSyncer *syncer;

    const int *colors;

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
        bool result_updated = false;
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
                        result_updated = true;
                    } else {
                        push_ret = false;
                        Utils::SmallConcurrentQueue<AdjustDij_NoLockUtils::CAPACITY>* dst_queue = queues->get(colors[dst_node], pid);
                        for (int tmp = 0; tmp < AdjustDij_NoLockUtils::PUSH_RETRY_TIMES; ++tmp) {
                            push_ret = dst_queue->push(dst_node, new_dst_result);
                            if (push_ret) break;
                        }
                        if (push_ret)
                            result_updated = true;
                        else
                            break;
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
        if (result_updated) syncer->incGlobalVersion();
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
                    syncer->incGlobalVersion();
                }
        }
    }

    void worker(AdjacentMatrix *matrix, int &totalScan) {
        int pid = omp_get_thread_num();
        bool more = true;
        while (more) {
            int oldVersion = syncer->globalVersion;
            if (syncer->localVersion[pid] < oldVersion) {
                relax(pid, matrix, totalScan);
                adjust(pid, matrix);
                if (heaps->empty(pid)) syncer->localVersion[pid] = oldVersion;
                //syncer->print(pid);
            } else {
                if (oldVersion == syncer->globalVersion) {
                    bool subMore = false;
                    for (int i = 0; i < THREAD_NUM; ++i)
                        if (syncer->localVersion[i] != oldVersion) {
                            subMore = true;
                            break;
                        }
                    if (!subMore && oldVersion == syncer->globalVersion) more = false;
                }
                //syncer->print(pid);
            }
        }
        //printf("worker pid=%d exit\n", pid);fflush(stdout);
    }

public:
    AdjustDij_NoLock(int thread_num, int num_relax_each_iteration) :
            THREAD_NUM(thread_num), NUM_RELAX_EACH_ITERATION(num_relax_each_iteration), heaps(NULL), queues(NULL), syncer(NULL), colors(NULL) {
        sprintf(name, "Dijstra + Adjustment + NoLock: THREAD_NUM=%d, NUM_RELAX_EACH_ITERATION=%d", THREAD_NUM, NUM_RELAX_EACH_ITERATION);
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
        syncer = new AdjustDij_NoLockUtils::NonBlockingSyncer(THREAD_NUM);
        syncer->reset();

        prepareTimer.Stop();

        // Step 2: start to compute
        runningTimer.Start();

        result[source] = 0;
        heaps->add(this->colors[source], source, 0);
        syncer->incGlobalVersion();

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
        delete syncer;

        /*
         * destroy heaps and queues for each thread
         */
        delete queues;
        delete heaps;

        prepareTimer.Stop();

        runningTimeS = runningTimer.GetSecond();
        prepareTimeS = prepareTimer.GetSecond();
        printf("[NoLock] checkSumNoI = 0x%x\n", adjMatrix.checkSumNoI());
#ifdef PERF_SCAN
        avgScan = totalScan / THREAD_NUM;
#endif
    }
};

#endif
