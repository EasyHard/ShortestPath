#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <malloc.h>
#include <climits>
#include "Graph.h"

inline static int archCmp(const void *e1, const void *e2) {
    const AdjacentMatrix::Arch * p1 = (const AdjacentMatrix::Arch *) e1, *p2 = (const AdjacentMatrix::Arch*) e2;
    return (p1->i < p2->i || (p1->i == p2->i && (p1->j < p2->j || (p1->j == p2->j && p1->w <= p2->w)))) ? -1 : 1;
}

void BaseGraph::allocateBuf() {
  if (n > 15000000) {
    result = (int*) malloc(sizeof(int) * n);
  } else {
    result = (int*) malloc(sizeof(int) * 15000000);
  }
    assert(result!=NULL);
}

const int BaseGraph::RESULT_MAX_VALUE = 0x7fffffff;

BaseGraph::BaseGraph() :
        n(0), m(0), result(NULL) {
}

BaseGraph::BaseGraph(int n, int m) :
        n(n), m(m) {
    allocateBuf();
}

BaseGraph::~BaseGraph() {
    if (result != NULL) free(result);
}

int BaseGraph::checkSum() {
    assert(result!=NULL);
    int ret = 0;
    for (int i = 0; i < n; ++i)
        ret ^= result[i] + i;
    return ret;
}

int BaseGraph::checkSumNoI() {
    assert(result!=NULL);
    int ret = 0;
    for (int i = 0; i < n; ++i)
        ret ^= result[i];
    return ret;
}

void BaseGraph::resetResult() {
    for (int i = 0; i < n; ++i)
        result[i] = RESULT_MAX_VALUE;
}


void BaseGraph::showResult() {
    assert(result!=NULL);
    printf("n=%d m=%d\n", n, m);
    for (int i = 0; i < n; ++i)
        printf("%d ", result[i]);
    printf("\n");
}

void AdjacentMatrix::reallocResultPad() {
  printf("reallocPad\n");
  if (result != NULL)
    free(result);
  if (nafterpad > 15000000) {
    result = (int*)malloc(sizeof(int) * nafterpad);
  } else {
    result = (int*)malloc(sizeof(int) * 15000000);
  }
  assert(result != NULL);
}
void AdjacentMatrix::resetResultPad() {
  for (int i = 0; i < nafterpad; ++i)
    result[i] = RESULT_MAX_VALUE;
}

int AdjacentMatrix::checkSumPad() {
  int ret = 0;
  for (int i = 0; i < n; i++) {
    ret ^= result[newNodeMap[i]] + i;
  }
  return ret;
}

int AdjacentMatrix::checkSumPadNoI() {
  int ret = 0;
  for (int i = 0; i < n; i++) {
    ret ^= result[newNodeMap[i]];
  }
  return ret;
}

void AdjacentMatrix::removeCutEdges(const int* colors) {
  for (int i = 0; i < m; i++) {
    if (colors[archArray[i].i] != colors[archArray[i].j]) {
      archArray[i].j = archArray[i].i;
    }
  }
}


