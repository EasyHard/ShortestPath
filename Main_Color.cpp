#include <stdio.h>

#include "Utils/Graph.h"
#include "Utils/ColorUtils.h"
#include "Utils/Dataset.h"

int main(int argc, char **argv) {

    AdjacentMatrix g(Dataset::DATA_CTR_OBJ);
    Colors colors;

    int ncolor = colors.color_BFS_DeltaDepth(g, 0, 256);
    colors.dumpToObjectFile(Dataset::DATA_CTR_COLOR);
    printf("ncolor = %d\n", ncolor);
    colors.refine(4);
    colors.printSummary();

    Colors colors64;
    ncolor = colors64.color_BFS_DeltaDepth(g, 0, 64);
    colors64.dumpToObjectFile(Dataset::DATA_CTR_COLOR64);
    printf("ncolor = %d\n", ncolor);
    colors64.refine(64);
    colors64.printSummary();

    // 256  avg=220028.375000 min=87187 max=367780
    // 128  avg=220028.375000 min=157023 max=295399
    // 64   avg=220028.375000 min=174399 max=288137


    return 0;
}

