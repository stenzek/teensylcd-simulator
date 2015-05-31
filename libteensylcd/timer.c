#include "timer.h"
#include <sys/time.h>
#include <stdlib.h>

uint64_t get_time_milliseconds()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint32_t)tv.tv_sec * 1000 + (uint32_t)tv.tv_usec / 1000;
}

uint64_t get_time_microseconds()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint64_t)tv.tv_sec * 1000000ULL + (uint64_t)tv.tv_usec;
}
