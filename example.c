#include "cslowlog.h"

#include <stdio.h>
#include <time.h>
#include <unistd.h>

int main(int argc, char** argv) {
    struct cslowlog_t timer;
    printf("Start\n");
    cslowlog_start(&timer, 0, 100000000);
    usleep(500000);
    cslowlog_printf(&timer, "Dead %d\n", 2);
    return 0;
}
