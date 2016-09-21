#include "cslowlog.h"
#include <iostream>
#include <time.h>

int main(int argc, char** argv) {
    std::cout << "Start" << std::endl;
    cslowlog::Timer t(0, 100000000);
    usleep(500000);
/*    t.run<void>([](struct timespec* elapsed) {
        std::cout << elapsed->tv_sec << "." << elapsed->tv_nsec << ": End" << std::endl;
        return;
    });
    */
    t << "Done" << "Success" << "Wow";
}