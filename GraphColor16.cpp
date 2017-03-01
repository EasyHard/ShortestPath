#include <stdio.h>

#include "Utils/Graph.h"
#include "Solver/Solver.h"

#include "Utils/Dataset.h"

#include "Test/TestCase.h"
#include "Test/TestPerf.h"

#include <climits>
int main(int argc, char **argv) {
  const char * objs[] = {Dataset::DATA_USA_OBJ, Dataset::DATA_CTR_OBJ, Dataset::DATA_W_OBJ, Dataset::DATA_E_OBJ, Dataset::DATA_LKS_OBJ, Dataset::DATA_CAL_OBJ};
  {
    AdjacentMatrix g(Dataset::DATA_USA_OBJ);
    Colors colors;
    colors.color_BFS_DeltaDepth(g, 0, 16);
    colors.dumpToObjectFile(Dataset::DATA_USA_COLOR16);
  }
  {
    AdjacentMatrix g(Dataset::DATA_CTR_OBJ);
    Colors colors;
    colors.color_BFS_DeltaDepth(g, 0, 16);
    colors.dumpToObjectFile(Dataset::DATA_CTR_COLOR16);
  }
  {
    AdjacentMatrix g(Dataset::DATA_W_OBJ);
    Colors colors;
    colors.color_BFS_DeltaDepth(g, 0, 16);
    colors.dumpToObjectFile(Dataset::DATA_W_COLOR16);
  }
  {
    AdjacentMatrix g(Dataset::DATA_E_OBJ);
    Colors colors;
    colors.color_BFS_DeltaDepth(g, 0, 16);
    colors.dumpToObjectFile(Dataset::DATA_E_COLOR16);
  }
  {
    AdjacentMatrix g(Dataset::DATA_LKS_OBJ);
    Colors colors;
    colors.color_BFS_DeltaDepth(g, 0, 16);
    colors.dumpToObjectFile(Dataset::DATA_LKS_COLOR16);
  }
  {
    AdjacentMatrix g(Dataset::DATA_CAL_OBJ);
    Colors colors;
    colors.color_BFS_DeltaDepth(g, 0, 16);
    colors.dumpToObjectFile(Dataset::DATA_CAL_COLOR16);
  }
  return 0;
}

