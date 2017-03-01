#ifndef COORDINATE_H
#define COORDINATE_H
#include "Graph.h"
#include <vector>
#include <map>

using std::vector;
struct Location {
  int x, y, id;
};

class Coordinate {
 public:
  int n;
  Location* location;
  Coordinate();
  ~Coordinate();
  void fromTextFile(const char * fileName);
  void fromObjFile(const char * fileName);
  int getX(int id);
  int getY(int id);
  void dumpObjFile(const char * fileName);
private:
  std::map<int, Location> nodemap;
};

class TwoFoldSpliter {

 public:

  struct TFSItem {
    Location loc;
    int link;
  };

  AdjacentMatrix& adjMatrix;
  Coordinate& co;
  int bound;
  int* colors;
  int* pos;
  int* itempos;
  bool* moved;
  int currColor;

  void linkSort(TFSItem* items[2], int st, int ed, int CURR);
  void doSplit(TFSItem* items[2], int st, int ed, int CURR);
  void rotate(TFSItem* items[2], int st, int ed, int CURR);
  TwoFoldSpliter(AdjacentMatrix &adjMatrix, Coordinate& co, int bound, int *colors)
    : adjMatrix(adjMatrix), co(co), bound(bound), colors(colors), currColor(0)
    {
      pos = new int[adjMatrix.n];
      moved = new bool[adjMatrix.n];
      itempos = new int[adjMatrix.n];
    }
  ~TwoFoldSpliter() {
    delete [] pos;
    delete [] moved;
    delete [] itempos;
  }
  void split();
};

class CoordinateSpliter {
 private:
  AdjacentMatrix& adjMatrix;
  Coordinate& co;
  int nTmpColor;
  int nsplit;
  int* colors;
  vector<int> splitHelper(int nSplit, Location* st, Location* ed);
  int costByX(int nSplit, Location *st, Location* ed);
  int costByY(int nSplit, Location *st, Location* ed);
  int getNewColor();
 public:
  CoordinateSpliter(AdjacentMatrix &adjMatrix, Coordinate& co, int nsplit, int* colors)
    : adjMatrix(adjMatrix), co(co), nsplit(nsplit), colors(colors) {}
  int *split();
};
#endif
