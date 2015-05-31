#ifndef __TEENSYLCD_BUTTONS_H
#define __TEENSYLCD_BUTTONS_H

#include "sim_avr.h"

enum TEENSYLCD_BUTTON
{
    TEENSYLCD_BUTTON_SW0,
    TEENSYLCD_BUTTON_SW1,
    NUM_TEENSYLCD_BUTTONS
};

void teensylcd_buttons_init(avr_t *avr);
unsigned char teensylcd_buttons_get_state(enum TEENSYLCD_BUTTON button);
void teensylcd_buttons_set_state(enum TEENSYLCD_BUTTON button, unsigned char state);
void teensylcd_buttons_push_autorelease(enum TEENSYLCD_BUTTON button);

#ifdef WITH_SDL

void teensylcd_buttons_update_from_keyboard();

#endif

#endif      // __TEENSYLCD_BUTTONS_H

