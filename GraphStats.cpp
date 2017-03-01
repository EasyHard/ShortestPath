#include <stdio.h>

#include "Utils/Graph.h"
#include "Solver/Solver.h"

#include "Utils/Dataset.h"

#include "Test/TestCase.h"
#include "Test/TestPerf.h"

#include <climits>
int main(int argc, char **argv) {
  const char * objs[] = {Dataset::DATA_USA_OBJ, Dataset::DATA_CTR_OBJ, Dataset::DATA_W_OBJ, Dataset::DATA_E_OBJ, Dataset::DATA_LKS_OBJ, Dataset::DATA_CAL_OBJ};
  for (int i = 0; i < sizeof(objs) / sizeof (char*); i++) {
    AdjacentMatrix g(objs[i]);
    int nmaxArc = 0; int nminArc = INT_MAX;
    for (int j = 0; j < g.n; j++) {
      nmaxArc = std::max(nmaxArc, g.g[j+1] - g.g[j]);
      nminArc = std::min(nminArc, g.g[j+1] - g.g[j]);
    }

    printf("%f %f %f %d %d %d\n", g.n/1024.0/1024, g.m/1024.0/1024.0, float(g.m)/g.n, nmaxArc, nminArc, g.padsoft(4));
  }
  return 0;
}

