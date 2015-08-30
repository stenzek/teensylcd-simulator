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

static SDL_Surface *screen = NULL;
static uint8_t *luminance_buffer = NULL;

static void(*led_change_callback)(int index, int value);

static void led_change_callback_wrapper(void *teensy, enum TEENSYLCD_LED led, bool state)
{
    if (led_change_callback != NULL)
        led_change_callback((int)led, (state) ? 1 : 0);
}

void set_led_change_callback(void(*fptr)(int index, int value))
{
    led_change_callback = fptr;
}

/* methods */
void setup();
void load_firmware(const char *filename, uint32_t frequency, bool verbose, bool trace_interrupts);
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

void load_firmware(const char *filename, uint32_t frequency, bool verbose, bool trace_interrupts)
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
    if (teensy == NULL || !teensylcd_init(teensy, frequency))
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
    printf("AVR core running...\n");
    
    /* prevent massive jump in time */
    last_time = get_time_microseconds();
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
            if (event.key.keysym.scancode == SDL_SCANCODE_Z)
            {
                /* push sw0 */
                teensylcd_set_button_state(teensy, TEENSYLCD_BUTTON_SW0, (event.type == SDL_KEYDOWN));
            }
            else if (event.key.keysym.scancode == SDL_SCANCODE_X)
            {
                /* push sw1 */
                teensylcd_set_button_state(teensy, TEENSYLCD_BUTTON_SW1, (event.type == SDL_KEYDOWN));
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
    setup();
    load_firmware("start.hex", TEENSYLCD_DEFAULT_FREQUENCY, true, false);
    emscripten_set_main_loop(run_loop, 0, 1);
    return 0;
}

