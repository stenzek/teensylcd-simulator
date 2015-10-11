#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <libgen.h>
#include <stdbool.h>
#include <signal.h>
#include <getopt.h>
#include <sys/time.h>

#include <SDL.h>
#include <SDL_main.h>

#include "teensylcd.h"
#include "timer.h"
#include "sim_avr.h"
#include "sim_gdb.h"

volatile bool exit_flag = false;

static void sigint_handler(int n)
{
    exit_flag = true;
}

static void tracer_event_callback(struct avr_t *avr, void *param, avr_tracer_event event, uint32_t p1, uint32_t p2, uint32_t p3, uint32_t p4)
{
    // filter out LCD-related events
    if (teensylcd_is_lcd_tracer_event(event, p1, p2, p3, p4))
        return;

    // filter interrupt messages
    if (event == avr_tracer_event_interrupt)
        return;

    printf("tracer_event(%u, %u, %u, %u, %u)\n", event, p1, p2, p3, p4);
}

static void usage(const char *progname)
{
    fprintf(stderr, "TeensyLCD Simulator\n");
    fprintf(stderr, "Connor McLaughlin, n8803951\n");
    fprintf(stderr, "Using simavr by Michel Pollet\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "Usage: %s [-f <frequency>] [-e <elf_file>] [-x <hex_file>] [-g port] [-v] [-t] [-h]\n", progname);
    fprintf(stderr, "       -f: Use this frequency, default 8000000 or 8mhz\n");
    fprintf(stderr, "       -e: Load this ELF file as firmware\n");
    fprintf(stderr, "       -x: Load this HEX file as firmware\n");
    fprintf(stderr, "       -g: Enable gdb on port\n");
    fprintf(stderr, "       -v: Verbose output\n");
    fprintf(stderr, "       -t: Trace interrupts\n");
    fprintf(stderr, "       -h: Help detail\n");
    fprintf(stderr, "\n");
}

int main(int argc, char *argv[])
{
    const char *elf_filename = NULL;
    const char *hex_filename = NULL;
    uint32_t frequency = 8000000;
    uint32_t gdb_port = 0;
    bool verbose = false;
    bool trace_interrupts = false;

    // parse options
    {
        if (argc == 1)
        {
            usage(argv[0]);
            return -1;
        }

        int c;
        while ((c = getopt(argc, argv, "f:e:x:g:vth")) != -1)
        {
            switch (c)
            {
            case 'f':
                frequency = atoi(optarg);
                break;
            case 'e':
                elf_filename = optarg;
                break;
            case 'x':
                hex_filename = optarg;
                break;
            case 'g':
                gdb_port = atoi(optarg);
                break;
            case 'v':
                verbose = true;
                break;
            case 't':
                trace_interrupts = true;
                break;
            case 'h':
                usage(argv[0]);
                return 0;
            case '?':
                usage(argv[0]);
                return -1;
            }
        }
    }

    if (elf_filename == NULL && hex_filename == NULL)
    {
        fprintf(stderr, "Either an ELF or HEX filename must be provided.\n");
        return -1;
    }

    if (frequency <= 0)
    {
        fprintf(stderr, "Invalid frequency, it must be a positive number\n");
        return -1;
    }

    if (gdb_port < 0 || gdb_port > 65535)
    {
        fprintf(stderr, "Invalid port, it must be 1-65535\n");
        return -1;
    }
    
    /* create teensy */
    struct teensylcd_t *teensy = (struct teensylcd_t *)malloc(sizeof(struct teensylcd_t));
    if (teensy == NULL || !teensylcd_init(teensy, frequency, (verbose) ? LOG_TRACE : LOG_WARNING))
    {
        fprintf(stderr, "Failed to create teensylcd.\n");
        return -1;
    }
       
    /* trace interrupts */
    if (trace_interrupts)
    {
        for (int vi = 0; vi < teensy->avr->interrupts.vector_count; vi++)
            teensy->avr->interrupts.vector[vi]->trace = 1;
    }

    // parse firmware
    if (elf_filename != NULL)
    {      
        fprintf(stdout, "Loading ELF firmware: %s...\n", elf_filename);
        if (!teensylcd_load_elf(teensy, elf_filename))
        {
            fprintf(stderr, "Failed to read ELF firmware\n");
            return -1;
        }
    }
    else
    {
        // hex
        fprintf(stdout, "Loading HEX firmware: %s...\n", hex_filename);
        if (!teensylcd_load_hex(teensy, hex_filename))
        {
            fprintf(stderr, "Failed to read ELF firmware\n");
            return -1;
        }
    }

	/* even if not setup at startup, activate gdb if crashing */
	teensy->avr->gdb_port = (gdb_port != 0) ? gdb_port : 1234;
	if (gdb_port != 0) {
		//teensy->avr->state = cpu_Stopped;
		avr_gdb_init(teensy->avr);
	}

    /* setup tracer */
    teensy->avr->tracer_callback = tracer_event_callback;

    /* create lcd window */
    fprintf(stdout, "Creating LCD window...\n");
    uint32_t window_scale = 2;
    SDL_Window *window = SDL_CreateWindow("TeensyLCD Simulator", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, PCD8544_LCD_X * window_scale, PCD8544_LCD_Y * window_scale, 0);
    SDL_Surface *backbuffer = SDL_CreateRGBSurface(0, PCD8544_LCD_X, PCD8544_LCD_Y, 32, 0xFF, 0xFF00, 0xFF0000, 0xFF000000);
    SDL_SetSurfaceBlendMode(backbuffer, SDL_BLENDMODE_NONE);

    /* hook signals */
    signal(SIGINT, sigint_handler);

    /* log */
    printf("AVR core running...\n");

    uint64_t last_time = get_time_microseconds();
    while (!exit_flag) 
    {
        uint64_t now_time = get_time_microseconds();
        uint64_t time_diff = now_time - last_time;
        last_time = now_time;

        /* pump events */
        SDL_PumpEvents();

        /* handle inputs */
        SDL_Event events[128];
        int nevents = SDL_PeepEvents(events, sizeof(events) / sizeof(events[0]), SDL_GETEVENT, SDL_FIRSTEVENT, SDL_LASTEVENT);
        if (nevents > 0)
        {
            for (int i = 0; i < nevents; i++)
            {
                if (events[i].type == SDL_KEYDOWN || events[i].type == SDL_KEYUP)
                {
                    if (events[i].key.keysym.scancode == SDL_SCANCODE_Z)
                    {
                        /* push sw0 */
                        teensylcd_set_button_state(teensy, TEENSYLCD_BUTTON_SW0, (events[i].type == SDL_KEYDOWN));
                    }
                    else if (events[i].key.keysym.scancode == SDL_SCANCODE_X)
                    {
                        /* push sw1 */
                        teensylcd_set_button_state(teensy, TEENSYLCD_BUTTON_SW1, (events[i].type == SDL_KEYDOWN));
                    }
                    else if (events[i].key.keysym.scancode >= SDL_SCANCODE_1 && events[i].key.keysym.scancode <= SDL_SCANCODE_9)
                    {
                        /* set display scale */
                        window_scale = events[i].key.keysym.scancode - SDL_SCANCODE_1 + 1;
                        SDL_SetWindowSize(window, PCD8544_LCD_X * window_scale, PCD8544_LCD_Y * window_scale);
                        teensy->lcd.pixels_changed = true;
                    }
                }
            }
        }
        
        /* run the avr for the time difference */
        if (!teensylcd_run_time_microseconds(teensy, time_diff))
            break;
        
        /* if the lcd data has changed, update the display */
        if (teensy->lcd.pixels_changed)
        {
            //printf("Window update\n");
            SDL_LockSurface(backbuffer);
            pcd8544_render_screen(&teensy->lcd, backbuffer->pixels, backbuffer->pitch);
            SDL_UnlockSurface(backbuffer);
            SDL_Surface *window_surface = SDL_GetWindowSurface(window);
            SDL_BlitScaled(backbuffer, NULL, window_surface, NULL);        
            SDL_UpdateWindowSurface(window);
            teensy->lcd.pixels_changed = false;
        }

        // sleep a little bit
        // run at roughly 60fps
        time_diff = get_time_microseconds() - now_time;
        if (time_diff < 16667) {
            usleep(16667 - time_diff);
            //printf("sleep  : %u\n", 16667 - (uint32_t)time_diff);
        }
	}
 
    fprintf(stdout, "Exiting...\n");

    SDL_Quit();   
    return 0;
}

