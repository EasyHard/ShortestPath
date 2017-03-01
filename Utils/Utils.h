#ifndef UTILS_H_
#define UTILS_H_

#include <stdlib.h>
#include <assert.h>
#include <sys/mman.h>
#include <omp.h>
#include <malloc.h>
#include <vector>
#include <immintrin.h>

template<typename T>
void swap(T &a, T &b) {
    T c = a;
    a = b;
    b = c;
}

namespace Utils {

struct Node {
  int w[4];
  int to[4];
  int tocolor[4];
  int dist;
  int heapIndex;
  bool frontier;
  // int prev; int next;
  // int shortestArc;
  // int bshortest;
} __attribute__((aligned(64)));;

  /* struct Node { */
  /*   int dist; */
  /*   int to[3]; */
  /*   int w[3]; */
  /*   int heapIndex; */
  /* } __attribute__((aligned(32))); */


    typedef struct {
      int key, value;
    } Item;

    /*
     *        0
     *       /  \
     *      1    2
     *     / \  / \
     *    3  4  5  6
     *
     *     x    floor[(x-1)/2]
     *   /   \       |
     * 2x+1 2x+2     x
     *
     */
    class Heap {

    private:
        Item *data;
        int *index;
        int len;

        inline int fa(int x) {
            return (x - 1) / 2;
        }

        void up(int x) {
            while (x > 0) {
                int tmp = (x - 1) / 2;
                if (data[tmp].value > data[x].value) {
                    swap(data[tmp], data[x]);
                    index[data[tmp].key] = tmp;
                    index[data[x].key] = x;
                    x = tmp;
                } else
                    break;
            }
        }

        void down(int x) {
            while (x * 2 + 1 < len) {
                int tmp = (x * 2 + 2 >= len || data[x * 2 + 1].value < data[x * 2 + 2].value) ? x * 2 + 1 : x * 2 + 2;
                if (data[tmp].value < data[x].value) {
                    swap(data[tmp], data[x]);
                    index[data[tmp].key] = tmp;
                    index[data[x].key] = x;
                    x = tmp;
                } else
                    break;
            }
        }

    public:
        Heap(int capacity) :
                len(0) {
            data = (Item*) malloc(sizeof(Item) * capacity);
            assert(data != NULL);
            index = (int*) malloc(sizeof(int) * capacity);
            assert(index !=NULL);
            memset(index, 0xff, sizeof(int) * capacity);
        }
      int size() {
        return len;
      }
        void add(int key, int value) {
            data[len].key = key;
            data[len].value = value;
            index[key] = len;
            ++len;
            up(len - 1);
        }

        void update(int key, int value) {
            data[index[key]].value = value;
            up(index[key]);
        }

        void addOrUpdate(int key, int value) {
            if (index[key] == -1)
              add(key, value);
            else if (value < data[index[key]].value) update(key, value);
        }

        void removeTop(Item *pData) {
            --len;
            if (pData != NULL) *pData = data[0];
            index[data[0].key] = -1;
            index[data[len].key] = 0;
            data[0] = data[len];
            // swap(data[0], data[len]);
            // index[data[0].key] = 0;
            // index[data[len].key] = -1;
            down(0);
        }

        const Item& top() {
            return data[0];
        }

        bool empty() const {
            return len == 0;
        }

        ~Heap() {
            free(data);
            free(index);
        }

        void print() {
            for (int i = 0; i < len; ++i)
                printf("(%d, %d) ", data[i].key, data[i].value);
            printf("\n");
        }
    };

    class Stack {
    protected:
        Item *data;
        int *index;
        int len;

    public:
        Stack(int capacity) :
                len(0) {
            data = (Item*) malloc(sizeof(Item) * capacity);
            assert(data != NULL);
            index = (int *) malloc(sizeof(int) * capacity);
            assert(index != NULL);
            memset(index, 0xff, sizeof(int) * capacity);
        }

        inline void addOrUpdate(int key, int value) {
            if (index[key] != -1) {
                if (data[index[key]].value > value) data[index[key]].value = value;
            } else {
                data[len].key = key;
                data[len].value = value;
                index[key] = len;
                ++len;
            }
        }

        bool empty() const {
            return len == 0;
        }

        inline void removeTop(Item *pTop) {
            assert(len != 0);
            --len;
            index[data[len].key] = -1;
            if (pTop != NULL) *pTop = data[len];
        }

