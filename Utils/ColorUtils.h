#ifndef COLOR_UTILS_H_
#define COLOR_UTILS_H_

#include "Coordinate.h"

class AdjacentMatrix;
class CacheFriendlyAdjacentMatrix;

class Colors {
    friend void reorg(CacheFriendlyAdjacentMatrix &graph, Colors &color);
    friend void unreorg(CacheFriendlyAdjacentMatrix &graph, Colors &color);
    friend void buildColorLabel(CacheFriendlyAdjacentMatrix &graph, Colors &color);
    friend void unBuildColorLabel(CacheFriendlyAdjacentMatrix &graph, Colors &color);
public:
    int n, threadNum;
    int *colors, *nodeNumberCount, *label, *unlabel;
private:


    inline void releaseNodeNumberCount();

    inline void release();

    inline void releaseAndAllocate();

public:
    Colors();

    void dumpToObjectFile(const char * const fileName) const;

    const int * getColors() const;

    const int * getlabel() const;

    const int * getunlabel() const;

    const int * getNodeNumberCount() const;

    void print() const;

    void printSummary() const;

    ~Colors();
    void rebalance(int threadNum, int nColor, int threadhold = 20000);
    void swapColor(int c1, int c2);
    void readFromObjectFile(AdjacentMatrix &adjMatrix, const char * const fileName);
    void fromArray(int n, int* colorArray);

    void color_from_file(AdjacentMatrix &adjMatrix, const char* filename, int threadNum);
    int color_BFS_DeltaDepth(AdjacentMatrix &adjMatrix, int source, int deltaDepth);
    void coordinateSplit(AdjacentMatrix &adjMatrix, Coordinate& coordiante, int nsplit);
    int twoFoldSplit(AdjacentMatrix &adjMatrix, Coordinate& co, int bound);
    void refine(int threadNum);
    int numOfCutEdge(AdjacentMatrix& adjMatrix);
};

void reorg(CacheFriendlyAdjacentMatrix &graph, Colors &color);
void unreorg(CacheFriendlyAdjacentMatrix &graph, Colors &color);
void reorgResult(CacheFriendlyAdjacentMatrix &graph);

#endif
