#include "Coordinate.h"
#include <stdio.h>
#include <assert.h>
#include <error.h>
#include <algorithm>
#include <string.h>

#include "RdtscTimer.h"

// #define dassert assert
#define dassert( ... )
Coordinate::Coordinate()
{
}

Coordinate::~Coordinate() {
  printf("deleting location\n");
  delete [] location;
}

int Coordinate::getX(int id) {
  return nodemap[id].x;
}

int Coordinate::getY(int id) {
  return nodemap[id].y;
}

void Coordinate::fromTextFile(const char * fileName) {
  FILE *fin = fopen(fileName, "r");
  assert(fin != NULL);
  char* ret;
  char buffer[0xff];
  do {
    ret = fgets(buffer, 0xff, fin);
    assert(ret!=NULL);
  } while (buffer[0] != 'p' && ret != NULL);
  int n;
  sscanf(buffer, "p aux sp co %d", &n);
  this->n = n;
  location = new Location[n];
  assert(location != NULL);
  for (int i = 0; i < n; i++) {
    do {
      ret = fgets(buffer, 0xff, fin);
      if (ret == NULL) {
        perror("fgets");
      }
      assert(ret!=NULL);
    } while (buffer[0] != 'v' && ret != NULL);
    int id, x, y;
    sscanf(buffer, "v %d %d %d", &id, &x, &y);
    id = id - 1;
    location[id].x = x;
    location[id].y = y;
    location[id].id = id;
    nodemap[id] = location[id];
  }
  fclose(fin);
}

void Coordinate::fromObjFile(const char * fileName)  {
  FILE *fin = fopen(fileName, "rb");
  int n; size_t ret;
  ret = fread(&n, sizeof(n), 1, fin);
  assert(ret == 1);
  this->n = n;
  location = new Location[n];
  assert(location != NULL);
  ret = fread(location, sizeof(Location), n, fin);
  assert(ret == (size_t)n);
  fclose(fin);
  for (int i = 0; i < n; i++) {
    int id = location[i].id;
    nodemap[id] = location[i];
  }
  return;
}

void Coordinate::dumpObjFile(const char* fileName)  {
  FILE *f = fopen(fileName, "wb");
  size_t ret;
  ret = fwrite(&n, sizeof(n), 1, f);
  assert(ret == 1);
  ret = fwrite(location, sizeof(Location), n, f);
  assert(ret == (size_t)n);
  fclose(f);
}

static bool cmpSortX(const Location& l1, const Location& l2) {
  return l1.x < l2.x;
}

static bool cmpSortY(const Location& l1, const Location& l2) {
  return l1.y < l2.y;
}

static void sortByX(Location* st, Location* ed) {
  std::sort(st, ed, cmpSortX);
}

static void sortByY(Location* st, Location* ed) {
  std::sort(st, ed, cmpSortY);
}

int CoordinateSpliter::getNewColor() {
  int result = nTmpColor;
  nTmpColor++;
  return result;
}

int CoordinateSpliter::costByX(int nSplit, Location* st, Location* ed) {
  assert(nSplit == 2);
  sortByX(st, ed);
  int result = 0;
  int mid = (ed - st) / 2;
  assert(nSplit == 2);
  int c = getNewColor();
  for (int i = 0; i < mid; i++)
    colors[st[i].id] = c;
  c = getNewColor();
  for (int i = mid; st + i != ed; i++) {
    colors[st[i].id] = c;
  }
  for (int i = 0; st+i != ed; i++) {
    int node = st[i].id;
    for (int j = adjMatrix.g[node]; j < adjMatrix.g[node+1]; j++) {
      int dst_node = adjMatrix.archArray[j].j;
      if (colors[node] != colors[dst_node])
        result++;
    }
  }
  return result;
}

int CoordinateSpliter::costByY(int nSplit, Location* st, Location* ed) {
  assert(nSplit == 2);
  sortByY(st, ed);
  int result = 0;
  int mid = (ed - st) / 2;
  assert(nSplit == 2);
  int c = getNewColor();
  for (int i = 0; i < mid; i++)
    colors[st[i].id] = c;
  c = getNewColor();
  for (int i = mid; st + i != ed; i++) {
    colors[st[i].id] = c;
  }
  for (int i = 0; st+i != ed; i++) {
    int node = st[i].id;
    for (int j = adjMatrix.g[node]; j < adjMatrix.g[node+1]; j++) {
      int dst_node = adjMatrix.archArray[j].j;
      if (colors[node] != colors[dst_node])
        result++;
    }
  }
  return result;
}