        const Item& top() {
            assert(len > 0);
            return data[len - 1];
        }

        virtual ~Stack() {
            free(data);
            free(index);
        }
    };

    class OMPLockStack: public Stack {

        // lock here
        omp_lock_t lock;
    public:
        OMPLockStack(int capacity) :
                Stack(capacity) {
            omp_init_lock(&lock);
        }

        /**
         * Insert if Not Exists or Try to Update if Exists
         * Thread safe.
         */
        void addOrUpdateSafe(int key, int value) {
            omp_set_lock(&lock);
            addOrUpdate(key, value);
            omp_unset_lock(&lock);
        }

        void removeTopSafe(Item *pTop) {
            omp_set_lock(&lock);
            removeTop(pTop);
            omp_unset_lock(&lock);
        }

        const Item& top() {
            assert(len > 0);
            return data[len - 1];
        }

        ~OMPLockStack() {
            omp_destroy_lock(&lock);
        }
    };

    /*
     * Single Producer and Single Consumer
     */
    template<int capacity>
    class SmallConcurrentQueue {

      static const int MASK;
      Item* data;
      volatile int head;
      volatile int tail;

    public:
        SmallConcurrentQueue() :
                head(0), tail(0) {
          data = (Item*)_mm_malloc(sizeof(Item)*capacity, 256);
        }

        inline bool push(int key, int value) {
            if (((tail + 1) & MASK) == head) return false;
            data[tail].key = key;
            data[tail].value = value;
            tail = (tail + 1) & MASK;
            return true;
        }

        inline bool push(Item *item) {
            if (((tail + 1) & MASK) == head) return false;
            data[tail] = *item;
            tail = (tail + 1) & MASK;
            return true;
        }

        inline bool pop(Item *item) {
            if (head == tail) return false;
            if (item != NULL) *item = data[head];
            head = (head + 1) & MASK;
            return true;
        }

        inline bool pop(Item &item) {
            if (head == tail) return false;
            item = data[head];
            head = (head + 1) & MASK;
            return true;
        }

        inline bool empty() const {
            return head == tail;
        }

        ~SmallConcurrentQueue() {
          _mm_free(data);
        }
    };// __attribute__((aligned(64)));
    template<int capacity>
    const int SmallConcurrentQueue<capacity>::MASK = capacity - 1;

    template<int CAPACITY>
    class TwoDConcurrentQueue {
        int thread_nums;
    public:
        SmallConcurrentQueue<CAPACITY> *queues;
        TwoDConcurrentQueue(int thread_nums) :
                thread_nums(thread_nums) {
            queues = new SmallConcurrentQueue<CAPACITY> [256 * 256];
        }
        inline SmallConcurrentQueue<CAPACITY>* get(int row, int col) const {
            return queues + row * 256 + col;
        }
        ~TwoDConcurrentQueue() {
            delete[] queues;
        }
    };

    class Stacks {
    protected:
        struct StackInfo {
            Item *stack;
            int len;
        };

        Item *data;
        int *index;
        StackInfo *stackInfo;

        const int * const label;

    public:
        Stacks(int n, int threadNum, const int *nodeNumberCount, const int *label) :
                label(label) {
            size_t dataLen = sizeof(Item) * n, indexLen = sizeof(int) * n, stackInfoLen = sizeof(StackInfo) * threadNum;
            char *buffer = (char*) malloc(dataLen + indexLen + stackInfoLen);
            assert(buffer!=NULL);
            data = (Item*) (buffer);
            index = (int*) (buffer + dataLen);
            stackInfo = (StackInfo*) (buffer + dataLen + indexLen);

            memset(index, 0xff, sizeof(int) * n);

            assert(threadNum > 0);
            stackInfo[0].stack = data;
            stackInfo[0].len = 0;
            for (int i = 1; i < threadNum; ++i) {
                stackInfo[i].stack = stackInfo[i - 1].stack + nodeNumberCount[i - 1];
                stackInfo[i].len = 0;
            }
        }

        virtual ~Stacks() {
            free(data);
        }

        inline void addOrUpdate(int threadNo, int key, int value) {
            Item *&data = stackInfo[threadNo].stack;
            int &len = stackInfo[threadNo].len;
            const int &lkey = label[key];
            if (index[lkey] != -1) {
                if (data[index[lkey]].value > value) data[index[lkey]].value = value;
            } else {
                data[len].key = key;
                data[len].value = value;
                index[lkey] = len;
                ++len;
            }
        }

