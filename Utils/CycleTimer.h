#ifndef CYCLETIMER_H_
#define CYCLETIMER_H_

#include <stdlib.h>
#include <sys/time.h>

class CycleTimer {
public:
    CycleTimer() :
            real_time_us_(0) {
    }

    void Start() {
        gettimeofday(&start_, NULL);
    }

    void Start(const struct timeval *pStart) {
        start_ = *pStart;
    }

    const struct timeval &getStart() const {
        return start_;
    }

    void Stop() {
        struct timeval stop;
        gettimeofday(&stop, NULL);
        real_time_us_ = 0;
        real_time_us_ += 1000000 * (stop.tv_sec - start_.tv_sec);
        real_time_us_ += (stop.tv_usec - start_.tv_usec);
    }

    long long getTimeFromStart() const {
        struct timeval stop;
        gettimeofday(&stop, NULL);
        return 1000000 * (stop.tv_sec - start_.tv_sec) + (stop.tv_usec - start_.tv_usec);
    }

    double GetSecond() const {
        return real_time_us_ * 1e-6;
    }

private:
    long long real_time_us_;
    struct timeval start_;

};

#endif /* CYCLETIMER_H_ */
