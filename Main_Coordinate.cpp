#include <stdio.h>

#include "Utils/Graph.h"
#include "Utils/ColorUtils.h"
#include "Utils/Dataset.h"
#include "Utils/Coordinate.h"

int main(int argc, char **argv) {
  const char* DATA_SRC = Dataset::DATA_CTR_OBJ_CO;
  Coordinate co = Coordinate::fromObjFile(DATA_SRC);
  


  // const char* DATA_SRC = Dataset::DATA_CTR_CO;
  // Coordinate co = Coordinate::fromTextFile(DATA_SRC);
  // co.dumpObjFile(Dataset::DATA_CTR_OBJ_CO);
  return 0;
}

