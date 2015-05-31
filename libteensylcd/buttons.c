#include "teensylcd_buttons.h"
#include "sim_irq.h"
#include "avr_ioport.h"
#include <stdio.h>
#include <stdlib.h>

static unsigned char button_state[NUM_TEENSYLCD_BUTTONS] = { 0 };
static const char button_pins[NUM_TEENSYLCD_BUTTONS] = { 'B', 'B' };
static const char button_pin_indices[NUM_TEENSYLCD_BUTTONS] = { 0, 1 };
static avr_t *button_avr = NULL;
static avr_irq_t *button_irqs[NUM_TEENSYLCD_BUTTONS] = { NULL };
    
/* adapted from button.h */
static avr_cycle_count_t button_auto_release(avr_t *avr, avr_cycle_count_t when, void *param)
{
	struct teensylcd_t *teensy = (struct teensylcd_t *)param;
	printf("button_auto_release\n");
    teensylcd_buttons_set_state((enum TEENSYLCD_BUTTON)param, 0);
	return 0;
}

void teensylcd_buttons_init(avr_t *avr)
{
    static const char *button_names[NUM_TEENSYLCD_BUTTONS] = { "teensylcd.sw0", "teensylcd.sw1" };
    button_avr = avr;

    for (unsigned int i = 0; i < NUM_TEENSYLCD_BUTTONS; i++)
    {
        //b->irq = avr_alloc_irq(&avr->irq_pool, 0, IRQ_BUTTON_COUNT, &name);
        button_irqs[i] = avr_alloc_irq(&avr->irq_pool, 0, 1, &button_names[i]);
        //avr_connect_irq(button_irqs[i], avr_io_getirq(avr, AVR_IOCTL_IOPORT_GETIRQ(button_pins[i]), button_pin_indices[i]));
        avr_connect_irq(button_irqs[i], avr_io_getirq(avr, AVR_IOCTL_IOPORT_GETIRQ('B'), i));
    }
}

unsigned char teensylcd_buttons_get_state(enum TEENSYLCD_BUTTON button)
{
    if (button >= NUM_TEENSYLCD_BUTTONS)
        return 0;

    return button_state[button];
}

void teensylcd_buttons_set_state(enum TEENSYLCD_BUTTON button, unsigned char state)
{
    if (button >= NUM_TEENSYLCD_BUTTONS || button_state[button] == state)
        return;

    printf("set button %u %u\n", button, state);
    button_state[button] = state;
    avr_raise_irq(button_irqs[button], state);
}

void teensylcd_buttons_push_autorelease(enum TEENSYLCD_BUTTON button)
{
    if (button >= NUM_TEENSYLCD_BUTTONS)
        return;

    teensylcd_buttons_set_state(button, 1);
    avr_cycle_timer_cancel(button_avr, button_auto_release, (void *)button);
    avr_cycle_timer_register_usec(button_avr, 200000, button_auto_release, (void *)button);
}

#ifdef WITH_SDL

#include <SDL.h>

void teensylcd_buttons_update_from_keyboard()
{
    SDL_Event events[128];
    int nevents = SDL_PeepEvents(events, sizeof(events) / sizeof(events[0]), SDL_PEEKEVENT, SDL_FIRSTEVENT, SDL_LASTEVENT);
    if (nevents <= 0)
        return;

    for (int i = 0; i < nevents; i++) 
    {
        if (events[i].type == SDL_KEYDOWN || events[i].type == SDL_KEYUP)
        {
            if (events[i].key.keysym.scancode == SDL_SCANCODE_Z)
                teensylcd_buttons_set_state(TEENSYLCD_BUTTON_SW0, (events[i].type == SDL_KEYDOWN) ? 1 : 0);
            else if (events[i].key.keysym.scancode == SDL_SCANCODE_X)
                teensylcd_buttons_set_state(TEENSYLCD_BUTTON_SW1, (events[i].type == SDL_KEYDOWN) ? 1 : 0);
        }
    }
}

#endif


