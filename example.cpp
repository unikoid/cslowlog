#include "cslowlog.hpp"
#include <iostream>
#include <time.h>

int main(int argc, char** argv) {
    std::cout << "Start" << std::endl;
    cslowlog::Timer t(0, 100000000);
    sleep(1);
    t.run<void>([](struct timespec* elapsed) {
        std::cout << elapsed->tv_sec << "." << elapsed->tv_nsec << ": End" << std::endl;
    });
    t.outLog<std::cout>() << "Success!" << std::endl;
    t.out<std::cout>() << "Yeah" << std::endl;
    std::cout << "Fin" << std::endl;
}
