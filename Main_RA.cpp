#include <hbwmalloc.h>
#include <cstdlib>      // std::rand, std::srand
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cassert>
#include <cstring>
#include <omp.h>

#include "Utils/CycleTimer.h"
const int npad = 15;
int totalKBperCore = 4;
int nloop = 1024*8;
const int THREAD_NUM = 256;

#define ONE curr = memory[curr];
#define FOUR ONE ONE ONE ONE
#define EIGHT FOUR FOUR
#define ONE6 EIGHT EIGHT
#define THREE2 ONE6 ONE6
#define ONE28 THREE2 THREE2 THREE2 THREE2

#pragma intel optimization_level 3
int runLoop(int loop, int count, int * memory) {
  int result = 0;
  int curr = memory[loop];
  //#pragma noprefetch memory
  {
    while (count != 0) {
      // ONE28
      // count -= 128;
      ONE28
        count -= 128;
      result += curr;
    }
  }
  return result;
}

// void generateTarget(int* target, int ntarget) {
//   for (int j = ntarget - 1; j >= 0; j--) {
//     target[j] = j - 1;
//   }
//   target[0] = ntarget - 1;
// }

void generateTarget(int* target, int ntarget) {
  bool mask[ntarget];
  memset(mask, 0, sizeof(mask));
  int curr = 0;
  int count = ntarget;
  int pid = omp_get_thread_num();
  while (count != 1) {
    mask[curr] = true;
    count--;
    int nstep = ((random() + pid) % count) + 1;
    int next = -1;
    for (int i = 0; i < ntarget && nstep != 0; i++) {
      if (!mask[i]) nstep--;
      if (nstep == 0) {
        next = i;
      }
    }
    assert(next != -1);
    target[curr] = next;
    curr = next;
  }
  mask[curr] = true;
  target[curr] = 0;
  for (int i = 0; i < ntarget; i++) {
    assert(mask[i]);
  }
  memset(mask, 0, sizeof(mask));
  curr = 0;
  count = 0;
  while (!mask[curr]) {
    mask[curr] = true;
    curr = target[curr];
    count++;
  }
  assert(count == ntarget);
}

int main(int argc, const char* argv[]) {
  if (argc > 1)
    totalKBperCore = atoi(argv[1]);
  nloop = 512*1024/totalKBperCore;
  omp_set_num_threads(THREAD_NUM);
  srandom ( unsigned ( 0x27f ) );

  const int n = totalKBperCore * 1024 / sizeof(int);
  const int step = 1 + npad;
  printf("KB = %d, # of integer = %d, # of item = %d, size of item = %u, nthread = %d\n", totalKBperCore, n, n / step, sizeof(int) * (1 + npad), THREAD_NUM);

  int totalSum = 0;
  //int *ptarget = (int*)_mm_malloc(sizeof(int) * THREAD_NUM * (n / step), 256);
  int* ptarget = NULL;
  hbw_posix_memalign((void**)&ptarget, 4096, sizeof(int) * THREAD_NUM * (n / step));
  //int *pmemory = (int*)_mm_malloc(sizeof(int) * THREAD_NUM * n, 256);
  int* pmemory = NULL;
  hbw_posix_memalign((void**)&pmemory, 4096, sizeof(int) * THREAD_NUM * n);
  assert(pmemory != NULL);
  assert(ptarget != NULL);
#pragma omp parallel
  {
    int pid = omp_get_thread_num();
    int *memory = pmemory + pid * n;
    int *target = ptarget + pid * (n / step);
#pragma noinline
    generateTarget(target, n / step);
    for (int i = 0; i < n; i += step) {
      memory[i] = target[i/step] * (1 + npad);
      for (int j = 1; j <= npad; j++) {
        memory[i + j] = target[i/step] * (1 + npad) + j;
      }
    }
  }
  printf("Data generated\n");
  CycleTimer overallTimer;
  overallTimer.Start();
#pragma omp parallel
  {
    int pid = omp_get_thread_num();
    double second = 0;
    int sum = 0;
    //int volatile *memory = (int volatile *)(pmemory + pid * n);
    int *memory = (int *)(pmemory + pid * n);
    CycleTimer timer;
    timer.Start();
#pragma noinline
    sum += (int)runLoop(0, n / step * nloop, memory);
    timer.Stop();
    second += timer.GetSecond();
#pragma omp atomic
    totalSum += sum;
  }
  overallTimer.Stop();
  double totalMB = double(totalKBperCore) * nloop * THREAD_NUM / 1024;
  printf("1 reading: nloop = %d, sum = %d, time = %lf, totalMB = %lf, bandwidth = %f MB/s\n"
         "average delay = %lf ns\n",
         nloop, totalSum, overallTimer.GetSecond(), totalMB, totalMB / overallTimer.GetSecond(),
         overallTimer.GetSecond() * 1e9 / (nloop * (n / step)));
  return 0;
}
