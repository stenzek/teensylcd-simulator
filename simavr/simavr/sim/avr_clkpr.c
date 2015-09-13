/*
	avr_clkpr.c

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


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "avr_clkpr.h"

static uint8_t avr_clkpr_ioreg_read(struct avr_t *avr, avr_io_addr_t addr, void *param)
{
    avr_clkpr_t *clkpr = (avr_clkpr_t *)param;
    return clkpr->current_prescale;
}

static void avr_clkpr_ioreg_write(struct avr_t *avr, avr_io_addr_t addr, uint8_t v, void *param)
{
    avr_clkpr_t *clkpr = (avr_clkpr_t *)param;

    AVR_LOG(avr, LOG_TRACE, "CLKPR(@0x%x) write 0x%x\n", addr, v);
    if (v & 0x80)
    {
        /* To set for clock prescaling factor it is necessary to access the CLKPR 
         * twice within 4 clock cycles.First you set the highest bit(CLKPCE) to 1
         * and all others to 0. Then you write the actual value for CLKPR.
         */
        clkpr->last_change_cycle = avr->cycle + 1 + 4;      /* IO Write operation = 1 cycle (not added yet), + 4 cycles wait */
        return;
    }

    if (avr->cycle > clkpr->last_change_cycle)
    {
        AVR_LOG(avr, LOG_WARNING, "CLKPR value written after timeout.\n");
        return;
    }

    unsigned int new_frequency = clkpr->base_frequency >> v;
    if (new_frequency == 0)
    {
        AVR_LOG(avr, LOG_ERROR, "CLKPR amount (%u) too large. keeping old frequency: %uhz\n", v, avr->frequency);
        return;
    }

    AVR_LOG(avr, LOG_TRACE, "CLKPR: base freq: %uhz, prescaler: %u, new frequency: %uhz\n", clkpr->base_frequency, v, new_frequency);
    clkpr->current_prescale = v;
    avr->frequency = new_frequency;
}

void avr_clkpr_init(avr_t *avr, avr_clkpr_t *clkpr)
{
    /* Register IO callbacks */
    avr_register_io_read(avr, clkpr->ioaddr, avr_clkpr_ioreg_read, (void *)clkpr);
    avr_register_io_write(avr, clkpr->ioaddr, avr_clkpr_ioreg_write, (void *)clkpr);

    /* Set to base frequency, irregardless of what user supplied (this is what it'll boot at) */
    avr->frequency = clkpr->base_frequency;
}
