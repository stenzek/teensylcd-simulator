#ifndef __PCD8544_H
#define __PCD8544_H

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include "sim_avr.h"

#define PCD8544_LCD_X 84
#define PCD8544_LCD_Y 48

/* state */
struct pcd8544_t
{
	/* current "raster" position */
	uint8_t position_x;					/* 0-84 */
	uint8_t position_y;					/* 0-6, groups of 8 */
	
	/* pixel bitmap, one bit for each pixel */
	uint8_t pixel_state[PCD8544_LCD_X * PCD8544_LCD_Y / 8];
	
	/* contrast level or Vop (0-127) */
	uint8_t contrast;
	
	/* changed flag, feel free to reset externally */
	bool pixels_changed;
	
	/* low-level access */
	
	/* reset state, TRUE indicates reset pin pulled low */
	bool reset;
    
    /* chip enable */
    bool chip_enable;
    
    /* current clock (0-7) */
    uint8_t clock_count;
	
	/* data transfer shift register */
	uint8_t data_shift_register;
	
	/* data pin level */
	bool data_pin_value;
	
	/* current mode, control or data */
	bool data_flag;
	
	/* whether extended commands are enabled */
	bool extended_commands;
    
    /* inverted colours */
    bool invert_display;
};

/* initialize state */
void pcd8544_init(struct avr_t *avr, struct pcd8544_t *lcd);

/* returns a 0/1 depending on whether the pixel is on/off */
bool pcd8544_get_pixel(const struct pcd8544_t *lcd, unsigned char x, unsigned char y);

/* writes the lcd contents out to an image, using the current contrast level */
void pcd8544_render_screen(const struct pcd8544_t *lcd, void *pixels, size_t pitch);

/* writes the lcd contents out to an image, using the current contrast level */
void pcd8544_render_luminance(const struct pcd8544_t *lcd, void *pixels);

#endif      // __PCD8544_H

