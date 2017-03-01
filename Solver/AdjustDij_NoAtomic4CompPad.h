#ifndef ADJUSTDIJ_NOATOMIC4Pad_H_
#define ADJUSTDIJ_NOATOMIC4Pad_H_

#include <assert.h>
#include <stdlib.h>
#include <malloc.h>
#include "../Utils/ColorUtils.h"

namespace AdjustDij_NoAtomic4PadUtils {
    static const int PUSH_RETRY_TIMES = 5;
    static const int CAPACITY = 256;
}

using Utils::Node;
using Utils::LeanHeap;

class AdjustDij_NoAtomic4Pad: public Solver<AdjacentMatrix> {
    char name[1024];
    const int THREAD_NUM, NUM_RELAX_EACH_ITERATION;

    Heaps *heaps;
    LeanHeap *lheaps[240];
    typedef Utils::TwoDConcurrentQueue<AdjustDij_NoAtomic4PadUtils::CAPACITY> TwoDConcurrentQueueType;

    TwoDConcurrentQueueType *queues;

    const int *colors;

    enum State {
        STATE_ACTIVE, STATE_READY_TO_STOP
    };

    volatile int *versions;

    /*
     * try to relax nodes belong to current thread according to current knowledge, and update new value.
     */

  void relax(const int &pid, AdjacentMatrix *matrix, int &totalScan, Node* ng, LeanHeap* lheap) {
        AdjacentMatrix::Arch *&archArray = matrix->archArray;
        Utils::Item data;
        int &src_node = data.key, &src_result = data.value;
        int left = NUM_RELAX_EACH_ITERATION;
        while (left > 0 && !heaps->empty(pid)) {
            heaps->removeTop(pid, &data);
            int curr = src_node;
            /*
             * relax according to src_node
             */
            bool push_ret = true;
#ifdef PERF_SCAN
            ++totalScan;
#endif
            // for (int i = g[src_node]; i < g[src_node + 1]; ++i) {
            //     int &dst_node = archArray[i].j, &weight = archArray[i].w, new_dst_result = src_result + weight;
#pragma unroll
            for (int i = 0; i < 4; ++i) {
              int &dst_node = ng[curr].to[i];
              int &weight = ng[curr].w[i], new_dst_result = src_result + weight;
                /*
                 * case colors[dst_node]==pid, we get right result[dst_node]
                 * case colors[dst_node]!=pid, current thread may have a cache value, but the cache value must be bigger than the actural one.
                 */
                if (ng[curr].tocolor[i] == pid && ng[dst_node].dist > new_dst_result) {
                  ng[dst_node].dist = new_dst_result;
                  heaps->addOrUpdate(pid, dst_node, new_dst_result);
                } else if (ng[curr].tocolor[i] != pid) {
                  push_ret = false;
                  Utils::SmallConcurrentQueue<AdjustDij_NoAtomic4Utils::CAPACITY>* dst_queue = queues->get(ng[curr].tocolor[i], pid);
                  for (int tmp = 0; tmp < AdjustDij_NoAtomic4Utils::PUSH_RETRY_TIMES; ++tmp) {
                    push_ret = dst_queue->push(dst_node, new_dst_result);
                    if (push_ret) break;
                  }
                  if (!push_ret) break;
                }
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

  void adjust(const int &pid, AdjacentMatrix *matrix, Node* ng, LeanHeap* lheap) {
        Utils::Item data;
        int &src_node = data.key, &src_result = data.value;
        for (int i = 0; i < THREAD_NUM; ++i) {
            while (queues->get(pid, i)->pop(data))
                if (ng[src_node].dist > src_result) {
                    ng[src_node].dist = src_result;
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

    void worker(AdjacentMatrix *matrix, int &totalScan, Node* ng, LeanHeap* lheap) {
        const int pid = omp_get_thread_num();
        int &myVersion = ((int*) versions)[pid*64];
        int localVersions[THREAD_NUM];
        bool more = true;
        while (more) {
            if (myVersion >= 0) {
              relax(pid, matrix, totalScan, ng, lheap);
              adjust(pid, matrix, ng, lheap);
              if (isHeapQueueEmpty(pid)) myVersion = ~((myVersion + 1) & 0x80000000);
            } else {
                if (!isHeapQueueEmpty(pid)) {
                    myVersion = ~myVersion;
                    continue;
                }
                bool someStateIsActive = false;
                for (int i = 0; i < THREAD_NUM && !someStateIsActive; ++i) {
                    if (versions[i*64] >= 0) someStateIsActive = true;
                    localVersions[i] = versions[i*64];
                }
                if (someStateIsActive) continue;
                bool isFinished = true;
                for (int i = 0; i < THREAD_NUM && !someStateIsActive && isFinished; ++i) {
                    if (versions[i*64] >= 0) someStateIsActive = true;
                    if (localVersions[i] != versions[i*64]) isFinished = false;
                }
                if (someStateIsActive) continue;
                if (isFinished) more = false;
                break;
            }
        }
    }

public:
    AdjustDij_NoAtomic4Pad(int thread_num, int num_relax_each_iteration) :
            THREAD_NUM(thread_num), NUM_RELAX_EACH_ITERATION(num_relax_each_iteration), heaps(NULL), queues(NULL), colors(NULL), versions(NULL) {
        sprintf(name, "Dijstra + Adjustment + NoAtomic4Pad: THREAD_NUM=%d, NUM_RELAX_EACH_ITERATION=%d", THREAD_NUM, NUM_RELAX_EACH_ITERATION);
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
        adjMatrix.reallocResultPad();
        int *&result = adjMatrix.result;
        adjMatrix.resetResultPad();

        /*
         * create heaps and queues for each thread
         */
        heaps = new Heaps(adjMatrix.nafterpad, THREAD_NUM, colors->getNodeNumberCount());

        queues = new TwoDConcurrentQueueType(THREAD_NUM);
        for (int i = 0; i < THREAD_NUM; i++) {
        }
        /*
         * create sync protocol for each thread
         */
        versions = (int *) _mm_malloc(sizeof(int) * THREAD_NUM * 64, 64);
        memset((void*) versions, 0, sizeof(int) * THREAD_NUM * 64);

        prepareTimer.Stop();


        // Step 2: start to compute
        runningTimer.Start();

        result[source] = 0;
        heaps->add(this->colors[source], source, 0);
        AdjacentMatrix::Arch *&archArray = adjMatrix.archArray;
        int *&g = adjMatrix.g;
        Node* ng = (Node*)_mm_malloc(sizeof(Node)* (adjMatrix.nafterpad), 256);
        assert(ng != NULL);
        for (int i = 0; i < adjMatrix.nafterpad; i++) {
          ng[i].dist = result[i];
          ng[i].heapIndex = -1;
          for (int j = 0; j < 4; j++) {
            assert(adjMatrix.tos[i*4+j] != -1);
            ng[i].to[j] = adjMatrix.tos[i*4+j];
            assert(ng[i].to[j] >= 0 && ng[i].to[j] < adjMatrix.nafterpad);
            ng[i].w[j] = adjMatrix.ws[i*4+j];
            assert(ng[i].w[j] >= 0);
            ng[i].tocolor[j] = this->colors[ng[i].to[j]];
          }
        }
        int sum = 0;
        for (int i = 0; i < adjMatrix.nafterpad; i++) {
          sum += ng[i].to[0];
          assert(ng[i].heapIndex == -1);
        }
        double slowest = 0; int sSum = 1; int maxCap = 0;
#pragma omp parallel
        {

          int pid = omp_get_thread_num();
#pragma omp barrier

            RdtscTimer localTimer;
#pragma noinline
            localTimer.Start();
            int ts = 0;
            worker(&adjMatrix, ts, ng, lheaps[pid]);
#pragma noinline
            localTimer.Stop();

#ifdef PERF_SCAN
#pragma omp atomic
            totalScan += ts;
#endif
#pragma omp critical
            {
              slowest = std::max(localTimer.GetSecond(), slowest);
              //maxCap = std::max(maxCap, lheaps[pid]->cap);
              sSum += sum;
              //delete lheaps[pid];
            }
        }
        runningTimer.Stop();
        for (int i = 0; i <= adjMatrix.nafterpad; i++) {
          result[i] = ng[i].dist;
        }
        _mm_free(ng);
        printf("[NoAtom4Pad] slowest = %lf sSum = %d maxCap = %d\n", slowest, sSum, maxCap);
        printf("[NoAtom4Pad] checkSumPad = 0x%x, checkSumPadNoI = 0x%x\n",
               adjMatrix.checkSumPad(), adjMatrix.checkSumPadNoI());
        // Step 3: destroy environment
        prepareTimer.Start();

        /*
         * destroy sync protocol for each thread
         */
        _mm_free((void *) versions);

        /*
         * destroy heaps and queues for each thread
         */
        delete queues;
        delete heaps;

        prepareTimer.Stop();

        runningTimeS = slowest;
        prepareTimeS = prepareTimer.GetSecond();
#ifdef PERF_SCAN
        avgScan = totalScan / THREAD_NUM;
#endif

    }
};

#endif
