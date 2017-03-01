#ifndef QUEUE_H_
#define QUEUE_H_

#include <stdlib.h>
#include <assert.h>

template<typename T>
class Queue {
public:
    T *data;
    int size, head, tail, capacity;

    Queue(int capacity) :
            size(0), head(0), tail(-1), capacity(capacity) {
        data = (T*) malloc(sizeof(T) * capacity);
        assert(data!=NULL);
    }

    ~Queue() {
        if (data != NULL) free(data);
    }

    bool empty() const {
        return size == 0;
    }

    T& front() {
        return data[head];
    }

    T& back() {
        return data[tail];
    }

    void pop_front() {
        --size;
        head = (head + 1) % capacity;
    }

    void push_back(const T &obj) {
        ++size;
        tail = (tail + 1) % capacity;
        data[tail] = obj;
    }

    int getSize() const {
        return size;
    }
};

#endif
