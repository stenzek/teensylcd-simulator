#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <libgen.h>
#include <stdbool.h>
#include <signal.h>
#include <sys/time.h>
#include <emscripten.h>
#include <assert.h>

#include <SDL.h>

#include "teensylcd.h"
#include "timer.h"
#include "sim_avr.h"
#include "sim_gdb.h"

#define MAXIMUM_TIME_DIFF (1000000) /* one second */

volatile bool exit_flag = false;

static struct teensylcd_t *teensy = NULL;
static uint64_t last_time = 0;
static uint32_t last_frequency = TEENSYLCD_DEFAULT_FREQUENCY;

static SDL_Surface *screen = NULL;
static uint8_t *luminance_buffer = NULL;

static void(*led_change_callback)(int index, int value);
static void(*frequency_change_callback)(uint32_t old_frequency, uint32_t new_frequency);

static void led_change_callback_wrapper(void *teensy, enum TEENSYLCD_LED led, bool state)
{
    if (led_change_callback != NULL)
        led_change_callback((int)led, (state) ? 1 : 0);
}

void set_led_change_callback(void(*fptr)(int, int))
{
    led_change_callback = fptr;
}

void set_frequency_change_callback(void(*fptr)(uint32_t, uint32_t))
{
    frequency_change_callback = fptr;
}

static bool enable_trace_ioports = false;
static bool enable_trace_interrupts = false;

static void tracer_event_callback(struct avr_t *avr, void *param, avr_tracer_event event, uint32_t p1, uint32_t p2, uint32_t p3, uint32_t p4)
{
    // filter out LCD-related events
    if (teensylcd_is_lcd_tracer_event(event, p1, p2, p3, p4))
        return;

    switch (event)
    {
    case avr_tracer_event_ioport:
        {
            if (!enable_trace_ioports)
                return;

            printf("TRACE: PIN%c BIT %u %u -> %u\n", p1, p2, p3, p4);
            break;
        }

    case avr_tracer_event_ddr:
        {
            if (!enable_trace_ioports)
                return;

            printf("TRACE: DDR%c 0x%02X -> 0x%02X (0b%u%u%u%u%u%u%u%u)\n", p1, p2, p3, (p3>>7)&1, (p3>>6)&1, (p3>>5)&1, (p3>>4)&1, (p3>>3)&1, (p3>>2)&1, (p3>>1)&1, p3&1);
            break;
        }

    case avr_tracer_event_interrupt:
        {
            if (!enable_trace_interrupts)
                return;

            // in datasheets interrupt vectors are one-based not zero-based
            printf("TRACE: interrupt %u fired\n", p1 + 1);
            break;
        }
    }
}

void set_tracer_messages(bool ioports, bool interrupts)
{
    enable_trace_ioports = ioports;
    enable_trace_interrupts = interrupts;

    printf("Trace ioports: %s\n", (ioports) ? "on" : "off");
    printf("Trace interrupts: %s\n", (interrupts) ? "on" : "off");
}

/* methods */
void setup();
void load_firmware(bool new_board, uint32_t frequency, const char *filename, bool verbose, bool trace_interrupts);
void reset_processor();
void pause_processor();
void resume_processor();
void set_processor_frequency(int frequency);
void set_button_state(int button, bool state);
void run_loop();

void setup()
{
    SDL_Init(SDL_INIT_VIDEO);
    
    /* create lcd window */
    fprintf(stdout, "Creating LCD window...\n");
    screen = SDL_SetVideoMode(PCD8544_LCD_X, PCD8544_LCD_Y, 32, SDL_HWSURFACE);
    luminance_buffer = (uint8_t *)malloc(PCD8544_LCD_X * PCD8544_LCD_Y);
    assert(screen != NULL && luminance_buffer != NULL);
    
    EM_ASM("SDL.defaults.copyOnLock = false; SDL.defaults.discardOnLock = true; SDL.defaults.opaqueFrontBuffer = true;");   
}

void load_firmware(bool new_board, uint32_t frequency, const char *filename, bool verbose, bool trace_interrupts)
{
    if (teensy != NULL)
    {
        // previous run, cancel it
        teensylcd_cleanup(teensy);
        free(teensy);
        teensy = NULL;
    }
    
    /* create teensy */
    teensy = (struct teensylcd_t *)malloc(sizeof(struct teensylcd_t));
    if (teensy == NULL) {
        fprintf(stderr, "malloc(sizeof(struct teensylcd_t)) failed.");
        return;
    }

    /* initialize it */
    bool init_result = false;
    if (new_board)
        init_result = teensylcd_init_new(teensy, frequency, LOG_TRACE);
    else
        init_result = teensylcd_init(teensy, frequency, LOG_TRACE);
    if (!init_result)
    {
        fprintf(stderr, "Failed to create teensylcd.\n");
        free(teensy);
        teensy = NULL;
        return;
    }
    
    /* update log */
    if (verbose)
    {
        teensy->avr->log = LOG_TRACE;
        teensy->avr->trace = true;
    }
    
    /* trace interrupts */
    if (trace_interrupts)
    {
        for (int vi = 0; vi < teensy->avr->interrupts.vector_count; vi++)
            teensy->avr->interrupts.vector[vi]->trace = 1;
    }

    /* tracer */
    teensy->avr->tracer_callback = tracer_event_callback;

    // parse firmware
    if (strstr(filename, ".elf") != NULL)
    {      
        fprintf(stdout, "Loading ELF firmware: %s...\n", filename);
        if (!teensylcd_load_elf(teensy, filename))
        {
            fprintf(stderr, "Failed to read ELF firmware\n");
            free(teensy);
            teensy = NULL;
            return;
        }
    }
    else
    {
        // hex
        fprintf(stdout, "Loading HEX firmware: %s...\n", filename);
        if (!teensylcd_load_hex(teensy, filename))
        {
            fprintf(stderr, "Failed to read ELF firmware\n");
            free(teensy);
            teensy = NULL;
            return;
        }
    }
    
    /* set led callback */
    teensylcd_set_led_callback(teensy, led_change_callback_wrapper);

    /* log */
    printf("AVR core initialized.\n");
    
    /* prevent massive jump in time */
    last_time = get_time_microseconds();
    last_frequency = frequency;
}

