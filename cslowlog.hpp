#pragma once
#include "cslowlog.h"
#include <functional>
#include <iostream>
#include <iomanip>
#include <cerrno>

#ifdef _POSIX_MONOTONIC_CLOCK
#define __CSLOWLOG_DEFAULT_CLK CLOCK_MONOTONIC
#else
#define __CSLOWLOG_DEFAULT_CLK CLOCK_REALTIME
#endif

namespace cslowlog {
    /**
     * Error class for clock-related errors
     */ 
    class ClockError : public std::runtime_error {
    public:
        /**
         * Last failed function return value
         */
        int retcode;
        /**
         * Last error number (errno)
         */
        int err;
        ClockError(int retcode) : std::runtime_error("CSlowlog clock error"), retcode(retcode), err(errno) {};
    };
    
    /**
     * Slowlog timer.
     */
    class Timer {
    protected:
        static std::ostream dummy; //// dummy "ostream" to be used instead of real one
        struct cslowlog_t timer;
        
        /**
         * Check if previously called function has returned error and throw ClockError in this case. 
         * @param return code of previously called function
         */
        int checkClkError(int retcode);

    public:
        /**
         * Create and start slowlog timer
         * @param number of seconds before timer expiration
         * @param number of nanoseconds before timer expiration
         * @param optional clock id (defaults to CLOCK_MONOTONIC if available, else CLOCK_REALTIME)
         */
        Timer(time_t sec, long nsec, clockid_t clk_id);
        
        /**
         * Check timer expiration
         */
        bool expired();
        
        /**
         * Get how much time has elapsed since timer start
         */
        struct timespec get_elapsed();
        
        /**
         * Run specified std::function if timer has expired
         * The only function argument is pointer to the time elapsed since timer start
         * This pointer is temporary and MUST not be used anywhere outside the function
         */
        template <typename T>
        void run(std::function <T (struct timespec*)>);
        
        /**
         * Return std::ostream instance specified via template parameter if timer has been expired,
         * otherwise return dummy std::ostream& that outputs nothing
         * Use it to log something to any ostream if timer expired
         */
        template<std::ostream&>
        std::ostream& out();
        
        /**
         * The same as out, but logs message about time elapsed to ostream before returning it
         * Use it to log something to any ostream if timer expired
         * @return chosen ostream (either from template parameter or "dummy")
         */
        template<std::ostream&>
        std::ostream& outLog();
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
    
    template <typename T>
    void Timer::run(std::function <T (struct timespec*)> f) {
        struct timespec elapsed;
        checkClkError(cslowlog_get_elapsed(&timer, &elapsed));
        if (cslowlog_tscmp(&elapsed, &timer.threshold) > 0) {
            f(&elapsed);
        }
    }
    
    template<std::ostream& s>
    std::ostream& Timer::out() {
        if (expired()) {
            return s;
        }
        return dummy;
    }

    template<std::ostream& s>
    std::ostream& Timer::outLog() {
        struct timespec elapsed;
        checkClkError(cslowlog_get_elapsed(&timer, &elapsed));
        if (cslowlog_tscmp(&elapsed, &timer.threshold) > 0) {
            s << "CSlowlog: " << elapsed.tv_sec << ".";
            s << std::setfill('0') << std::right << std::setw(9) << elapsed.tv_nsec;
            return s << " elapsed; ";
        }
        return dummy;
    }
};
