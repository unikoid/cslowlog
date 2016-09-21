#ifndef _CSLOWLOG_H_
#define _CSLOWLOG_H_

#if !defined(_XOPEN_SOURCE) && !defined(_POSIX_C_SOURCE)
#define _XOPEN_SOURCE 600
#endif

#include <unistd.h>
#if !defined(_POSIX_TIMERS) || (_POSIX_TIMERS <= 0)
#error Posix timers are not supported on this system
#endif

#include <stdio.h>
#include <time.h>
#include <stdarg.h>

#define NSEC 1000000000

struct cslowlog_t {
    struct timespec start;
    struct timespec threshold;
};

/* timespec-related functions */

/**
 * Calculate difference between timespecs x and y and store it into result
 * @param first timespec
 * @param second timespec
 * @param result pointer (first - second)
 */
void cslowlog_tsdiff(struct timespec* x, struct timespec* y, struct timespec* result) {
    struct timespec* a = x;
    struct timespec* b = y;
    // TODO: check with nsec > 1E+9
    if (x->tv_sec < y->tv_sec) {
        a = y;
        b = x;
    }
    result->tv_sec = x->tv_sec + x->tv_nsec / NSEC - y->tv_sec - y->tv_nsec / NSEC;
    result->tv_nsec = x->tv_nsec % NSEC - y->tv_nsec % NSEC;
    if (result->tv_nsec < 0) {
        result->tv_sec -= 1;
        result->tv_nsec *= -1;
    }
}

/**
 * Compare two timespecs
 * @param first timespec
 * @param second timespec
 * @return positive if x > y; negative if x < y; 0 if equal.
 */
int cslowlog_tscmp(struct timespec* x, struct timespec* y) {
    // time_t (timespec.tv_sec) may be unsigned, 
    return x->tv_sec > y->tv_sec ? 1 : x->tv_sec < y->tv_sec ? -1 : x->tv_nsec - y->tv_nsec;
}

/**
 * Initialize slowlog timer to start time measuring
 * @param timer to be initialized
 * @param seconds after which timer should expire
 * @param nanoseconds after which timer should expire (should be less then 1 billion)
 * @return 0 if successful, otherwise -1
 */
int cslowlog_start(struct cslowlog_t* timer, time_t sec, long nsec) {
    timer->threshold.tv_sec = sec;
    timer->threshold.tv_nsec = nsec;
    return clock_gettime(CLOCK_REALTIME, &timer->start);
}

/**
 * Get time elapsed since last slowlog timer start (last cslowlog_start on this timer)
 * @param slowlog timer
 * @param result
 * @return 0 if successful, otherwise -1
 */
int cslowlog_get_elapsed(struct cslowlog_t* timer, struct timespec* elapsed) {
    struct timespec current;
    if (clock_gettime(CLOCK_REALTIME, &current) < 0) {
        return -1;
    }
    cslowlog_tsdiff(&current, &timer->start, elapsed);
    return 0;
} 

/**
 * Check whether slowlog timer has been expired
 * @param slowlog timer
 * @return 1 if timer has been expired, 0 if not, -1 in case of an error
 */
int cslowlog_expired(struct cslowlog_t* timer) {
    struct timespec elapsed;
    if (cslowlog_get_elapsed(timer, &elapsed) < 0) {
        return -1;
    }
    return cslowlog_tscmp(&elapsed, &timer->threshold) > 0 ? 1 : 0;
}

/**
 * If slowlog timer has been expired - call provided function with time elapsed
 *  and provided data pointer
 * Otherwise do nothing
 * @param slowlog timer
 * @param pointer to callback function to be called
 * @param data to be passed into callback function
 * @return 0 if successful, otherwise -1
 */
int cslowlog_run(struct cslowlog_t* timer, void (*callback)(struct timespec*, void*), void* data) {
    struct timespec elapsed;
    if (cslowlog_get_elapsed(timer, &elapsed) < 0) {
        return -1;
    }
    if (cslowlog_tscmp(&elapsed, &timer->threshold) > 0) {
        callback(&elapsed, data);
    }
    return 0;
}

struct __cslowlog_printf_callback_data {
    va_list* args;
    const char* format;
    int result;
};

void __cslowlog_printf_callback(struct timespec* elapsed, void* d) {
    struct __cslowlog_printf_callback_data* data = (struct __cslowlog_printf_callback_data*)d;
    int result = printf("CSlowLog: %ld.%09ld elapsed; ", elapsed->tv_sec, elapsed->tv_nsec);
    if (result < 0) {
        data->result = result;
        return;
    }
    data->result = vprintf(data->format, *(data->args));
    if (data->result >= 0) {
        data->result += result;
    }
    return;
}

/**
 * Print a message if timer has been expired
 * @param cslowlog timer
 * @param format string (the same as for printf(3))
 * @return negative value in case of error, otherwise number of bytes written
 */
int cslowlog_printf(struct cslowlog_t* timer, const char* format, ...) {
    struct __cslowlog_printf_callback_data data;
    va_list va;
    va_start(va, format);
    data.args = &va;
    data.format = format;
    cslowlog_run(timer, __cslowlog_printf_callback, (void *)&data);
    va_end(va);
    return data.result;
}

#undef NSEC

#ifdef __cplusplus
#include <functional>
#include <iostream>

namespace cslowlog {
    class Timer {
    protected:
        static std::ostream dummy;
        struct cslowlog_t timer;
    public:
        Timer(time_t sec, long nsec);
        bool expired();
        struct timespec get_elapsed();
        template<typename T>
        T run(std::function <T (struct timespec*)>);
        template<std::ostream&>
        std::ostream& getOStream();
    };

    std::ostream Timer::dummy{nullptr};

    Timer::Timer(time_t sec, long nsec) {
        cslowlog_start(&timer, sec, nsec);
    }

    bool Timer::expired() {
        return cslowlog_expired(&timer) > 0;
    }
    
    struct timespec Timer::get_elapsed() {
        struct timespec elapsed;
        cslowlog_get_elapsed(&timer, &elapsed);
        return elapsed;
    }
    
    template<typename T>
    T Timer::run(std::function <T (struct timespec*)> f) {
        struct timespec elapsed;
        cslowlog_get_elapsed(&timer, &elapsed);
        return f(&elapsed);
    }
    
    template<std::ostream& s>
    std::ostream& Timer::getOStream() {
        if (expired()) {
            return s;
        }
        return dummy;
    }
}

#endif

#endif