#ifndef RDTSCTIMER_H_
#define RDTSCTIMER_H_

#include <stdlib.h>
#include <sys/time.h>

static inline unsigned long rdtsc()
{
	unsigned int hi, lo;

	__asm volatile (
		"xorl %%eax, %%eax \n\t"
		"cpuid             \n\t"
		"rdtsc             \n\t"
		:"=a"(lo), "=d"(hi)
		:
		:"%ebx", "%ecx"
	);
	return ((unsigned long)hi << 32) | lo;
}

const static double freq = 1.05e9;
class RdtscTimer {
public:
    RdtscTimer() :
            start_(0) {
    }

    void Start() {
      start_ = rdtsc();
    }

    void Stop() {
      unsigned long stop_ = rdtsc();
      real_cycles = stop_ - start_;
    }

    double GetSecond() const {
      return real_cycles / freq;
    }

private:
    unsigned long real_cycles, start_;

};

#endif