        inline bool empty(int threadNo) const {
            return stackInfo[threadNo].len == 0;
        }

        inline void removeTop(int threadNo, Item *pTop) {
            Item *&data = stackInfo[threadNo].stack;
            int &len = stackInfo[threadNo].len;
            assert(len > 0);
            --len;
            index[label[data[len].key]] = -1;
            if (pTop != NULL) *pTop = data[len];
        }

        inline const Item& top(int threadNo) const {
            Item *&data = stackInfo[threadNo].stack;
            int &len = stackInfo[threadNo].len;
            assert(len > 0);
            return data[len - 1];
        }
    };

    class OMPLockStacks: public Stacks {
        // lock here
        omp_lock_t *locks;
        int threadNum;
    public:
        OMPLockStacks(int n, int threadNum, const int *nodeNumberCount, const int *label) :
                Stacks(n, threadNum, nodeNumberCount, label), threadNum(threadNum) {
            locks = (omp_lock_t*) malloc(sizeof(omp_lock_t) * threadNum);
            assert(locks != NULL);
            for (int i = 0; i < threadNum; ++i)
                omp_init_lock(locks + i);
        }

        ~OMPLockStacks() {
            for (int i = 0; i < threadNum; ++i)
                omp_destroy_lock(locks + i);
            free(locks);
        }

        inline void addOrUpdateSafe(int threadNo, int key, int value) {
            omp_set_lock(locks + threadNo);
            addOrUpdate(threadNo, key, value);
            omp_unset_lock(locks + threadNo);
        }

        inline void removeTopSafe(int threadNo, Item *pTop) {
            omp_set_lock(locks + threadNo);
            removeTop(threadNo, pTop);
            omp_unset_lock(locks + threadNo);
        }
    };

    struct Record {
      int aur, key, value, index;
    };

  class LeanHeap {
  public:
    //std::vector<Record> record;
    Item *heap;
    Node* nodes;
    int cap;
    int size;
    LeanHeap(int initCap, Node* nodes)
      : cap(initCap), size(0), nodes(nodes) {
      heap = (Item*)_mm_malloc(sizeof(Item) * initCap, 64);
      memset(heap, 0, sizeof(Item)*initCap);
    }
    ~LeanHeap() {
      _mm_free(heap);
    }

    inline void up(int x) {
      while (x > 0) {
        int tmp = (x - 1) / 2;
        if (heap[tmp].value > heap[x].value) {
          //heapInfo.nswap++;
          swap(heap[tmp], heap[x]);
          //nodes[heap[tmp].key].heapIndex = tmp;
          nodes[heap[x].key].heapIndex = x;
          x = tmp;
        } else {
          break;
        }
      }
      nodes[heap[x].key].heapIndex = x;
    }

    // inline void up(int x) {
    //   int64_t* t = (int64_t*)heap;
    //   _mm_prefetch((const char*)(t + (x >> 1)), _MM_HINT_T0);
    //   int64_t xv = t[x];
    //   while (x > 0) {
    //     _mm_prefetch((const char*)(t + (x >> 2)), _MM_HINT_T0);
    //     int tmp = (x - 1) / 2;
    //     if (t[tmp] > xv) {
    //       t[x] = t[tmp];
    //       nodes[t[x] & 0xffffffffUL].heapIndex = x;
    //       //swap(heap[tmp], heap[x]);
    //       //nswap++;
    //       x = tmp;
    //     } else {
    //       break;
    //     }
    //   }
    //   t[x] = xv;
    //   nodes[t[x] & 0xffffffffUL].heapIndex = x;
    // }

    inline void down(int x) {
      //_mm_prefetch((const char*)(heap + (x << 1)), _MM_HINT_T0);
      int xval = heap[x].value, xkey = heap[x].key;
      while (x * 2 + 1 < size) {
        //_mm_prefetch((const char*)(heap + (x << 2)), _MM_HINT_T0);
        int tmp = (x * 2 + 2 >= size || heap[x * 2 + 1].value < heap[x * 2 + 2].value) ? x * 2 + 1 : x * 2 + 2;
        if (heap[tmp].value < xval) {
          heap[x] = heap[tmp];
          nodes[heap[x].key].heapIndex = x;
          //nswap++;
          x = tmp;
        } else {
          break;
        }
      }
      heap[x].value = xval; heap[x].key = xkey;
      nodes[heap[x].key].heapIndex = x;
    }