int AdjacentMatrix::padsoft(int npad) {
  int ncount = 0;
  int *nnewnode = new int[n];
  newNodeMap = new int[n];
  for (int i = 0; i < n; i++) {
    int nedge = g[i+1] - g[i];
    if (nedge <= npad) nnewnode[i] = 1;
    else nnewnode[i] = (nedge - npad + npad - 3)/(npad - 2) + 1;
    ncount += nnewnode[i];
  }
  delete [] nnewnode;
  return ncount;
}
void AdjacentMatrix::pad(int npad, int* colors) {
  int ncount = 0;
  if (nnewnode != NULL) {
    delete [] nnewnode;
  }
  nnewnode = new int[n];
  if (newNodeMap != NULL) {
    delete [] newNodeMap;
  }
  newNodeMap = new int[n];
  for (int i = 0; i < n; i++) {
    int nedge = g[i+1] - g[i];
    newNodeMap[i] = ncount;
    if (nedge <= npad) nnewnode[i] = 1;
    else nnewnode[i] = (nedge - npad + npad - 3)/(npad - 2) + 1;
    ncount += nnewnode[i];
  }
  printf("graph n = %d, npad = %d, n_after_pad = %d\n",
         n, npad, ncount);
  if (tos != NULL) {
    delete [] tos;
  }
  tos = new int[ncount*npad];
  for (int i = 0; i < ncount*npad; i++) tos[i] = INT_MIN;
  if (ws != NULL) {
    delete [] ws;
  }
  ws = new int[ncount*npad];
  if (padcolor != NULL) {
    delete [] padcolor;
  }
  padcolor = new int[ncount];
  if (padold != NULL) {
    delete [] padold;
  }
  padold = new int[ncount];
  for (int i = 0; i < ncount; i++) padold[i] = -1;
  for (int i = 0; i < n; i++) {
    int curr = newNodeMap[i];
    padcolor[curr] = colors[i];
    padold[curr] = i;
    int ne = 0;
    for (int j = g[i]; j < g[i+1];) {
      int to = newNodeMap[archArray[j].j];
      int w = archArray[j].w;
      if (ne != npad - 1) {
        tos[curr*npad + ne] = to;
        ws[curr*npad + ne] = w;
        ne++; j++;
      } else if (j == g[i+1] - 1) {
        tos[curr*npad + ne] = to;
        ws[curr*npad + ne] = w;
        ne++;
        break;
      } else {
        tos[curr*npad + ne] = curr + 1;
        ws[curr*npad + ne] = 0;
        ne = 0; curr++;
        padcolor[curr] = colors[i];
        padold[curr] = i;
        tos[curr*npad + ne] = curr - 1;
        ws[curr*npad + ne] = 0;
        ne++;
      }
    }
    while (ne != npad) {
      tos[curr*npad + ne] = curr;
      ws[curr*npad + ne] = 1;
      ne++;
    }
  }
  printf("pad finished\n");
  nafterpad = ncount;

  // // Check padding graph
  // for (int i = 0; i < ncount; i++) {
  //   assert(padold[i] != -1);
  // }

  // for (int i = 0; i < ncount*npad; i++) assert(tos[i] != INT_MIN);

  // for (int i = 0; i < n; i++) {
  //   for (int j = g[i]; j < g[i+1]; j++) {
  //     bool found = false;
  //     int padi = newNodeMap[i];
  //     for (int m = padi; m < padi + nnewnode[i]; m++) {
  //       for (int e = 0; e < 4; e++) {
  //         if (tos[m*4+e] == newNodeMap[archArray[j].j] &&
  //             ws[m*4+e] == archArray[j].w)
  //           found = true;
  //       }
  //     }
  //     if (!found) {
  //       printf("i = %d, padi = %d, nnewnode = %d\n", i, padi, nnewnode[i]);
  //       for (int j = g[i]; j < g[i+1]; j++) {
  //         const int &dst = archArray[j].j, &weight = archArray[j].w;
  //         printf("[dst = %d, w = %d] ", dst, weight);
  //       }
  //       printf("\n");
  //       for (int m = newNodeMap[i]; m < newNodeMap[i] + nnewnode[i]; m++) {
  //         printf("node %d:", m);
  //         for (int e = 0; e < 4; e++) {
  //           printf(" (to %d, w %d)", tos[m*4+e], ws[m*4+e]);
  //         }
  //         printf("\n");
  //       }
  //     }
  //     assert(found);
  //   }
  // }
}


void AdjacentMatrix::buildSimple() {
  tos = (int*)memalign(256, sizeof(int)*m);
  ws = (int*)memalign(256, sizeof(int)*m);
  toarch = (ToArch*)memalign(256, sizeof(ToArch)*m);
  for (int i = 0; i < m; i++) {
    tos[i] = archArray[i].j;
    ws[i] = archArray[i].w;
    toarch[i].to = archArray[i].j;
    toarch[i].w = archArray[i].w;
  }

  data = (int*)memalign(256, sizeof(int)*(8*n));
  int curr = 0;
  for (int i = 0; i < n; i++) {
    data[curr] = RESULT_MAX_VALUE;
    curr++;
    data[curr] = -1;
    curr++;
    for (int j = g[i]; j < g[i] + 3; j++) {
      if (j < g[i+1]) {
        data[curr] = archArray[j].j;
        data[curr+1] = archArray[j].w;
      } else {
        data[curr] = i;
        data[curr+1] = 1;
      }
      curr += 2;
    }
  }
}

void AdjacentMatrix::buildIndex() {
    memset(g, 0xff, sizeof(int) * (n + 1));
    for (int i = 0; i < m; ++i)
        if (g[archArray[i].i] == -1) g[archArray[i].i] = i;
    g[n] = m;
    for (int i = n - 1; i >= 0; --i)
        if (g[i] == -1) g[i] = g[i + 1];
}

