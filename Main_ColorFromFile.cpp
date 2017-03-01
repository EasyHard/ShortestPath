#include <stdio.h>

#include "Utils/Graph.h"
#include "Utils/ColorUtils.h"
#include "Utils/Dataset.h"

int main(int argc, char **argv) {

    AdjacentMatrix g(Dataset::DATA_CTR_OBJ);
    Colors colors;

    colors.color_from_file(g, "./tmppartition180", 180);
    colors.dumpToObjectFile(Dataset::DATA_CTR_COLOR180);
    colors.printSummary();

     // 256  avg=220028.375000 min=87187 max=367780
    // 128  avg=220028.375000 min=157023 max=295399
    // 64   avg=220028.375000 min=174399 max=288137


    return 0;
}

