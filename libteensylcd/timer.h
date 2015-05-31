#ifndef __LIBTEENSYLCD_TIMER_H
#define __LIBTEENSYLCD_TIMER_H

#include <stdint.h>

/* returns current time in milliseconds */
uint64_t get_time_milliseconds();

/* returns current time in microseconds */
uint64_t get_time_microseconds();

#endif			//  __LIBTEENSYLCD_TIMER_H
