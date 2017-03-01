#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <map>
#include "ColorUtils.h"
#include "Graph.h"


inline void Colors::releaseNodeNumberCount() {
    if (nodeNumberCount != NULL) {
        free(nodeNumberCount);
        nodeNumberCount = label = unlabel = NULL;
    }
}

inline void Colors::release() {
    if (colors != NULL) {
        free(colors);
        colors = NULL;
    }
    releaseNodeNumberCount();
}

inline void Colors::releaseAndAllocate() {
    release();
    colors = (int*) malloc(sizeof(int) * n);
    assert(colors != NULL);
}

Colors::Colors() :
        n(0), threadNum(0), colors(NULL), nodeNumberCount(NULL), label(NULL), unlabel(NULL) {
}

void Colors::dumpToObjectFile(const char * const fileName) const {
    FILE *fout = fopen(fileName, "wb");
    fwrite(&n, sizeof(n), 1, fout);
    fwrite(colors, sizeof(int), n, fout);
    fclose(fout);
}

const int * Colors::getColors() const {
    return colors;
}

const int * Colors::getlabel() const {
    return label;
}

const int * Colors::getunlabel() const {
    return unlabel;
}

const int * Colors::getNodeNumberCount() const {
    return nodeNumberCount;
}

void Colors::print() const {
    printf("n=%d threadNum=%d\n", n, threadNum);
    for (int i = 0; i < n; ++i)
        printf("%d ", colors[i]);
    printf("\n");
    for (int i = 0; i < threadNum; ++i)
        printf("%d ", nodeNumberCount[i]);
    printf("\n");
}

void Colors::printSummary() const {
    printf("n=%d threadNum=%d\n", n, threadNum);
    double avg = ((double) n) / threadNum;
    int min = n, max = 0;
    for (int i = 0; i < threadNum; ++i) {
        if (min > nodeNumberCount[i]) min = nodeNumberCount[i];
        if (max < nodeNumberCount[i]) max = nodeNumberCount[i];
    }
    printf("\tavg=%.6f min=%d max=%d\n", avg, min, max);
}

Colors::~Colors() {
    release();
}

void Colors::fromArray(int arrayLen, int* colorArray) {
  printf("fromArray n = %d\n", arrayLen);
  n = arrayLen;
  releaseAndAllocate();
  for (int i = 0; i < n; i++)
    colors[i] = colorArray[i];
  printf("fromArray finished\n");
}
void Colors::readFromObjectFile(AdjacentMatrix &adjMatrix, const char * const fileName) {
    n = adjMatrix.n;
    releaseAndAllocate();
    FILE *fin = fopen(fileName, "rb");
    int ret;
    ret = fread(&n, sizeof(n), 1, fin);
    assert(ret == 1);
    ret = fread(colors, sizeof(int), n, fin);
    assert(ret == n);
    fclose(fin);
}

int Colors::numOfCutEdge(AdjacentMatrix &adjMatrix) {
  int nCutEdge = 0;
  for (int i = 0; i < adjMatrix.n; i++) {
    for (int j = adjMatrix.g[i]; j < adjMatrix.g[i+1]; j++) {
      if (colors[adjMatrix.archArray[j].i] != colors[adjMatrix.archArray[j].j]) {
        nCutEdge++;
      }
    }
  }
  return nCutEdge;
}

void Colors::coordinateSplit(AdjacentMatrix &adjMatrix, Coordinate& co, int nsplit) {
  n = adjMatrix.n;
  assert(n == co.n);
  releaseAndAllocate();
  CoordinateSpliter spliter(adjMatrix, co, nsplit, colors);
  spliter.split();
}

int Colors::twoFoldSplit(AdjacentMatrix &adjMatrix, Coordinate& co, int bound) {
  n = adjMatrix.n;
  assert(n == co.n);
  releaseAndAllocate();
  TwoFoldSpliter spliter(adjMatrix, co, bound, colors);
  spliter.split();
  return spliter.currColor;
}

void Colors::color_from_file(AdjacentMatrix &adjMatrix, const char* filename, int threadNum) {
  printf("color_from_file starts\n");
  n = adjMatrix.n;
  releaseAndAllocate();
  FILE* file = fopen(filename, "r");
  for (int i = 0; i < n; i++) {
    fscanf(file, "%d\n", colors + i);
  }
  fclose(file);
  refine(threadNum);
}

