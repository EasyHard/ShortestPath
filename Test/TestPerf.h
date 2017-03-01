#ifndef TEST_PERF_H_
#define TEST_PERF_H_

#include "../Utils/CycleTimer.h"

template <size_t THREAD_NUM, size_t BUFFER_SIZE>
class TestLinkListWriteOnly {
public:
  TestLinkListWriteOnly() {};
  static void test() {
    omp_set_num_threads(THREAD_NUM);
    const int strip = 64;
    int * g = new int[THREAD_NUM*BUFFER_SIZE];
    int * idx = new int[THREAD_NUM*strip];
    CycleTimer timer;
    timer.Start();
#pragma omp parallel
    {
      int pid = omp_get_thread_num();
      int *buf = g + pid * BUFFER_SIZE;
      size_t i = 0;
      while (i != 1024*1024*16) {
        buf[i%BUFFER_SIZE] = i;
        i++;
        // if ( (i & 0xff) == 0xff) {
        //   buf[i%BUFFER_SIZE] = i+1;
        // }
      }
      for (size_t i = 0; i < BUFFER_SIZE; i++)
        g[BUFFER_SIZE*pid + i] = buf[i];
    }
    timer.Stop();
    printf("testLinkListWriteOnly\n\ttime=%.6lf\n\n", timer.GetSecond());
  }
};

template <size_t THREAD_NUM, size_t BUFFER_SIZE>
class TestLinkListRandomWriteOnly {
public:
  TestLinkListRandomWriteOnly() {};
  static void test() {
    omp_set_num_threads(THREAD_NUM);
    const int strip = 64;
    void* vg;
    long long * g;// = new long long[THREAD_NUM*BUFFER_SIZE];
    posix_memalign(&vg, 1024*32, THREAD_NUM*BUFFER_SIZE*sizeof(long long));
    g = (long long*) vg;
    printf("g = %p\n", g);
    int * idx = new int[THREAD_NUM*strip];
    CycleTimer timer;
    timer.Start();
#pragma omp parallel
    {
      int pid = omp_get_thread_num();
      long long *buf = g + pid * BUFFER_SIZE;
      size_t i = 0; size_t next = 41;
      while (i != 1024*1024*16) {
        for (int j = 0; j < 16; j++) {
          buf[(next + j )% BUFFER_SIZE] = i;
          i++;
        }
        next = next*13 + 89;
        //if (next > BUFFER_SIZE) next -= BUFFER_SIZE;
        //i++;
      }
      for (size_t i = 0; i < BUFFER_SIZE; i++)
        g[BUFFER_SIZE*pid + i] = buf[i];
    }
    timer.Stop();
    printf("testLinkListRandomWriteOnly\n\ttime=%.6lf\n\n", timer.GetSecond());
  }
};

template <size_t THREAD_NUM, size_t BUFFER_SIZE>
class TestLinkListRandomReadOnly {
public:
  TestLinkListRandomReadOnly() {};
  static void test() {
    omp_set_num_threads(THREAD_NUM);
    const int strip = 64;
    void* vg;
    int * g;// = new int[THREAD_NUM*BUFFER_SIZE];
    posix_memalign(&vg, 1024*32, THREAD_NUM*BUFFER_SIZE*sizeof(int));
    g = (int*) vg;
    printf("g = %p\n", g);
    int * idx = new int[THREAD_NUM*strip];
    CycleTimer timer;
    timer.Start();
#pragma omp parallel
    {
      int pid = omp_get_thread_num();
      int *buf = g + pid * BUFFER_SIZE;
      size_t i = 0; size_t next = 41;
      int sum = -1;
      while (i != 1024*1024*16) {
        sum += buf[next % BUFFER_SIZE];
        next = next*13 + 3;
        //if (next > BUFFER_SIZE) next -= BUFFER_SIZE;
        i++;
      }
      for (size_t i = 0; i < BUFFER_SIZE; i++)
        g[BUFFER_SIZE*pid + i] = sum + i;
    }
    timer.Stop();
    printf("testLinkListRandomReadOnly\n\ttime=%.6lf\n\n", timer.GetSecond());
  }
};


