#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>
#include <stdint.h>
#include <time.h>
#include <stdarg.h>
#include <math.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/wait.h>

#include "log.h"

uint64_t zero = 0;

uint64_t get_ms_time() {
    uint64_t            ms; // Milliseconds
    time_t          s;  // Seconds
    struct timespec spec;

    clock_gettime(CLOCK_REALTIME, &spec);

    s  = spec.tv_sec;
    ms = spec.tv_nsec / 1000000; // Convert nanoseconds to milliseconds
    if (ms > 999) {
        s++;
        ms = 0;
    }
    ms+=s*1000;
    return ms;
}

void Log(char* msg, ...)
{
        if (zero == 0) {
                zero = get_ms_time();
        }
        uint32_t t = get_ms_time() - zero;
        fprintf(stderr, "%09d ", t);

        va_list args;

        va_start(args, msg);
        vfprintf(stderr, msg, args);
        va_end(args);
}
