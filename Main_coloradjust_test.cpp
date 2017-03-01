#include <stdio.h>

#include "Utils/Graph.h"
#include "Solver/Solver.h"

#include "Utils/Dataset.h"

#include "Test/TestCase.h"
#include "Test/TestPerf.h"

int main(int argc, char **argv) {
    const char * DATA_SRC = Dataset::DATA_CTR_OBJ, *DATA_SRC2 = Dataset::DATA_CTR_COLOR64;
    AdjacentMatrix g(DATA_SRC);
    Colors colors;
    colors.color_BFS_DeltaDepth(g, 0, 32);
    //colors.readFromObjectFile(g, DATA_SRC2);
    int max = -1, min = 0x7fffffff;
    for (int i = 0; i < g.n; i++) {
      if (max < colors.getColors()[i]) max = colors.getColors()[i];
      if (min > colors.getColors()[i]) min = colors.getColors()[i];
    }
    printf("maxColor = %d, minColor = %d\n", max, min);
    colors.refine(max+1);
    int threadNum = 120;
    colors.rebalance(threadNum, max+1);
    colors.refine(max+1);
    colors.printSummary();
    return 0;
}

