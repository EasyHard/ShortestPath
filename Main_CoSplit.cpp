#include <stdio.h>

#include "Utils/Graph.h"
#include "Utils/ColorUtils.h"
#include "Utils/Dataset.h"
#include "Utils/Coordinate.h"

#include "Utils/RdtscTimer.h"

int main(int argc, char **argv) {

  CacheFriendlyAdjacentMatrix g(Dataset::DATA_E_OBJ);
  Coordinate co; co.fromTextFile(Dataset::DATA_E_CO);
  Colors colors;
  //colors.coordinateSplit(g, co, 65536);
  //colors.refine(65536);
  printf("start twoFold\n");
  RdtscTimer timer; timer.Start();
  int ncolor = colors.twoFoldSplit(g, co, 128);
  timer.Stop(); printf("time = %lf\n", timer.GetSecond());
  colors.refine(ncolor);
  colors.dumpToObjectFile("./Dataset/USA-road-d.E.obj.cocolor128");
  colors.printSummary();
  printf("# of cut edges = %d\n", colors.numOfCutEdge(g));
  timer.Start();
  colors.color_BFS_DeltaDepth(g, 0, 16);
  timer.Stop(); printf("bfs time = %lf\n", timer.GetSecond());
  int max = -1, min = 0x7fffffff;
  for (int i = 0; i < g.n; i++) {
    if (max < colors.getColors()[i]) max = colors.getColors()[i];
    if (min > colors.getColors()[i]) min = colors.getColors()[i];
  }
  printf("max = %d, min = %d\n", max, min);
  colors.refine(max+1);
  colors.printSummary();
  printf("# of cut edges = %d\n", colors.numOfCutEdge(g));
  // 256  avg=220028.375000 min=87187 max=367780
  // 128  avg=220028.375000 min=157023 max=295399
  // 64   avg=220028.375000 min=174399 max=288137


  return 0;
}

