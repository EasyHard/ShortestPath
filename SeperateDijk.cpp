#include <stdio.h>

#include "Utils/Graph.h"
#include "Solver/Solver.h"

#include "Utils/Dataset.h"

#include "Test/TestCase.h"
#include "Test/TestPerf.h"

#include "Utils/Config.h"

#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <assert.h>


int main(int argc, char **argv) {
  Config::readFromFile("Conf/SeperateDijk.conf");
  int source = atoi(conf["source"].c_str());
  int bfsdelta = atoi(conf["bfsdelta"].c_str());
  const char *DATA_SRC = conf["graph"].c_str();
  const char *DATA_SRC2 = conf["color"].c_str();
  int nthread = atoi(conf["nthread"].c_str());
  const char *SOURCE_PATH = conf["source_path"].c_str();

  double perf[10];
  CacheFriendlyAdjacentMatrix g(DATA_SRC);
  source = source % g.n;
  Colors colors;
  if (bfsdelta != -1 && bfsdelta != 0) {
    colors.color_BFS_DeltaDepth(g, 0, 16);
  } else {
    colors.readFromObjectFile(g, DATA_SRC2);
  }

  if (conf["reorg"] != "") {
    int max = -1, min = 0x7fffffff;
    for (int i = 0; i < g.n; i++) {
      if (max < colors.getColors()[i]) max = colors.getColors()[i];
      if (min > colors.getColors()[i]) min = colors.getColors()[i];
    }
    printf("max = %d, min = %d\n", max, min);
    colors.refine(max+1);
    colors.printSummary();
    reorg(g, colors);
    source = g.index[source];
    colors.refine(nthread);
    colors.printSummary();
    reorg(g, colors);
    source = g.index[source];
  } else {
    colors.refine(nthread);
  }
  double sum = 0;
  if (conf["pad"] != "") {
    g.pad(4, colors.colors);
    Colors padcolors;
    padcolors.fromArray(g.nafterpad, g.padcolor);
    padcolors.refine(nthread);
    g.reallocResultPad();
    SeperateDijk solver(nthread, SOURCE_PATH, 32);
    TestCase::runTest(g, &padcolors, g.newNodeMap[source], &solver, true, false, 4, &perf[0]);
    printf("checkSumPadNoI = 0x%x\n", g.checkSumPadNoI());
  } else {
    SeperateDijk solver(nthread, SOURCE_PATH, 32);
    TestCase::runTest(g, &colors, source, &solver, true, false, 4, &perf[0]);
    printf("checkSumNoI = 0x%x\n", g.checkSumNoI());
  }
  for (int i = 1; i < 4; i++) sum += perf[i];
  printf("average = %lf\n", sum/3);
  return 0;
}