    // inline void down(int x) {
    //   while (x * 2 + 1 < size) {
    //     int tmp = (x * 2 + 2 >= size || heap[x * 2 + 1].value < heap[x * 2 + 2].value) ? x * 2 + 1 : x * 2 + 2;
    //     if (heap[tmp].value < heap[x].value) {
    //       //                  heapInfo.nswap++;
    //       swap(heap[tmp], heap[x]);
    //       // nodes[heap[tmp].key].heapIndex = tmp;
    //       nodes[heap[x].key].heapIndex = x;
    //       x = tmp;
    //     } else {
    //       break;
    //     }
    //   }
    //   nodes[heap[x].key].heapIndex = x;
    // }

    inline void add(int key, int value) {
      // Record r; r.aur = 0; r.key = key; r.value = value; record.push_back(r);
      if (size == cap) {
        Realloc(cap*2);
        cap = cap*2;
      }
      heap[size].key = key;
      heap[size].value = value;
      //nodes[key].heapIndex = size;
      size++;
      up(size - 1);
    }

    inline void update(int key, int value) {
      // Record r; r.aur = 1; r.key = key; r.value = value; r.index = nodes[key].heapIndex;
      // record.push_back(r);
      heap[nodes[key].heapIndex].value = value;
      up(nodes[key].heapIndex);
    }

    inline void addOrUpdate(int key, int value) {
      if (nodes[key].heapIndex == -1)
        add(key, value);
      else if (value < heap[nodes[key].heapIndex].value) update(key, value);
    }

    inline void removeTop(Item *pData) {
      // Record r; r.aur = 2; record.push_back(r);
      --size;
      swap(heap[0], heap[size]);
      nodes[heap[size].key].heapIndex = -1;
      if (size != 0) {
        down(0);
      }
    }

    inline void Realloc(int newsize) {
      Item* tmp = (Item*)_mm_malloc(newsize * sizeof(Item), 64);
      memcpy(tmp, heap, sizeof(Item) * cap);
      heap = tmp;
    }

  } __attribute__((aligned(64)));

  class LeanHeapR {
  public:
    Item *heap;
    int cap;
    int size;
    int nswap;
    Node* nodes;
  LeanHeapR(int initCap)
    : cap(initCap), size(0), nswap(0) {
      heap = (Item*)_mm_malloc(sizeof(Item) * initCap, 64);
    }
    ~LeanHeapR() {
      _mm_free(heap);
    }

    inline void up(int x) {
      int64_t* t = (int64_t*)heap;
      //_mm_prefetch((const char*)(t + (x >> 1)), _MM_HINT_T0);
      int64_t xv = t[x];
      while (x > 0) {
        //_mm_prefetch((const char*)(t + (x >> 2)), _MM_HINT_T0);
        int tmp = (x - 1) / 2;
        if (t[tmp] > xv) {
          t[x] = t[tmp];
          //swap(heap[tmp], heap[x]);
          //nswap++;
          x = tmp;
        } else {
          break;
        }
      }
      t[x] = xv;
    }

    inline void down(int x) {
      int64_t* t = (int64_t*)heap;
      // _mm_prefetch((const char*)(t + (x << 1)), _MM_HINT_T0);
      int64_t xv = t[x];
      while (x * 2 + 1 < size) {
        // _mm_prefetch((const char*)(t + (x << 2)), _MM_HINT_T0);
        int64_t t1 = t[x * 2 + 1], t2 = t[x * 2 + 2];
        int tmp = (x * 2 + 2 >= size || t1 < t2) ? x * 2 + 1 : x * 2 + 2;
        if (t[tmp] < xv) {
          t[x] = t[tmp];
          //swap(heap[tmp], heap[x]);
          //nswap++;
          x = tmp;
        } else {
          break;
        }
      }
      t[x] = xv;
    }

    // inline void down(int x) {
    //   //_mm_prefetch((const char*)(heap + (x << 1)), _MM_HINT_T0);
    //   int xval = heap[x].value, xkey = heap[x].key;
    //   while (x * 2 + 1 < size) {
    //     //_mm_prefetch((const char*)(heap + (x << 2)), _MM_HINT_T0);
    //     int tmp = (x * 2 + 2 >= size || heap[x * 2 + 1].value < heap[x * 2 + 2].value) ? x * 2 + 1 : x * 2 + 2;
    //     if (heap[tmp].value < xval) {
    //       heap[x] = heap[tmp];
    //       x = tmp;
    //     } else {
    //       break;
    //     }
    //   }
    //   heap[x].value = xval; heap[x].key = xkey;
    // }

