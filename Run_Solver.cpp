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
  srand(time(NULL));
  FILE* file = fopen("runSolver.data", "a");
  assert(file != NULL);
  Config::readFromFile("Conf/Run_Solver.conf");
  int nloop = atoi(conf["nloop"].c_str());
  for (int i = 0; i < nloop; i++) {
    int source = atoi(conf["source"].c_str());
    if (source == -1) {
      source = rand();
    }
    int stthread = atoi(conf["stthread"].c_str());
    int edthread = atoi(conf["edthread"].c_str());
    for (int nthread = stthread; nthread <= edthread; nthread += 64) {
      if (conf["reorg"] != "") {
        TestCase::runSolverReorg(conf["graph"].c_str(),
                                 conf["color"].c_str(),
                                 nthread,
                                 source,
                                 atoi(conf["bfsdelta"].c_str()),
                                 atoi(conf["nrelaxiter"].c_str()),
                                 atoi(conf["ntimes"].c_str()),
                                 file
                                 );
      } else {
        TestCase::runSolverNoReorg(conf["graph"].c_str(),
                                   conf["color"].c_str(),
                                   nthread,
                                   source,
                                   atoi(conf["bfsdelta"].c_str()),
                                   atoi(conf["nrelaxiter"].c_str()),
                                   atoi(conf["ntimes"].c_str()),
                                   file
                                   );
      }
    }
  }
  fclose(file);
  return 0;
}

