#pragma once
#include "cslowlog.h"
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