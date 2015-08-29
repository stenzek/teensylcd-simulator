#ifndef __LIBTEENSYLCD_TEENSYLCD_H
#define __LIBTEENSYLCD_TEENSYLCD_H

#include <stdint.h>
#include <stdbool.h>
#include "pcd8544.h"

/* default clock speed, ie 8Mhz */
#define TEENSYLCD_DEFAULT_FREQUENCY (8000000)

/* led enumeration */
enum TEENSYLCD_LED
{
	TEENSYLCD_LED0,
	TEENSYLCD_LED1,
	TEENSYLCD_LED2,
	NUM_TEENSYLCD_LEDS
};

/* led change callback */
typedef void(*teensylcd_led_change_callback)(void *teensy, enum TEENSYLCD_LED led, bool state);

/* button enumeration */
enum TEENSYLCD_BUTTON
{
    TEENSYLCD_BUTTON_SW0,
    TEENSYLCD_BUTTON_SW1,
    NUM_TEENSYLCD_BUTTONS
};

/* a simulated teensylcd */
struct teensylcd_t
{
	struct avr_t *avr;
	struct pcd8544_t lcd;
	bool led_states[NUM_TEENSYLCD_LEDS];
    teensylcd_led_change_callback led_change_callback;
	bool button_states[NUM_TEENSYLCD_BUTTONS];
	struct avr_irq_t *button_irqs[NUM_TEENSYLCD_BUTTONS];
	uint64_t next_cycles_sub;
};

/* initializer */
bool teensylcd_init(struct teensylcd_t *teensy, uint32_t frequency);

/* reset the processor, keeping firmware loaded */
void teensylcd_reset(struct teensylcd_t *teensy);

/* ELF firmware loader */
bool teensylcd_load_elf(struct teensylcd_t *teensy, const char *filename);

/* HEX firmware loader */
bool teensylcd_load_hex(struct teensylcd_t *teensy, const char *filename);

/* led state reader */
bool teensylcd_get_led_state(const struct teensylcd_t *teensy, enum TEENSYLCD_LED led);

/* led callback setter */
void teensylcd_set_led_callback(struct teensylcd_t *teensy, teensylcd_led_change_callback callback);

/* button state accessors/mutators */
bool teensylcd_get_button_state(const struct teensylcd_t *teensy, enum TEENSYLCD_BUTTON button);
void teensylcd_set_button_state(struct teensylcd_t *teensy, enum TEENSYLCD_BUTTON button, bool state);

/* button pusher, it will be automatically released, TODO move timer to parameter */
void teensylcd_push_button(struct teensylcd_t *teensy, enum TEENSYLCD_BUTTON button);

/* run a single clock cycle on the avr */
bool teensylcd_run_single(struct teensylcd_t *teensy);

/* run the avr for the specified simulated time period (in microseconds) */
bool teensylcd_run_time_microseconds(struct teensylcd_t *teensy, uint32_t run_time);

/* run the avr for the specified simulated time period (in milliseconds) */
bool teensylcd_run_time_milliseconds(struct teensylcd_t *teensy, uint32_t run_time);

/* run a single "frame" on the avr, ie 1/framerate seconds */
bool teensylcd_run_frame(struct teensylcd_t *teensy, uint32_t target_framerate);

/* run the avr until a refresh is performed, ie the lcd raster position reaches the current position (usually 0,0) again */
bool teensylcd_run_until_refresh(struct teensylcd_t *teensy);

/* cleanup */
void teensylcd_cleanup(struct teensylcd_t *teensy);

#endif		// __LIBTEENSYLCD_TEENSYLCD_H

