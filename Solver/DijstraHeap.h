#ifndef DIJSTRA_HEAP_H_
#define DIJSTRA_HEAP_H_

#include "../Utils/Utils.h"

class DijstraHeap: public Solver<AdjacentMatrix> {
public:
    const char * getName() {
        return "Dijstra + Heap";
    }

    void compute(AdjacentMatrix &adjMatrix, Colors *colors, int source, double &prepareTimeS, double &runningTimeS, double &totalScan, double &avgScan) {
        CycleTimer prepareTimer, runningTimer;
#ifdef PERF_SCAN
        unsigned int ts = 0;
#endif

        // Step 1: prepare Environment
        prepareTimer.Start();

        int &n = adjMatrix.n;
        AdjacentMatrix::Arch *&archArray = adjMatrix.archArray;
        int *&g = adjMatrix.g;

        int *&result = adjMatrix.result;
        adjMatrix.resetResult();

        Utils::Heap heap(n);

        prepareTimer.Stop();

        // Step 2: start to compute
        runningTimer.Start();
        result[source] = 0;
        heap.add(source, result[source]);
        for (int i = 0; i < n && !heap.empty(); ++i) {
            const Utils::Item top = heap.top();
            const int &src = top.key;
            heap.removeTop(NULL);
#ifdef PERF_SCAN
            ++ts;
#endif
            for (int j = g[src]; j < g[src + 1]; ++j) {
                const int &dst = archArray[j].j, &weight = archArray[j].w;
                if (result[dst] > result[src] + weight) {
                    result[dst] = result[src] + weight;
                    heap.addOrUpdate(dst, result[dst]);
                }
            }
        }

        runningTimer.Stop();

        // Step 3: destroy environment
        prepareTimer.Start();
        prepareTimer.Stop();

        runningTimeS = runningTimer.GetSecond();
        prepareTimeS = prepareTimer.GetSecond();
        printf("checkSumNoI = 0x%x\n", adjMatrix.checkSumNoI());
#ifdef PERF_SCAN
        totalScan =ts;
        avgScan = totalScan;
#endif
    }
};

#endif