int Colors::color_BFS_DeltaDepth(AdjacentMatrix &adjMatrix, int source, int deltaDepth) {
    n = adjMatrix.n;
    releaseAndAllocate();
    AdjacentMatrix::Arch *&archArray = adjMatrix.archArray;
    int *&g = adjMatrix.g;

    const int UNVISITED = 0xffffffff;
    memset(colors, 0xff, sizeof(int) * n);
    int *buffer = (int *) malloc(2 * n * sizeof(int));
    int *depth = buffer, *queue = depth + n, *&father = colors;
    int h = 0, t = 0;
    queue[h] = source;
    depth[source] = 0;
    father[source] = source;
    while (h <= t) {
        int &node_src = queue[h];
        for (int j = g[node_src]; j < g[node_src + 1]; ++j) {
            const int &node_dst = archArray[j].j;
            if (father[node_dst] == UNVISITED) {
                father[node_dst] = node_src;
                depth[node_dst] = depth[node_src] + 1;
                ++t;
                queue[t] = node_dst;
            }
        }
        ++h;
    }
    father[source] = UNVISITED;
    int current_thread_id = 0;
    for (int i = 0; i <= t; ++i) {
        int &j = queue[i];
        if ((depth[j] & (deltaDepth - 1)) == 0) {
            colors[j] = current_thread_id;
            ++current_thread_id;
        } else {
            colors[j] = colors[father[j]];
        }
    }
    free(buffer);
    return current_thread_id;
}

void Colors::rebalance(int threadNum, int nColor, int threadhold) {
  int nNodeInThread[threadNum];
  for (int i = 0; i < threadNum; i++) {
    nNodeInThread[i] = 0;
  }
  for (int i = 0; i < nColor; i++) {
    nNodeInThread[i % threadNum] += nodeNumberCount[i];
  }
  while (true) {
    int maxi = 0; int mini = 0;
    for (int i = 0; i < threadNum; i++) {
      if (nNodeInThread[i] > nNodeInThread[maxi]) {
        maxi = i;
      }
      if (nNodeInThread[i] < nNodeInThread[mini]) {
        mini = i;
      }
    }
    printf("biggest thread = %d, smallest = %d\n", nNodeInThread[maxi], nNodeInThread[mini]);
    if (nNodeInThread[mini] + threadhold > nNodeInThread[maxi])
      break;
    int maxInMaxI = maxi;
    for (int i = maxi; i < nColor; i += threadNum) {
      if (nodeNumberCount[i] > maxInMaxI)
        maxInMaxI = i;
    }
    int minInMinI = mini;
    for (int i = mini; i < nColor; i += threadNum) {
      if (nodeNumberCount[i] < minInMinI)
        minInMinI = i;
    }

    assert(maxInMaxI % threadNum == maxi);
    assert(minInMinI % threadNum == mini);
    printf("largest part in biggest = %d\n", nodeNumberCount[maxInMaxI]);
    printf("tiniest part in smallest = %d\n", nodeNumberCount[minInMinI]);
    if (nodeNumberCount[maxInMaxI] <= nodeNumberCount[minInMinI] ) {
      break;
    }
    nNodeInThread[maxi] -= nodeNumberCount[maxInMaxI] + nodeNumberCount[minInMinI];
    nNodeInThread[mini] -= nodeNumberCount[minInMinI] + nodeNumberCount[maxInMaxI];
    swapColor(maxInMaxI, minInMinI);
  }
}

void Colors::swapColor(int c1, int c2) {
  int tmp = nodeNumberCount[c1];
  nodeNumberCount[c1] = nodeNumberCount[c2];
  nodeNumberCount[c2] = tmp;
  for (int i = 0; i < n; i++) {
    if (colors[i] == c1) {
      colors[i] = c2;
    } else if (colors[i] == c2) {
      colors[i] = c1;
    }
  }
}

void Colors::refine(int threadNum) {
    this->threadNum = threadNum;
    releaseNodeNumberCount();
    nodeNumberCount = (int*) malloc(sizeof(int) * (threadNum + n + n));
    memset(nodeNumberCount, 0, sizeof(int) * threadNum);
    for (int i = 0; i < n; ++i) {
        colors[i] %= threadNum;
        ++nodeNumberCount[colors[i]];
    }
    label = nodeNumberCount + threadNum;
    unlabel = nodeNumberCount + threadNum + n;
    int *tmp = (int*) malloc(sizeof(int) * threadNum);
    for (int i = 0, cur = 0; i < threadNum; ++i) {
        tmp[i] = cur;
        cur += nodeNumberCount[i];
    }
    for (int i = 0; i < n; ++i) {
        label[i] = tmp[colors[i]];
        unlabel[label[i]] = i;
        ++tmp[colors[i]];
    }
    free(tmp);
}

template<typename T>
static void swap(T &a, T&b) {
    T c = a;
    a = b;
    b = c;
}

void buildColorLabel(CacheFriendlyAdjacentMatrix &graph, Colors &color) {
    int &n = graph.n, &threadNum = color.threadNum;
    int *colors = (int*) malloc(sizeof(int) * n);
    printf("colors[576739] = %d\n", color.colors[576739]);
    printf("colors[1520470] = %d\n", color.colors[1520470]);
    for (int i=0;i<n;++i)
        colors[graph.index[i]]=color.colors[i];
    printf("swapping color\n");
    swap(colors, color.colors);
    printf("colors[576739] = %d\n", color.colors[576739]);
    printf("colors[1520470] = %d\n", color.colors[1520470]);
    printf("idx[576739] = %d, color = %d\n", graph.index[576739], color.colors[graph.index[576739]]);
    printf("idx[1520470] = %d, color = %d\n", graph.index[1520470], color.colors[graph.index[1520470]]);
    free(colors);
    int *label = color.nodeNumberCount + threadNum;
    int *unlabel = color.nodeNumberCount + threadNum + n;
    int *tmp = (int*) malloc(sizeof(int) * threadNum);
    for (int i = 0, cur = 0; i < threadNum; ++i) {
        tmp[i] = cur;
        cur += color.nodeNumberCount[i];
    }
    for (int i = 0; i < n; ++i) {
        label[i] = tmp[color.colors[i]];
        unlabel[label[i]] = i;
        ++tmp[color.colors[i]];
    }
    free(tmp);
}