template <size_t THREAD_NUM, size_t BUFFER_SIZE>
class TestLinkListWriteWithIdx {
public:
  TestLinkListWriteWithIdx() {};
  static void test() {
    omp_set_num_threads(THREAD_NUM);
    int * g = new int[THREAD_NUM*BUFFER_SIZE];
    int * idx = new int[THREAD_NUM];
    CycleTimer timer;
    timer.Start();
#pragma omp parallel
    {
      int pid = omp_get_thread_num();
      int *buf = g + pid * BUFFER_SIZE;
      size_t i = 0;
      while (i != 1024*1024*16) {
        size_t j = 0;
        while (j != 256 && i != 1024*1024*16) {
          buf[i%BUFFER_SIZE] = i;
          i++;
          j++;
        }
        idx[pid] = i;
      }
      for (size_t i = 0; i < BUFFER_SIZE; i++)
        g[BUFFER_SIZE*pid + i] = buf[i] + idx[pid];
    }
    timer.Stop();
    printf("TestLinkListWriteWithIdx\n\ttime=%.6lf\n\n", timer.GetSecond());
  }
};


template <size_t THREAD_NUM, size_t BUFFER_SIZE>
class TestLinkListWriteWithIdxAndRemoteRead  {
public:
  TestLinkListWriteWithIdxAndRemoteRead () {};
  static void test() {
    omp_set_num_threads(THREAD_NUM);
    int * g = new int[THREAD_NUM*BUFFER_SIZE];
    //    int * idx = new int[THREAD_NUM];
    int * lastIdx = new int[THREAD_NUM*THREAD_NUM];
    CycleTimer timer;
    timer.Start();
#pragma omp parallel
    {
      int sum = 0;
      int pid = omp_get_thread_num();
      int *buf = g + pid * BUFFER_SIZE;
      size_t i = 0;
      for (int k = 0; k < THREAD_NUM; k++) lastIdx[k] = 0;
      while (i != 1024*1024*16) {
        size_t j = 0;
        while (j != 65536 && i != 1024*1024*16) {
          buf[i%BUFFER_SIZE] = i;
          i++;
          j++;
        }
        //        idx[pid] = i;
        for (int p = 0; p < THREAD_NUM; p++) {
          if (p != pid) {
            for (int k = lastIdx[p]; k < lastIdx[p] + 32; k++) {
              sum += g[p*BUFFER_SIZE+k%BUFFER_SIZE];
            }
            lastIdx[p] += 32;
          }
        }
      }
      for (size_t i = 0; i < BUFFER_SIZE; i++)
        g[BUFFER_SIZE*pid + i] = buf[i] + sum;
    }
    timer.Stop();
    printf("TestLinkListWriteWithIdxAndRemoteRead\n\ttime=%.6lf\n\n", timer.GetSecond());
  }
};


template <size_t THREAD_NUM, size_t BUFFER_SIZE>
class TestLinkListWriteWithIdxStrip {
public:
  TestLinkListWriteWithIdxStrip() {};
  static void test() {
    omp_set_num_threads(THREAD_NUM);
    const int strip = 64;
    int * g = new int[THREAD_NUM*BUFFER_SIZE];
    int * idx = new int[THREAD_NUM*strip];
    CycleTimer timer;
    timer.Start();
#pragma omp parallel
    {
      int pid = omp_get_thread_num();
      int *buf = g + pid * BUFFER_SIZE;
      size_t i = 0;
      while (i != 1024*1024*16) {
        buf[i%BUFFER_SIZE] = i;
        i++;
        if ( (i & 0xff) == 0xff) {
          idx[pid*strip] = i;
        }
      }
      for (size_t i = 0; i < BUFFER_SIZE; i++)
        g[BUFFER_SIZE*pid + i] = buf[i] + idx[pid];
    }
    timer.Stop();
    printf("TestLinkListWriteWithIdxStrip\n\ttime=%.6lf\n\n", timer.GetSecond());
  }
};

class TestPerfAdder {
    const int THREAD_NUM;
    const unsigned int LIMIT;
private:
    void testSpeedOfNonAtomicAndLockBasedAdd() {
        CycleTimer timer;
        unsigned int sum;
        timer.Start();
        sum = 0;
#pragma omp parallel
        {
            unsigned int localSum = 0;
            for (unsigned int i = 0; i < LIMIT; ++i) {
                ++localSum;
            }
#pragma omp atomic
            sum += localSum;
        }
        timer.Stop();
        assert(sum == LIMIT * THREAD_NUM);
        printf("testSpeedOfNonAtomicAndLockBasedAdd\n\ttime=%.6lf\n\n", timer.GetSecond());
    }

