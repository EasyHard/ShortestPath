#include <stdio.h>

#include "Utils/Graph.h"
#include "Solver/Solver.h"

#include "Utils/Dataset.h"

#include "Test/TestCase.h"
#include "Test/TestPerf.h"

#include <stdio.h>
#include <time.h>

int main(int argc, char **argv) {
  const char * objs[] = {Dataset::DATA_USA_OBJ, Dataset::DATA_CTR_OBJ, Dataset::DATA_W_OBJ, Dataset::DATA_E_OBJ, Dataset::DATA_LKS_OBJ, Dataset::DATA_CAL_OBJ};
#ifndef IS_MIC
  FILE* file = fopen("Dijkstra.data", "w");
  const int nobjs = sizeof(objs) / sizeof(char*);
  const int nloop = 5, ntimes = 5;
  srand(time(NULL));
  for (int i = 0; i < nobjs; i++) {
    double perf[nloop*ntimes];
    for (int j = 0; j < nloop; j++) {
      Solver<AdjacentMatrix> *pSolver = NULL;
      pSolver = new DijstraHeap();
      AdjacentMatrix g(objs[i]);
      int source = rand() % g.n;
      Colors c;
      TestCase::runTest(g, &c, 0, pSolver, true, false, ntimes, perf + (ntimes*j));
    }
    double sum = 0, max = 0, min = 1e9;
    for (int j = 0; j < nloop*ntimes; j++) {
      sum += perf[j];
      max = std::max(perf[j], max);
      min = std::min(perf[j], min);
    }
    fprintf(file, "%lf\t%lf\t%lf\n", sum/(ntimes*nloop), max, min);
  }
  fclose(file);

#endif
    return 0;
}

