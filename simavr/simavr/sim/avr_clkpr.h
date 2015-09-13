/*

	avr_clkpr.h

	CLKPR (Clock Prescaler) handling

	Copyright 2015 Connor McLaughlin <connor.mclaughlin@connect.qut.edu.au>

 	This file is part of simavr.

	simavr is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	simavr is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with simavr.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __AVR_CLKPR_H__
#define __AVR_CLKPR_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "sim_avr.h"

typedef struct avr_clkpr_t {
    unsigned int base_frequency;
    avr_io_addr_t ioaddr;
    unsigned char current_prescale;
    avr_cycle_count_t last_change_cycle;
} avr_clkpr_t;

void avr_clkpr_init(avr_t *avr, avr_clkpr_t *clkpr);

#define AVR_CLKPR_DECLARE(_base_frequency, _addr)  \
    .clkpr = { \
        .base_frequency = (_base_frequency), \
        .ioaddr = (_addr), \
        .current_prescale = 0x00 \
    }

#ifdef __cplusplus
};
#endif

#endif /*__AVR_CLKPR_H__*/
