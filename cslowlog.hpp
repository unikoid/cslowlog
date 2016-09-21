#pragma once
#include "cslowlog.h"
#include <functional>
#include <iostream>

#ifdef _POSIX_MONOTONIC_CLOCK
#define __CSLOWLOG_DEFAULT_CLK CLOCK_MONOTONIC
#else
#define __CSLOWLOG_DEFAULT_CLK CLOCK_REALTIME
#endif

namespace cslowlog {
    class ClockError : public std::runtime_error {
    public:
        int code;
        ClockError(int code) : std::runtime_error("CSlowlog clock error"), code(code) {};
    };
    
    class Timer {
    protected:
        static std::ostream dummy;
        struct cslowlog_t timer;
        int checkClkError(int retcode);
    public:
        Timer(time_t sec, long nsec, clockid_t clk_id);
        bool expired();
        struct timespec get_elapsed();
        template<typename T>
        T run(std::function <T (struct timespec*)>);
        template<std::ostream&>
        std::ostream& getOStream();
    };

    std::ostream Timer::dummy{nullptr};

    Timer::Timer(time_t sec, long nsec, clockid_t clk_id=__CSLOWLOG_DEFAULT_CLK) {
        checkClkError(cslowlog_start(&timer, clk_id, sec, nsec));
    }

    int Timer::checkClkError(int retcode) {
        if (retcode < 0) {
            throw new ClockError(retcode);
        }
        return retcode;
    }
    
    bool Timer::expired() {
        return checkClkError(cslowlog_expired(&timer)) > 0;
    }
    
    struct timespec Timer::get_elapsed() {
        struct timespec elapsed;
        checkClkError(cslowlog_get_elapsed(&timer, &elapsed));
        return elapsed;
    }
    
    template<typename T>
    T Timer::run(std::function <T (struct timespec*)> f) {
        struct timespec elapsed;
        checkClkError(cslowlog_get_elapsed(&timer, &elapsed));
        return f(&elapsed);
    }
    
    template<std::ostream& s>
    std::ostream& Timer::getOStream() {
        if (expired()) {
            return s;
        }
        return dummy;
    }
};