vector<int> CoordinateSpliter::splitHelper(int nSplit, Location* st, Location* ed) {
  printf("split st, ed = %ld, %ld. nsplit = %d\n", st - co.location, ed - co.location, nSplit );
  vector<int> result;
  if (nSplit == 1) {
    return result;
  }
  else {
    int costX = costByX(2, st, ed);
    int costY = costByY(2, st, ed);
    printf("costX = %d, costY = %d\n", costX, costY);
    if (costX < costY) {
      sortByX(st, ed);
    } else {
      sortByY(st, ed);
    }
    int mid = (ed-st) / 2;
    result.push_back(st-co.location + mid);
    vector<int> a = splitHelper(nSplit / 2, st, st+mid);
    vector<int> b = splitHelper(nSplit / 2, st+mid, ed);
    result.insert(result.end(), a.begin(), a.end());
    result.insert(result.end(), b.begin(), b.end());
    return result;
  }
}

int* CoordinateSpliter::split() {
  memset(colors, 0, sizeof(int)*co.n);
  vector<int> v = splitHelper(nsplit, co.location, co.location+co.n);
  std::sort(v.begin(), v.end());
  int last = 0;
  int count = 0;
  for (int tt = 0; tt < v.size(); tt++) {
    int idx = v[tt];
    printf("v[%d] = %d\n", tt, idx);
    for (int i = last; i < idx; i++)
      colors[co.location[i].id] = count;
    count++;
    last = idx;
  }
  for (int i = last; i < co.n; i++)
    colors[co.location[i].id] = count;
  for (int i = 0; i < 256; i++)
    printf("colors[%d] = %d\n", i, colors[i]);
  return colors;
}



void TwoFoldSpliter::linkSort(TFSItem* items[2], int st, int ed, int CURR) {
  int OTHER = CURR ^ 1;
  const int X = 0, Y = 1;
  if (ed - st <= 1) {
    return ;
  }
  int mid = (st + ed) / 2;
  TFSItem p = items[CURR][mid];
  items[CURR][mid] = items[CURR][ed-1];
  items[OTHER][items[CURR][mid].link].link = mid;
  int j = st;
  for (int i = st; i < ed - 1; i++) {
    if ((CURR == X && items[CURR][i].loc.x < p.loc.x) ||
        (CURR == Y && items[CURR][i].loc.y < p.loc.y)) {
      TFSItem tmp = items[CURR][j];
      items[CURR][j] = items[CURR][i];
      items[OTHER][items[CURR][j].link].link = j;
      items[CURR][i] = tmp;
      items[OTHER][items[CURR][i].link].link = i;
      j++;
    }
  }
  items[CURR][ed-1] = items[CURR][j];
  items[OTHER][items[CURR][ed-1].link].link = ed-1;
  items[CURR][j] = p;
  items[OTHER][items[CURR][j].link].link = j;
  linkSort(items, st, j, CURR);
  linkSort(items, j+1, ed, CURR);
}

static bool itemcmpX(const TwoFoldSpliter::TFSItem& x, const TwoFoldSpliter::TFSItem& y) {
  return x.loc.x < y.loc.x;
}

static bool itemcmpY(const TwoFoldSpliter::TFSItem& x, const TwoFoldSpliter::TFSItem& y) {
  return x.loc.y < y.loc.y;
}

