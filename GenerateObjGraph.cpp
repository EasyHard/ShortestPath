#include <stdio.h>

#include "Utils/Graph.h"
#include "Solver/Solver.h"

#include "Utils/Dataset.h"

#include "Test/TestCase.h"
#include "Test/TestPerf.h"

int main(int argc, char **argv) {
  const char * srcs[] = {Dataset::DATA_CTR_TEXT};
  const char * objs[] = {Dataset::DATA_CTR_OBJ};
  for (int i = 0; i < sizeof(srcs) / sizeof (char*); i++) {
    AdjacentMatrix g(srcs[i]);
    g.dumpToObjectFile(objs[i]);
  }
  return 0;
}

