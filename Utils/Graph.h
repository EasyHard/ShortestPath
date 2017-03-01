#ifndef GRAPH_H_
#define GRAPH_H_


class Colors;

class BaseGraph {
protected:
    void allocateBuf();
public:

    int n, m, nafterpad;
    int *result;
    static const int RESULT_MAX_VALUE;

    BaseGraph();

    BaseGraph(int n, int m);

    virtual ~BaseGraph();

    int checkSum();
    int checkSumNoI();
    void resetResult();

    void showResult();

    virtual void print()=0;


};
class AdjacentMatrix: public BaseGraph {

public:

    struct Arch {
        int i, j, w;
    };
    int *tos;
    int *ws;
    int *padcolor;
    int *padold;
    int *newNodeMap;
    int *nnewnode;
  struct ToArch {
    int to;
    int w;
  };
  int* data;
  ToArch *toarch;
protected:
    void buildIndex();

    void readFromTextFile(const char * const fileName);

    void readFromObjectFile(const char * const fileName);

    void allocateBuf();

    void freeBuf();

public:
    void releasePadArray();
    Arch* archArray;
    int *g;       // i g[i]<= <g[i+1] invalid:g[i]==-1

    AdjacentMatrix(const char * const fileName);

    AdjacentMatrix(int n, int m, Arch *archArray);
    int checkSumPad();
    int checkSumPadNoI();
    void resetResultPad();
    void reallocResultPad();
  void buildSimple();
  int padsoft(int npad);
  void pad(int npad, int* color);
  void removeCutEdges(const int* colors);
    virtual ~AdjacentMatrix();

    void dumpToObjectFile(const char * const fileName);

    Arch *getArchArray();

    virtual void print();
};

class CacheFriendlyAdjacentMatrix: public AdjacentMatrix {
    friend void reorg(CacheFriendlyAdjacentMatrix &graph, Colors &color);
    friend void unreorg(CacheFriendlyAdjacentMatrix &graph, Colors &color);
protected:
    void allocateBuf();
    void freeBuf();
public:
    int *index, *rindex;

    CacheFriendlyAdjacentMatrix(const char * const fileName);
    CacheFriendlyAdjacentMatrix(int n, int m, Arch *archArray);
    virtual ~CacheFriendlyAdjacentMatrix();
};

#endif