    void testSpeedOfAtomicBasedAdd() {
        CycleTimer timer;
        unsigned int sum;
        timer.Start();
        sum = 0;
#pragma omp parallel
        {
            for (unsigned int i = 0; i < LIMIT; ++i) {
#pragma omp atomic
                ++sum;
            }
        }
        timer.Stop();
        assert(sum == LIMIT * THREAD_NUM);
        printf("testSpeedOfAtomicBasedAdd\n\ttime=%.6lf\n\n", timer.GetSecond());
    }

    void testSpeedOfLockBasedAdd() {
        CycleTimer timer;
        unsigned int sum;
        omp_lock_t lock;
        omp_init_lock(&lock);
        timer.Start();
        sum = 0;
#pragma omp parallel
        {
            for (unsigned int i = 0; i < LIMIT; ++i) {
                omp_set_lock(&lock);
                ++sum;
                omp_unset_lock(&lock);
            }
        }
        timer.Stop();
        omp_destroy_lock(&lock);
        assert(sum == LIMIT * THREAD_NUM);
        printf("testSpeedOfLockBasedAdd\n\ttime=%.6lf\n\n", timer.GetSecond());
    }

public:
    TestPerfAdder(const int THREAD_NUM, const unsigned int LIMIT = 0x0000ffffu) :
            THREAD_NUM(THREAD_NUM), LIMIT(LIMIT) {
    }

    void testSpeedOfAtomicAndLockBasedAdd() {
        omp_set_num_threads(THREAD_NUM);
        for (int i = 0; i < 5; ++i)
            testSpeedOfNonAtomicAndLockBasedAdd();
        for (int i = 0; i < 5; ++i)
            testSpeedOfAtomicBasedAdd();
        for (int i = 0; i < 5; ++i)
            testSpeedOfLockBasedAdd();
    }
};

class TestCorePerf {
    const int n;
    int * const origBuf;
    int * buf;
private:
    static void qsort(int *buf, int h, int t) {
        int i = h, j = t, x = buf[(i + j) / 2];
        while (i <= j) {
            while (i <= j && buf[i] < x)
                ++i;
            while (i <= j && x < buf[j])
                --j;
            if (i <= j) {
                swap(buf[i], buf[j]);
                ++i;
                --j;
            }
        }
        if (i < t) qsort(buf, i, t);
        if (h < j) qsort(buf, h, j);
    }
public:
    TestCorePerf() :
            n(0x1000000), origBuf((int*) malloc(sizeof(int) * 2 * n)) {
        assert(origBuf != NULL);
        for (int i = 0; i < n; ++i) {
            origBuf[i] = rand();
        }
        buf = origBuf + n;
    }

    ~TestCorePerf() {
        free(origBuf);
    }

    void reset() {
        memcpy(buf, origBuf, sizeof(int) * n);
    }

    void testSpeed(const char * const NAME) {
        reset();
        CycleTimer timer;
        timer.Start();
        qsort(buf, 0, n - 1);
        timer.Stop();
        int x = 0;
        for (int i = 0; i < n; ++i)
            x ^= buf[i];
        printf("testSpeed NAME=%s x=%08x\n\ttime=%.6lf\n\n", NAME, x, timer.GetSecond());
    }
};

class TestPerf {
public:
    static void testPerfAdder() {
#ifndef IS_MIC
        TestPerfAdder testPerfAdder(8);
        testPerfAdder.testSpeedOfAtomicAndLockBasedAdd();
#else
        TestPerfAdder testPerfAdder(120);
        testPerfAdder.testSpeedOfAtomicAndLockBasedAdd();
#endif
    }

    static void testPerfCoreSpeed() {
        TestCorePerf testCorePerf;
#ifndef IS_MIC
        for (int i = 0; i < 5; ++i) {
            testCorePerf.testSpeed("CPU");
        }
#else
        for (int i = 0; i < 5; ++i) {
            testCorePerf.testSpeed("MIC");
        }
#endif
    }
};

#endif
