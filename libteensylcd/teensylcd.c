#include "teensylcd.h"
#include "sim_avr.h"
#include "sim_irq.h"
#include "sim_elf.h"
#include "sim_hex.h"
#include "sim_time.h"
#include "avr_ioport.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

static const char *button_names[NUM_TEENSYLCD_BUTTONS] = { "teensylcd.sw0", "teensylcd.sw1" };

/* led change hooks */

static void led0_changed_hook(struct avr_irq_t *irq, uint32_t value, void *param)
{
    struct teensylcd_t *teensy = (struct teensylcd_t *)param;
    teensy->led_states[TEENSYLCD_LED0] = (value != 0);
    printf("LED0 turned %s\n", (value != 0) ? "ON" : "OFF");
    
    if (teensy->led_change_callback != NULL)
        teensy->led_change_callback(teensy, TEENSYLCD_LED0, (value != 0));
}

static void led1_changed_hook(struct avr_irq_t *irq, uint32_t value, void *param)
{
    struct teensylcd_t *teensy = (struct teensylcd_t *)param;
    teensy->led_states[TEENSYLCD_LED1] = (value != 0);
    printf("LED1 turned %s\n", (value != 0) ? "ON" : "OFF");
    
    if (teensy->led_change_callback != NULL)
        teensy->led_change_callback(teensy, TEENSYLCD_LED1, (value != 0));
}

static void led2_changed_hook(struct avr_irq_t *irq, uint32_t value, void *param)
{
    struct teensylcd_t *teensy = (struct teensylcd_t *)param;
    teensy->led_states[TEENSYLCD_LED2] = (value != 0);
    printf("LED2 turned %s\n", (value != 0) ? "ON" : "OFF");
    
    if (teensy->led_change_callback != NULL)
        teensy->led_change_callback(teensy, TEENSYLCD_LED2, (value != 0));
}

/* adapted from button.h */
static avr_cycle_count_t button_auto_release(avr_t *avr, avr_cycle_count_t when, void *param)
{
    struct teensylcd_t *teensy = (struct teensylcd_t *)param;
    printf("button_auto_release\n");
    teensylcd_set_button_state(teensy, (enum TEENSYLCD_BUTTON)param, 0);
    return 0;
}

bool teensylcd_init(struct teensylcd_t *teensy, uint32_t frequency, int loglevel)
{
    /* create mcu */
    teensy->avr = avr_make_mcu_by_name("atmega32u4");
    if (teensy->avr == NULL)
    {
        fprintf(stderr, "Failed to create AVR core.\n");
        return false;
    }

    /* init mcu */
    avr_init(teensy->avr);
    teensy->avr->frequency = frequency;
    teensy->avr->log = loglevel;
    teensy->avr->trace = (loglevel >= LOG_TRACE);
    
    /* hook up leds */
    avr_irq_register_notify(avr_io_getirq(teensy->avr, AVR_IOCTL_IOPORT_GETIRQ('B'), 2), led0_changed_hook, teensy);
    avr_irq_register_notify(avr_io_getirq(teensy->avr, AVR_IOCTL_IOPORT_GETIRQ('B'), 3), led1_changed_hook, teensy);
    //LED2
    memset(teensy->led_states, 0, sizeof(teensy->led_states));
    teensy->led_change_callback = NULL;
    
    /* hook up buttons */
    teensy->button_irqs[TEENSYLCD_BUTTON_SW0] = avr_alloc_irq(&teensy->avr->irq_pool, 0, 1, &button_names[TEENSYLCD_BUTTON_SW0]);
    teensy->button_irqs[TEENSYLCD_BUTTON_SW1] = avr_alloc_irq(&teensy->avr->irq_pool, 0, 1, &button_names[TEENSYLCD_BUTTON_SW1]);
    avr_connect_irq(teensy->button_irqs[TEENSYLCD_BUTTON_SW0], avr_io_getirq(teensy->avr, AVR_IOCTL_IOPORT_GETIRQ('B'), 0));
    avr_connect_irq(teensy->button_irqs[TEENSYLCD_BUTTON_SW1], avr_io_getirq(teensy->avr, AVR_IOCTL_IOPORT_GETIRQ('B'), 1));
    memset(teensy->button_states, 0, sizeof(teensy->button_states));
    
    /* hook up lcd */
    pcd8544_init(teensy->avr, &teensy->lcd);
    printf("TeensyLCD initialized. Frequency = %uhz\n", teensy->avr->frequency);
    return true;
}

void teensylcd_reset(struct teensylcd_t *teensy)
{
    avr_reset(teensy->avr);
}

bool teensylcd_load_elf(struct teensylcd_t *teensy, const char *filename)
{
#ifdef __EMSCRIPTEN__
    fprintf(stderr, "Not compiled with ELF support.\n");
    return false;
#else
    elf_firmware_t f = {{0}};
    if (elf_read_firmware(filename, &f))
    {
        fprintf(stderr, "Failed to read ELF firmware\n");
        return false;
    }

    avr_load_firmware(teensy->avr, &f);
    return true;
#endif
}