void run_loop()
{
    static bool wait_flag = false;
    if (teensy == NULL)
    {
        if (!wait_flag)
        {
            fprintf(stdout, "Waiting for firmware...\n");
            wait_flag = true;
        }
        
        return;
    }
    wait_flag = false;
           
    uint64_t now_time = get_time_microseconds();
    uint64_t time_diff = now_time - last_time;
    last_time = now_time;
    if (time_diff > MAXIMUM_TIME_DIFF)
    {
        fprintf(stderr, "WARN: Time diff (%u) exceeds MAXIMUM_TIME_DIFF (%u). Capping at MAXIMUM_TIME_DIFF", (uint32_t)time_diff, MAXIMUM_TIME_DIFF);
        time_diff = MAXIMUM_TIME_DIFF;
    }

    /* pump events */
    SDL_PumpEvents();

    /* handle inputs */
    while (true)
    {
        SDL_Event event;
        int nevents = SDL_PeepEvents(&event, 1, SDL_GETEVENT, SDL_FIRSTEVENT, SDL_LASTEVENT);
        if (nevents <= 0)
            break;
    
        if (event.type == SDL_KEYDOWN || event.type == SDL_KEYUP)
        {
            static const int button_mapping[][2] = {
                { SDL_SCANCODE_Z, TEENSYLCD_BUTTON_SW0 },
                { SDL_SCANCODE_X, TEENSYLCD_BUTTON_SW1 },
                { SDL_SCANCODE_C, TEENSYLCD_BUTTON_SW2 },
                { SDL_SCANCODE_V, TEENSYLCD_BUTTON_SW3 },
                { SDL_SCANCODE_W, TEENSYLCD_STICK_UP },
                { SDL_SCANCODE_S, TEENSYLCD_STICK_DOWN },
                { SDL_SCANCODE_A, TEENSYLCD_STICK_LEFT },
                { SDL_SCANCODE_D, TEENSYLCD_STICK_RIGHT },
                { SDL_SCANCODE_F, TEENSYLCD_STICK_PUSH }
            };

            for (int i = 0; i < sizeof(button_mapping) / sizeof(button_mapping[0]); i++) {
                if (button_mapping[i][0] == event.key.keysym.scancode) {
                    teensylcd_set_button_state(teensy, (enum TEENSYLCD_BUTTON)button_mapping[i][1], (event.type == SDL_KEYDOWN));
                    break;
                }
            }
        }
    }
    
    /* run the avr for the time difference */
    if (!teensylcd_run_time_microseconds(teensy, time_diff))
        return;
    
    /* if the lcd data has changed, update the display */
    if (teensy->lcd.pixels_changed)
    {
        //printf("Window update\n");
        
        pcd8544_render_luminance(&teensy->lcd, luminance_buffer);
        
        if (SDL_MUSTLOCK(screen))
            SDL_LockSurface(screen);
            
        /* map luminance to rgba */
        for (size_t y = 0; y < PCD8544_LCD_Y; y++)
        {
            //const uint8_t *pixin = luminance_buffer + (y * PCD8544_LCD_X);
            uint32_t *pixout = screen->pixels + (y * screen->pitch);
            for (size_t x = 0; x < PCD8544_LCD_X; x++)
            {
                //uint8_t luminance = *(pixin++);
                uint8_t luminance = luminance_buffer[(y * PCD8544_LCD_X) + x];
                *(pixout++) = SDL_MapRGBA(screen->format, luminance, luminance, luminance, 255);
            }
        }
        
        if (SDL_MUSTLOCK(screen))
            SDL_UnlockSurface(screen);

        SDL_Flip(screen);
        
        teensy->lcd.pixels_changed = false;
    }

    /* check for frequency changes */
    if (teensy->avr->frequency != last_frequency)
    {
        if (frequency_change_callback != NULL)
            frequency_change_callback(last_frequency, teensy->avr->frequency);

        last_frequency = teensy->avr->frequency;
    }
}

void reset_processor()
{
    fprintf(stdout, "Resetting processor.\n");
    teensylcd_reset(teensy);
}

void pause_processor()
{
    fprintf(stdout, "Pausing processor.\n");
    emscripten_pause_main_loop();
}

void resume_processor()
{
    fprintf(stdout, "Resuming processor.\n");
    emscripten_resume_main_loop();

    /* prevent massive jump in time */
    last_time = get_time_microseconds();
}

void set_processor_frequency(int frequency)
{
    if (teensy->avr->frequency == frequency)
        return;

    teensy->avr->frequency = frequency;
    last_time = get_time_microseconds();
    fprintf(stdout, "Frequency changed to %d hz\n", frequency);
}

void set_button_state(int button, bool state)
{
    teensylcd_set_button_state(teensy, button, state);
}

int main(int argc, char *argv[])
{
    fprintf(stdout, "main()\n");
    setup();
    fprintf(stdout, "setup() done\n");
    load_firmware(false, TEENSYLCD_DEFAULT_FREQUENCY, "start.hex", true, false);
    fprintf(stdout, "load_firmware() done\n");
    emscripten_set_main_loop(run_loop, 0, 1);
    return 0;
}