void AdjacentMatrix::readFromTextFile(const char * const fileName) {
    FILE *fin = fopen(fileName, "r");

    char* ret;
    char buffer[0xff];
    do {
        ret = fgets(buffer, 0xff, fin);
        assert(ret!=NULL);
    } while (buffer[0] != 'p' && ret != NULL);
    sscanf(buffer, "p sp %d %d", &n, &m);
    BaseGraph::allocateBuf();

    allocateBuf();

    for (int i = 0; i < m; ++i) {
        do {
            ret = fgets(buffer, 0xff, fin);
            assert(ret!=NULL);
        } while (buffer[0] != 'a' && ret != NULL);
        sscanf(buffer, "a %d %d %d", &archArray[i].i, &archArray[i].j, &archArray[i].w);
        --archArray[i].i;
        --archArray[i].j;
    }

    fclose(fin);

    qsort(archArray, m, sizeof(Arch), archCmp);
    buildIndex();
}

void AdjacentMatrix::readFromObjectFile(const char * const fileName) {
    size_t ret;
    FILE *fin = fopen(fileName, "rb");
    ret = fread(&n, sizeof(n), 1, fin);
    assert(ret == 1);
    BaseGraph::allocateBuf();
    ret = fread(&m, sizeof(m), 1, fin);
    assert(ret == 1);
    assert(m >= 0);

    allocateBuf();
    ret = fread(archArray, sizeof(Arch), m, fin);
    assert(ret == (size_t )m);
    ret = fread(g, sizeof(int), n + 1, fin);
    assert(ret == (size_t )(n + 1));
    fclose(fin);
}

void AdjacentMatrix::allocateBuf() {
    char *buf = (char*) malloc(sizeof(Arch) * m + sizeof(int) * (n + 1));
    assert(buf!=NULL);
    archArray = (Arch*) buf;
    g = (int *) (buf + sizeof(Arch) * m);
}

void AdjacentMatrix::freeBuf() {
    if (archArray != NULL) {
        free(archArray);
        archArray = NULL;
        g = NULL;
    }
}

AdjacentMatrix::AdjacentMatrix(const char * const fileName):
  tos(NULL), ws(NULL), padcolor(NULL), padold(NULL), newNodeMap(NULL), nnewnode(NULL)
{
    int len = strlen(fileName);
    if (strcmp(fileName + len - 6, "obj.gr") == 0) {
        readFromObjectFile(fileName);
    } else {
        readFromTextFile(fileName);
    }
}

AdjacentMatrix::AdjacentMatrix(int n, int m, Arch *archArray) :
  BaseGraph(n, m),   tos(NULL), ws(NULL), padcolor(NULL), padold(NULL), newNodeMap(NULL), nnewnode(NULL) {
    allocateBuf();
    memcpy(this->archArray, archArray, sizeof(Arch) * m);
    qsort(this->archArray, m, sizeof(Arch), archCmp);
    buildIndex();
}

AdjacentMatrix::~AdjacentMatrix() {
    freeBuf();
}

void AdjacentMatrix::dumpToObjectFile(const char * const fileName) {
    FILE *fout = fopen(fileName, "wb");
    fwrite(&n, sizeof(n), 1, fout);
    fwrite(&m, sizeof(m), 1, fout);
    fwrite(archArray, sizeof(Arch), m, fout);
    fwrite(g, sizeof(int), n + 1, fout);
    fclose(fout);
}

AdjacentMatrix::Arch *AdjacentMatrix::getArchArray() {
    return &archArray[0];
}

void AdjacentMatrix::print() {
    printf("n=%d m=%d\n", n, m);
    for (int i = 0; i < n; ++i)
        for (int j = g[i]; j < g[i + 1]; ++j)
            printf("%d %d %d\n", i, archArray[j].j, archArray[j].w);
}

void CacheFriendlyAdjacentMatrix::allocateBuf() {
    char *buf = (char*) malloc(sizeof(int) * n * 2);
    assert(buf!=NULL);
    index = (int*) buf;
    rindex = (int*) (buf + sizeof(int) * n);
}

void CacheFriendlyAdjacentMatrix::freeBuf() {
    if (index != NULL) {
        free(index);
        index = NULL;
        rindex = NULL;
    }
}

CacheFriendlyAdjacentMatrix::CacheFriendlyAdjacentMatrix(const char * const fileName) :
        AdjacentMatrix(fileName), index(NULL), rindex(NULL) {
    allocateBuf();
}

CacheFriendlyAdjacentMatrix::CacheFriendlyAdjacentMatrix(int n, int m, Arch *archArray) :
        AdjacentMatrix(n, m, archArray), index(NULL), rindex(NULL) {
    allocateBuf();
}

CacheFriendlyAdjacentMatrix::~CacheFriendlyAdjacentMatrix() {
    freeBuf();
}