bool teensylcd_load_hex(struct teensylcd_t *teensy, const char *filename)
{
    ihex_chunk_p chunk = NULL;
    int count = read_ihex_chunks(filename, &chunk);
    if (count <= 0)
    {
        fprintf(stderr, "Failed to read HEX firmware\n");
        return false;
    }

    fprintf(stdout, "  Loaded %d sections from hex file\n", count);
    elf_firmware_t f = {{0}};
    for (int ci = 0; ci < count; ci++)
    {
        if (chunk[ci].baseaddr < (1 * 1024 * 1024))
        {
            f.flash = chunk[ci].data;
            f.flashsize = chunk[ci].size;
            f.flashbase = chunk[ci].baseaddr;
            fprintf(stdout, "  Load HEX flash %08x, %d\n", f.flashbase, f.flashsize);
        }
        else if (chunk[ci].baseaddr >= AVR_SEGMENT_OFFSET_EEPROM || chunk[ci].baseaddr + AVR_SEGMENT_OFFSET_FLASH >= AVR_SEGMENT_OFFSET_EEPROM)
        {
            f.eeprom = chunk[ci].data;
            f.eesize = chunk[ci].size;
            fprintf(stdout, "  Load HEX eeprom %08x, %d\n", chunk[ci].baseaddr, f.eesize);
        }
    }

    avr_load_firmware(teensy->avr, &f);
    return true;
}

bool teensylcd_get_led_state(const struct teensylcd_t *teensy, enum TEENSYLCD_LED led)
{
    assert(led < NUM_TEENSYLCD_LEDS);
    return teensy->led_states[led];
}

void teensylcd_set_led_callback(struct teensylcd_t *teensy, teensylcd_led_change_callback callback)
{
    teensy->led_change_callback = callback;
}

bool teensylcd_get_button_state(const struct teensylcd_t *teensy, enum TEENSYLCD_BUTTON button)
{
    assert(button < NUM_TEENSYLCD_BUTTONS);
    return teensy->button_states[button];
}

void teensylcd_set_button_state(struct teensylcd_t *teensy, enum TEENSYLCD_BUTTON button, bool state)
{
    assert(button < NUM_TEENSYLCD_BUTTONS);
    if (teensy->button_states[button] == state)
        return;
        
    teensy->button_states[button] = state;
    avr_raise_irq(teensy->button_irqs[button], state);
    
    //printf("set button %u %u\n", button, state);
}

void teensylcd_push_button(struct teensylcd_t *teensy, enum TEENSYLCD_BUTTON button)
{
    assert(button < NUM_TEENSYLCD_BUTTONS);
    
    teensylcd_set_button_state(teensy, button, true);
    
    /* todo: case where both buttons are pushed at same time */
    avr_cycle_timer_cancel(teensy->avr, button_auto_release, teensy);
    avr_cycle_timer_register_usec(teensy->avr, 200000, button_auto_release, teensy);
}

bool teensylcd_run_single(struct teensylcd_t *teensy)
{
    int state = avr_run(teensy->avr);
    if (state == cpu_Done || state == cpu_Crashed)
        return false;
    else
        return true;
}

bool teensylcd_run_time_microseconds(struct teensylcd_t *teensy, uint32_t run_time)
{
    /* calculate cycles to execute for */
    uint64_t cycles_to_execute = avr_usec_to_cycles(teensy->avr, run_time);
    uint64_t last_cycles = teensy->avr->cycle;

    /**
     * execute as many cycles as possible in one avr_run_one call before the next 
     * interrupt timer has to kick. this reduces the number of times we back out,
     * just to invoke another run call again.
     */
    teensy->avr->run_cycle_limit = cycles_to_execute;
    
    /* shouldn't happen, but just in case */
    if (cycles_to_execute < teensy->next_cycles_sub)
    {
        fprintf(stderr, "WARN: AVR running ahead by %u cycles\n", (uint32_t)teensy->next_cycles_sub);
    }
    else
    {
        cycles_to_execute -= teensy->next_cycles_sub;
        teensy->next_cycles_sub = 0;
        //printf("AVR run %u cycles\n", (uint32_t)cycles_to_execute);

        while (cycles_to_execute > 0)
        {
            int state = avr_run(teensy->avr);
            if (state == cpu_Done || state == cpu_Crashed)
                return false;

            uint64_t diff_cycles = teensy->avr->cycle - last_cycles;
            last_cycles = teensy->avr->cycle;

            if (diff_cycles > cycles_to_execute) {
                teensy->next_cycles_sub = diff_cycles - cycles_to_execute;
                cycles_to_execute = 0;
            } else {
                cycles_to_execute -= diff_cycles;
            }
        }
    }

    return true;
}

bool teensylcd_run_time_milliseconds(struct teensylcd_t *teensy, uint32_t run_time)
{
    return teensylcd_run_time_microseconds(teensy, run_time * 1000);
}

bool teensylcd_run_frame(struct teensylcd_t *teensy, uint32_t target_framerate)
{
    assert(target_framerate > 0);
    return teensylcd_run_time_microseconds(teensy, 1000000 / target_framerate);
}

bool teensylcd_run_until_refresh(struct teensylcd_t *teensy)
{
    /* flatten out lcd position to a single value for easier comparison */
    uint32_t initial_lcd_pos = ((uint32_t)teensy->lcd.position_y * PCD8544_LCD_X) + (uint32_t)teensy->lcd.position_x;
    uint32_t lcd_pos = initial_lcd_pos;
    
    /* wait for the raster to reach this position again */
    do
    {
        uint32_t last_position = lcd_pos;
        while (lcd_pos == last_position)
        {
            /* run until the position changes */
            if (!teensylcd_run_single(teensy))
                return false;
            
            lcd_pos = ((uint32_t)teensy->lcd.position_y * PCD8544_LCD_X) + (uint32_t)teensy->lcd.position_x;
        }
    }
    while (lcd_pos != initial_lcd_pos);
    return true;
}

void teensylcd_cleanup(struct teensylcd_t *teensy)
{
    avr_terminate(teensy->avr);
    // @TODO the free() here breaks emscripten, figure out why, because it's a leak.
    //free(teensy->avr);
}