void unBuildColorLabel(CacheFriendlyAdjacentMatrix &graph, Colors &color) {
    int &n = graph.n, &threadNum = color.threadNum;
    int *colors = (int*) malloc(sizeof(int) * n);
    for (int i=0;i<n;++i)
        colors[graph.rindex[i]]=color.colors[i];
    swap(colors, color.colors);
    free(colors);
    int *label = color.nodeNumberCount + threadNum;
    int *unlabel = color.nodeNumberCount + threadNum + n;
    int *tmp = (int*) malloc(sizeof(int) * threadNum);
    for (int i = 0, cur = 0; i < threadNum; ++i) {
        tmp[i] = cur;
        cur += color.nodeNumberCount[i];
    }
    for (int i = 0; i < n; ++i) {
        label[i] = tmp[color.colors[i]];
        unlabel[label[i]] = i;
        ++tmp[color.colors[i]];
    }
    free(tmp);
}

void reorg(CacheFriendlyAdjacentMatrix &graph, Colors &color) {
    int &n = graph.n, &m = graph.m, *&index = graph.index, *&rindex = graph.rindex;
    int *&nodeNumberCount = color.nodeNumberCount;

    int *tmp = (int*) malloc(sizeof(int) * color.threadNum);
    assert(tmp!=NULL);
    tmp[0] = 0;
    for (int i = 1; i < color.threadNum; ++i)
        tmp[i] = tmp[i - 1] + nodeNumberCount[i - 1];
    for (int i = 0; i < n; ++i) {
        int &ci = color.colors[i];
        index[i] = tmp[ci];
        rindex[tmp[ci]] = i;
        ++tmp[ci];
    }
    free(tmp);

    char *buf = (char*) malloc(sizeof(AdjacentMatrix::Arch) * m + sizeof(int) * (n + 1));
    assert(buf!=NULL);
    AdjacentMatrix::Arch *archArray = (AdjacentMatrix::Arch*) buf;
    int *g = (int *) (buf + sizeof(AdjacentMatrix::Arch) * m);
    for (int i = 0, j = 0; i < n; ++i) {
        int &v0 = rindex[i];
        for (int k = graph.g[v0]; k < graph.g[v0 + 1]; ++k) {
            archArray[j].i = index[graph.archArray[k].i];
            archArray[j].j = index[graph.archArray[k].j];
            archArray[j].w = graph.archArray[k].w;
            ++j;
        }
    }
    swap(archArray, graph.archArray);
    swap(g, graph.g);

    free(archArray);
    graph.buildIndex();

    buildColorLabel(graph, color);
}

bool checkReorg(CacheFriendlyAdjacentMatrix &graph, Colors &color) {
  int currColor = 0;
  for (int i = 0; i < graph.n; i++) {
    if (color.colors[i] == currColor) {
      // do nothing
    } else if (color.colors[i] > currColor) {
      currColor = color.colors[i];
    } else {
      return false;
    }
  }
  return true;
}

void unreorg(CacheFriendlyAdjacentMatrix &graph, Colors &color) {
    int &n = graph.n, &m = graph.m, *&index = graph.index, *&rindex = graph.rindex;

    char *buf = (char*) malloc(sizeof(AdjacentMatrix::Arch) * m + sizeof(int) * (n + 1));
    assert(buf!=NULL);
    AdjacentMatrix::Arch *archArray = (AdjacentMatrix::Arch*) buf;
    int *g = (int *) (buf + sizeof(AdjacentMatrix::Arch) * m);
    for (int i = 0, j = 0; i < n; ++i) {
        int &v0 = index[i];
        for (int k = graph.g[v0]; k < graph.g[v0 + 1]; ++k) {
            archArray[j].i = rindex[graph.archArray[k].i];
            archArray[j].j = rindex[graph.archArray[k].j];
            archArray[j].w = graph.archArray[k].w;
            ++j;
        }
    }
    swap(archArray, graph.archArray);
    swap(g, graph.g);

    free(archArray);
    graph.buildIndex();

    unBuildColorLabel(graph, color);
    assert(checkReorg(graph, color));
}

void reorgResult(CacheFriendlyAdjacentMatrix &graph) {
    int &n = graph.n, *&rindex = graph.rindex;

    int *result = (int*) malloc(sizeof(int) * n);
    assert(result!=NULL);

    for (int i = 0; i < n; ++i)
        result[rindex[i]] = graph.result[i];

    swap(result, graph.result);
    free(result);
}
