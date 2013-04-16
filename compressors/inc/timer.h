// simple Timer class, Jan 2010, thomas{AT}thomasfischer{DOT}biz
// tested under windows and linux
// license: do whatever you want to do with it ;)


#ifndef TIMER_H_
#define TIMER_H_

// boost timer is awful, measures cpu time on linux only...
// thus we have to hack together some cross platform timer :(

#if defined WIN32 || defined __CYGWIN__
#include <windows.h>
#else
#include <sys/time.h>
#endif

class Timer
{
protected:
#ifdef WIN32
    LARGE_INTEGER start;
#else
    struct timeval start;
#endif

public:
    Timer()
    {
        restart();
    }

    double elapsed()
    {
#if defined WIN32 || defined __CYGWIN__
        LARGE_INTEGER tick, ticksPerSecond;
        QueryPerformanceFrequency(&ticksPerSecond);
        QueryPerformanceCounter(&tick);
        return ((double)tick.QuadPart - (double)start.QuadPart) / (double)ticksPerSecond.QuadPart;
#else
        struct timeval now;
        gettimeofday(&now, NULL);
        return (now.tv_sec - start.tv_sec) + (now.tv_usec - start.tv_usec)/1000000.0;
#endif
    }

    void restart()
    {
#ifdef WIN32
        QueryPerformanceCounter(&start);
#else
        gettimeofday(&start, NULL);
#endif
    }
};


#endif /* TIMER_H_ */