void TwoFoldSpliter::split() {
  printf("start split\n");
  int n = adjMatrix.n;
  const int X = 0, Y = 1;
  TFSItem* items[2];
  items[X] = new TFSItem[n];
  items[Y] = new TFSItem[n];
  for (int i = 0; i < n; i++) {
    for (int j = X; j <= Y; j++) {
      items[j][i].loc = co.location[i];
      items[j][i].link = i;
    }
  }

  RdtscTimer timer; timer.Start();
  std::sort(items[X], items[X] + n, itemcmpX);
  std::sort(items[Y], items[Y] + n, itemcmpY);
  timer.Stop(); printf("timer sort = %lf\n", timer.GetSecond());
  timer.Start();
  for (int t = X; t <= Y; t++) {
    int OTHER = t ^ 1;
    for (int i = 0; i < n; i++) {
      itempos[items[t][i].loc.id] = i;
    }
    for (int i = 0; i < n; i++) {
      items[OTHER][i].link = itempos[items[OTHER][i].loc.id];
    }
  }
  timer.Stop(); printf("timer link = %lf\n", timer.GetSecond());
  for (int i = 0; i < n; i++) {
    if (i != n - 1)
      dassert(items[Y][i].loc.y <= items[Y][i+1].loc.y);
    dassert(items[X][items[Y][i].link].loc.id == items[Y][i].loc.id);
  }
  for (int i = 0; i < n; i++) {
    if (i != n - 1)
      dassert(items[X][i].loc.x <= items[X][i+1].loc.x);
    dassert(items[Y][items[X][i].link].loc.id == items[X][i].loc.id);
  }
  timer.Start();
  doSplit(items, 0, n, X);
  timer.Stop(); printf("doSplit time = %lf\n", timer.GetSecond());
  for (int i = 0; i < n; i++) {
    dassert(colors[i] >= 0 && colors[i] <= currColor);
  }
  delete [] items[X];
  delete [] items[Y];
}

class SplitCmp {
public:
  int mid, mask;
  SplitCmp(int mid, int mask): mid(mid), mask(mask) {};
  bool operator () (const TwoFoldSpliter::TFSItem& x, const TwoFoldSpliter::TFSItem& y) const {
    if (x.link < mid && y.link >= mid) {
      return true;
    } else if (x.link >= mid && y.link < mid) {
      return false;
    } else {
      if (mask == 0) return x.loc.x < y.loc.x;
      else return x.loc.y < y.loc.y;
    }
  }
};
void TwoFoldSpliter::doSplit(TFSItem* items[2], int st, int ed, int CURR) {
  const int X = 0, Y = 1;
  int NEXT = CURR ^ 1;

  if (ed - st < bound) {
    for (int i = st; i < ed; i++)
      colors[items[CURR][i].loc.id] = currColor;
    currColor += 1;
    return;
  }
  int mid = (st + ed) / 2;
  SplitCmp splitCmp(mid, NEXT);
  // std::sort(items[NEXT] + st, items[NEXT] + ed, splitCmp);
  // for (int i = st; i < ed; i++) {
  //   items[CURR][items[NEXT][i].link].link = i;
  // }

  const int FIRST = 0, LAST = 1;
  int count[2];count[FIRST] = st;count[LAST] = mid;
  for (int i = st; i < ed; i++) {
    int fall;
    if (items[NEXT][i].link < mid) {
      fall = FIRST;
    } else {
      fall = LAST;
    }
    pos[i] = count[fall];
    count[fall]++;
  }
  dassert(count[FIRST] == mid);
  dassert(count[LAST] == ed);

  rotate(items, st, ed, NEXT);
  for (int i = st; i < ed; i++) {
    items[CURR][items[NEXT][i].link].link = i;
  }
  for (int i = st; i < ed; i++) {
    dassert(items[Y][items[X][i].link].loc.id == items[X][i].loc.id);
    dassert(items[X][items[Y][i].link].loc.id == items[Y][i].loc.id);
  }
  doSplit(items, st, mid, NEXT);
  doSplit(items, mid, ed, NEXT);
}

void TwoFoldSpliter::rotate(TFSItem* items[2], int st, int ed, int CURR) {
  int OTHER = CURR ^ 1;
  for (int i = st; i < ed; i++) moved[i] = false;
  for (int i = st; i < ed; i++) {
    if (!moved[i]) {
      int j = i;
      TFSItem p = items[CURR][i];
      while (!moved[j]) {
        TFSItem tmp = items[CURR][pos[j]];
        items[CURR][pos[j]] = p;
        p = tmp;
        //items[OTHER][items[CURR][pos[j]].link].link = pos[j];
        moved[j] = true;
        j = pos[j];
      }
    }
  }
}
