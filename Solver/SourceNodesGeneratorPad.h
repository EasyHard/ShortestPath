#ifndef SOURCE_NODES_GENERATOR_PAD_H_
#define SOURCE_NODES_GENERATOR_PAD_H_

#include "../Utils/Utils.h"
#include "../Utils/Dataset.h"

#include <vector>
#include <map>

class SourceNodesGeneratorPad: public Solver<AdjacentMatrix> {
  const char * SOURCE_SRC;
public:
 SourceNodesGeneratorPad(const char * SOURCE_SRC) : SOURCE_SRC(SOURCE_SRC) {}

    const char * getName() {
        return "SourceNodesGeneratorPad";
    }
    const int *colors, *label, *unlabel;
    void compute(AdjacentMatrix &adjMatrix, Colors *colors, int source, double &prepareTimeS, double &runningTimeS, double &totalScan, double &avgScan) {
        CycleTimer prepareTimer, runningTimer;
#ifdef PERF_SCAN
        unsigned int ts = 0;
#endif
        // Step 1: prepare Environment
        prepareTimer.Start();

        this->colors = colors->getColors();
        this->label = colors->getlabel();
        this->unlabel = colors->getunlabel();
        int n = adjMatrix.nafterpad;
        AdjacentMatrix::Arch *&archArray = adjMatrix.archArray;
        int *&g = adjMatrix.g;
        free(adjMatrix.result);
        adjMatrix.result = (int*) malloc(sizeof(int) * n);
        int *&result = adjMatrix.result;
        for (int i = 0; i < n; i++) {
          result[i] = BaseGraph::RESULT_MAX_VALUE;
        }

        Utils::Heap heap(n);
        std::map<int, int> parents;
        prepareTimer.Stop();
        std::vector<int> sourceNodes;
        for (int j = g[0]; j < g[1]; ++j) {
          const int &dst = archArray[j].j, &weight = archArray[j].w;
          printf("dst = %d, color[dst] = %d, w = %d\n", dst, this->colors[dst], weight);
        }
        for (int j = 0; j < 4; j++) {
          printf("tos[%d] = %d\n", j, adjMatrix.tos[j]);
        }
        // Step 2: start to compute
        runningTimer.Start();
        result[source] = 0;
        heap.add(source, result[source]);
        parents[source] = -1;
        printf("nafterpad = %d\n", adjMatrix.nafterpad);
        for (int i = 0; i < adjMatrix.nafterpad && !heap.empty(); ++i) {
            const Utils::Item top = heap.top();
            const int &src = top.key;
            const int &parent = parents[src];
            if (parent == -1 || this->colors[parent] != this->colors[src])
              sourceNodes.push_back(src);
            heap.removeTop(NULL);
#ifdef PERF_SCAN
            ++ts;
#endif
            // /////
            // // No pad
            // /////
            // for (int j = g[src]; j < g[src + 1]; ++j) {
            //     const int &dst = archArray[j].j, &weight = archArray[j].w;
            //     if (result[dst] > result[src] + weight) {
            //         result[dst] = result[src] + weight;
            //         heap.addOrUpdate(dst, result[dst]);
            //         parents[dst] = src;
            //     }
            // }

            /////
            // paded
            /////
            for (int j = 0; j < 4; ++j) {
              const int &dst = adjMatrix.tos[src*4+j], &weight = adjMatrix.ws[src*4+j];
              if (dst == -1) break;
                if (result[dst] > result[src] + weight) {
                    result[dst] = result[src] + weight;
                    heap.addOrUpdate(dst, result[dst]);
                    parents[dst] = src;
                }
            }
        }
        runningTimer.Stop();
        assert(heap.empty());
        int nunreached = 0;
        for (int i = 0; i < adjMatrix.nafterpad; i++) {
          if (adjMatrix.result[i] == BaseGraph::RESULT_MAX_VALUE) {
            nunreached++;
            if (nunreached < 20) {
              printf("unreached i = %d\n", i);
              for (int j = 0; j < 4; j++) {
                printf("(to %d, w %d) ", adjMatrix.tos[i*4+j], adjMatrix.ws[i*4+j]);
              }
              printf("\n");
              printf("old = %d\n", adjMatrix.padold[i]);
              int old = adjMatrix.padold[i];
              for (int j = adjMatrix.g[old]; j < adjMatrix.g[old+1]; j++) {
                printf("[to %d, w %d]\n", adjMatrix.archArray[j].j, adjMatrix.archArray[j].w);
              }
              printf("\n");
            }
          }
        }
        printf("nunreached = %d\n", nunreached);
        // Step 3: destroy environment
        prepareTimer.Start();
        prepareTimer.Stop();

        runningTimeS = runningTimer.GetSecond();
        prepareTimeS = prepareTimer.GetSecond();
#ifdef PERF_SCAN
        totalScan =ts;
        avgScan = totalScan;
#endif
        printf("dijk finisted\n# of source node = %zu, checksumNoI = 0x%x\n", sourceNodes.size(), adjMatrix.checkSumPadNoI());
        std::map<int, int> tcn, scn;
        for (int i = 0; i < n; i++)
          tcn[this->colors[i]]++;
        for (int i = 0; i < sourceNodes.size(); i++) {
          scn[this->colors[sourceNodes[i]]]++;
        }
        for (int i = 0; i < colors->threadNum; i++)
          printf("color = %d, # of source = %d, # of node = %d\n", i, scn[i], tcn[i]);
        FILE* file = fopen(SOURCE_SRC, "wb");
        if (file == NULL) {
          perror("file fopen");
        }
        size_t nnode = sourceNodes.size();
        fwrite(&nnode, sizeof(size_t), 1, file);
        for (int i = 0; i < nnode; i++) {
          fwrite((&sourceNodes[0]) + i, sizeof(int), 1, file);
          fwrite(result+sourceNodes[i], sizeof(int), 1, file);
        }
        fclose(file);
        printf("dijk finisted\n# of source node = %zu, checksumNoI = 0x%x\n", sourceNodes.size(), adjMatrix.checkSumPadNoI());
        checkSourceNode(adjMatrix, adjMatrix.checkSumPadNoI());
    }
    void checkSourceNode(AdjacentMatrix &adjMatrix, int checkSum) {
        int &n = adjMatrix.nafterpad;
        AdjacentMatrix::Arch *&archArray = adjMatrix.archArray;
        int *&g = adjMatrix.g;
        int *&result = adjMatrix.result;
        adjMatrix.resetResultPad();
        Utils::Heap heap(n);
        FILE* file = fopen(SOURCE_SRC, "rb");
        size_t nnode;
        fread(&nnode, sizeof(size_t), 1, file);
        for (int i = 0; i < nnode; i++) {
          int node, val;
          fread(&node, sizeof(int), 1, file);
          fread(&val, sizeof(int), 1, file);
          heap.addOrUpdate(node, val);
        }
        fclose(file);
        for (int i = 0; i < n && !heap.empty(); ++i) {
            const Utils::Item top = heap.top();
            const int &src = top.key;
            const int &value = top.value;
            result[src] = value;
            heap.removeTop(NULL);
            for (int j = 0; j < 4; ++j) {
              const int &dst = adjMatrix.tos[src*4+j], &weight = adjMatrix.ws[src*4+j];
              if (dst == -1) break;
              if (this->colors[dst] == this->colors[src]) {
                if (result[dst] > result[src] + weight) {
                  result[dst] = result[src] + weight;
                  heap.addOrUpdate(dst, result[dst]);
                }
              }
            }
        }
        int nunreached = 0;
        for (int i = 0; i < n; i++) {
          if (adjMatrix.result[i] == BaseGraph::RESULT_MAX_VALUE) {
            nunreached++;
          }
        }
        printf("nunreached = %d\n", nunreached);
        printf("checkSumNoI = 0x%x\n", adjMatrix.checkSumPadNoI());
        assert(adjMatrix.checkSumPadNoI() == checkSum);
    }
};

#endif

