#include <cstdlib>      // std::rand, std::srand
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cassert>
#include <cstring>
#include <omp.h>
#include <immintrin.h>

#include "Utils/CycleTimer.h"
const int npad = 15;
const int totalKBperCore = 1024;
const int nloop = 1024*4;
const int THREAD_NUM = 60;

#define ONE _mm_prefetch((const char*)(memory + curr), _MM_HINT_T1); \
  _mm_prefetch((const char*)(memory + curr1), _MM_HINT_T1);          \
  _mm_prefetch((const char*)(memory + curr2), _MM_HINT_T1);          \
  _mm_prefetch((const char*)(memory + curr3), _MM_HINT_T1);          \
  curr = memory[curr]; curr1 = memory[curr1];   curr2 = memory[curr2]; curr3 = memory[curr3];
#define FOUR ONE ONE ONE ONE
#define EIGHT FOUR FOUR
#define ONE6 EIGHT EIGHT
#define THREE2 ONE6 ONE6
#define ONE28 THREE2 THREE2 THREE2 THREE2

#pragma intel optimization_level 3
int runLoop(int loop, int count, int * memory) {
  int result = 0;
  int curr = memory[loop];
  int curr1 = memory[loop + 1 + npad];
  int curr2 = memory[loop + (1 + npad)*2];
  int curr3 = memory[loop + (1 + npad)*3];
  {
    while (count != 0) {
      ONE28
      count -= 128;
      // THREE2
      //   count -= 32*16;
      result = curr + curr1;
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

int main() {
  omp_set_num_threads(THREAD_NUM);
  srandom ( unsigned ( 0x27f ) );

  const int n = totalKBperCore * 1024 / sizeof(int);
  const int step = 1 + npad;
  printf("KB = %d, # of integer = %d, # of item = %d, size of item = %u, nthread = %d\n", totalKBperCore, n, n / step, sizeof(int) * (1 + npad), THREAD_NUM);

  int totalSum = 0;
  int *ptarget = (int*)_mm_malloc(sizeof(int) * THREAD_NUM * (n / step), 256);
  int *pmemory = (int*)_mm_malloc(sizeof(int) * THREAD_NUM * n, 256);
  assert(pmemory != NULL);
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