    inline void add(int key, int value) {
      if (size == cap) {
        Realloc(cap*2);
      }
      heap[size].key = key;
      heap[size].value = value;
      size++;
      up(size - 1);
    }

    inline void update(int key, int value, int index) {
      heap[index].value = value;
      up(index);
    }

    inline void addOrUpdate(int key, int value) {
    }

    inline void removeTop() {
      --size;
      swap(heap[0], heap[size]);
      //nswap++;
      down(0);
    }

    inline void Realloc(int newsize) {
      Item* tmp = (Item*)_mm_malloc(newsize * sizeof(Item), 64);
      memcpy(tmp, heap, sizeof(Item) * cap);
      heap = tmp;
    }

  };

    class Heaps {
        struct HeapInfo {
            Item *heap;
            int len;
        }  __attribute__((aligned(64)));;
      int threadNum;
        int *index;
        HeapInfo *heapInfo;

    public:
      Heaps(int n, int threadNum, const int *nodeNumberCount) : threadNum(threadNum) {
          index = (int*)_mm_malloc(sizeof(int)*n, 1024*1024);
          assert(index != NULL);
          heapInfo = (HeapInfo*)_mm_malloc(sizeof(HeapInfo)*threadNum, 64);
          assert(index != NULL && heapInfo != NULL);
          memset(index, 0xff, sizeof(int) * n);
          for (int i = 0; i < threadNum; i++) {
            heapInfo[i].heap = (Item*)_mm_malloc(sizeof(Item)*nodeNumberCount[i], 64);
            heapInfo[i].len = 0;
          }
          printf("index = %p\n", index);
        }

        ~Heaps() {
          for (int i = 0; i < threadNum; i++) {
            _mm_free(heapInfo[i].heap);
          }
          printf("index = %p\n", index);
          _mm_free(index);
          _mm_free(heapInfo);
        }

    private:
        inline int fa(int x) {
            return (x - 1) / 2;
        }

        inline void up(HeapInfo &heapInfo, int x) {
            Item *&data = heapInfo.heap;
            while (x > 0) {
                int tmp = (x - 1) / 2;
                if (data[tmp].value > data[x].value) {
                  //heapInfo.nswap++;
                    swap(data[tmp], data[x]);
                    index[data[tmp].key] = tmp;
                    index[data[x].key] = x;
                    x = tmp;
                } else
                    break;
            }
        }

        inline void down(HeapInfo &heapInfo, int x) {
            Item *&data = heapInfo.heap;
            int &len = heapInfo.len;
            while (x * 2 + 1 < len) {
                int tmp = (x * 2 + 2 >= len || data[x * 2 + 1].value < data[x * 2 + 2].value) ? x * 2 + 1 : x * 2 + 2;
                if (data[tmp].value < data[x].value) {
                  //                  heapInfo.nswap++;
                    swap(data[tmp], data[x]);
                    index[data[tmp].key] = tmp;
                    index[data[x].key] = x;
                    x = tmp;
                } else
                    break;
            }
        }

    public:

        inline void add(int threadNo, int key, int value) {
            Item *&data = heapInfo[threadNo].heap;
            int &len = heapInfo[threadNo].len;
            data[len].key = key;
            data[len].value = value;
            index[key] = len;
            ++len;
            up(heapInfo[threadNo], len - 1);
        }

        inline void update(int threadNo, int key, int value) {
            Item *&data = heapInfo[threadNo].heap;

            data[index[key]].value = value;
            up(heapInfo[threadNo], index[key]);
        }

        inline void addOrUpdate(int threadNo, int key, int value) {
            Item *&data = heapInfo[threadNo].heap;
            if (index[key] == -1)
                add(threadNo, key, value);
            else if (value < data[index[key]].value) update(threadNo, key, value);
        }

        inline void removeTop(int threadNo, Item *pData) {
            Item *&data = heapInfo[threadNo].heap;
            int &len = heapInfo[threadNo].len;

            --len;
            //            heapInfo[threadNo].nswap++;
            swap(data[0], data[len]);
            if (pData != NULL) *pData = data[len];
            index[data[0].key] = 0;
            index[data[len].key] = -1;
            down(heapInfo[threadNo], 0);
        }

        inline const Item& top(int threadNo) const {
            return heapInfo[threadNo].heap[0];
        }

        inline bool empty(int threadNo) const {
            return heapInfo[threadNo].len == 0;
        }

      inline int size(int threadNo) const {
        return heapInfo[threadNo].len;
      }
        inline void print(int threadNo) const {
            Item *&data = heapInfo[threadNo].heap;
            for (int i = 0; i < heapInfo[threadNo].len; ++i)
                printf("(%d, %d) ", data[i].key, data[i].value);
            printf("\n");
        }

    };


    class LHeaps {
      struct HeapInfo {
        int* heap;
        int len;
      }  __attribute__((aligned(64)));;
      int threadNum;
      HeapInfo *heapInfo;
      Node* ng;
    public:
    LHeaps(int n, int threadNum, const int *nodeNumberCount, Node* ng) : threadNum(threadNum),
        ng(ng) {
          heapInfo = (HeapInfo*)_mm_malloc(sizeof(HeapInfo)*threadNum, 64);
          assert(heapInfo != NULL);
          for (int i = 0; i < threadNum; i++) {
            heapInfo[i].heap = (int*)_mm_malloc(sizeof(int)*nodeNumberCount[i], 64);
            heapInfo[i].len = 0;
          }
        }

      ~LHeaps() {
        for (int i = 0; i < threadNum; i++) {
          _mm_free(heapInfo[i].heap);
        }
        _mm_free(heapInfo);
      }

    private:
      inline void up(HeapInfo &heapInfo, int x) {
        int *&data = heapInfo.heap;
        int key = data[x];
        int value = ng[data[x]].dist;
        while (x > 0) {
          int tmp = (x - 1) / 2;
          if (ng[data[tmp]].dist > value) {
            data[x] = data[tmp];
            //swap(data[tmp], data[x]);
            //ng[data[tmp]].heapIndex = tmp;
            ng[data[x]].heapIndex = x;
            x = tmp;
          } else
            break;
        }
        data[x] = key;
        ng[key].heapIndex = x;
      }

      inline void down(HeapInfo &heapInfo, int x) {
        int *&data = heapInfo.heap;
        int &len = heapInfo.len;
        while (x * 2 + 1 < len) {
          int tmp = (x * 2 + 2 >= len || ng[data[x * 2 + 1]].dist < ng[data[x * 2 + 2]].dist) ?
            x * 2 + 1 : x * 2 + 2;
          if (ng[data[tmp]].dist < ng[data[x]].dist) {
            swap(data[tmp], data[x]);
            ng[data[tmp]].heapIndex = tmp;
            ng[data[x]].heapIndex = x;
            x = tmp;
          } else
            break;
        }
      }

    public:

      inline void add(int threadNo, int key, int value) {
        int *&data = heapInfo[threadNo].heap;
        int &len = heapInfo[threadNo].len;
        data[len] = key;
        ng[key].dist = value;
        ng[key].heapIndex = len;
        ++len;
        up(heapInfo[threadNo], len - 1);
      }

      inline void update(int threadNo, int key, int value) {
        int *&data = heapInfo[threadNo].heap;
        ng[key].dist = value;
        up(heapInfo[threadNo], ng[key].heapIndex);
      }

      inline void addOrUpdate(int threadNo, int key, int value) {
        int *&data = heapInfo[threadNo].heap;
        if (ng[key].heapIndex == -1)
          add(threadNo, key, value);
        else if (value < ng[key].dist) update(threadNo, key, value);
      }

      inline int removeTop(int threadNo) {
        int *&data = heapInfo[threadNo].heap;
        int result = data[0];
        int &len = heapInfo[threadNo].len;
        --len;
        swap(data[0], data[len]);
        ng[data[0]].heapIndex = 0;
        ng[data[len]].heapIndex = -1;
        down(heapInfo[threadNo], 0);
        return result;
      }

      inline int top(int threadNo) const {
        return heapInfo[threadNo].heap[0];
      }

      inline bool empty(int threadNo) const {
        return heapInfo[threadNo].len == 0;
      }

      inline int size(int threadNo) const {
        return heapInfo[threadNo].len;
      }
    };
}

#endif
