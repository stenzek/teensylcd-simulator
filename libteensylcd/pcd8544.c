#include "pcd8544.h"
#include "avr_ioport.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>

// handler
static void lcd_control_handler(struct pcd8544_t *lcd, uint8_t value)
{
    /* From datasheet page 14 */
    
    /* NOP */
    if (value == 0)
        return;
        
    /* Function set -- check the first 5 bits match to avoid clashing with other functionality */
    if ((value & 0xF8) == 0x20)
    {
        lcd->extended_commands = (value & 0x01) != 0;
        // (value & 0x02) == vertical addressing, TODO!
        // (value & 0x04) == power down control
        return;
    }
    
    /* in extended command mode? (H=1) */
    if (lcd->extended_commands)
    {
        /* set Vop */
        if (value & 0x80)
        {
            printf("LCD contrast change %02X\n", value & 0x7F);
            lcd->contrast = value & 0x7F;
            return;
        }
        
        /* reserved */
        if (value & 0x40)
            return;
            
        /* function set 0x20 handled above */
            
        /* bias system */
        if (value & 0x10)
        {
            printf("LCD bias change %02X\n", value & 0x07);
            return;
        }
        
        /* reserved */
        if (value & 0x08)
        {
            /**
             * lcd.c sends this whilst in extended mode to set normal mode..
             * it shouldn't.
             */
            return;
        }
            
        /* temperature control */
        if (value & 0x04)
        {
            printf("LCD temperature coefficent change %02X\n", value & 0x03);
            return;
        }
        
        /* reserved */
        if (value & 0x02)
            return;
            
        /* reserved */
        if (value & 0x01)
            return;
    }
    else
    {
        /* set X address */
        if (value & 0x80) 
        {
            lcd->position_x = value & 0x3F;
            //printf("Set LCD column to %u\n", lcd->position_x);
            return;
        }
        
        /* set Y address */
        if (value & 0x40)
        {
            lcd->position_y = value & 0x07;
            //printf("Set LCD row to %u\n", lcd->position_y);
            return;
        }
        
        /* function set 0x20 handled above */
        
        /* reserved */
        if (value & 0x10)
            return;
        
        /* display control */
        if (value & 0x08)
        {
            // ((value & 0x4) >> 1) | (value & 0x1) -> D/E
            // -> 00 - display blank, 10 - normal mode, 01 - all display segments on, 11 - inverse video mode
            lcd->invert_display = (value == 0x03);
            return;
        }
        
        /* reserved */
        if (value & 0x04)
            return;
    }
    
    // if we hit here it wasn't handled
    printf("Unhandled LCD command %02X, extended commands %s\n", value, (lcd->extended_commands) ? "on" : "off");
}

// handler
static void lcd_data_handler(struct pcd8544_t *lcd, uint8_t value)
{
    if (lcd->reset)
        return;

    /* value is 8 bits, each corresponding to a row of a single column */
    uint8_t pixel_x = lcd->position_x;
    uint8_t pixel_y = lcd->position_y * 8;
    for (unsigned char i = 0; i < 8; i++)
    {
        bool pixel_state = ((value >> i) & 0x01) != 0;
        //printf("Set pixel [%u, %u] to %u\n", pixel_x, pixel_y, (uint32_t)pixel_state);
        
        /* calculate index */
        uint32_t idx = ((uint32_t)pixel_y * PCD8544_LCD_X) + (uint32_t)pixel_x;
        uint32_t byte = idx / 8;
        uint32_t bit = idx % 8;
        
        // update pixel
        if (pixel_state)
            lcd->pixel_state[byte] |= (1 << bit);
        else
            lcd->pixel_state[byte] &= ~(1 << bit);

        lcd->pixels_changed |= true;
        pixel_y++;
    }

    // increment column/row
    lcd->position_x++;
    if (lcd->position_x == 84) {
        lcd->position_x = 0;
        lcd->position_y++;
        lcd->position_y %= (48 / 8);
    }
}

static void lcd_dcpin_changed_hook(struct avr_irq_t *irq, uint32_t value, void *param)
{
    struct pcd8544_t *lcd = (struct pcd8544_t *)param;
    
    //printf("LCD DCPIN changed -> %u %s\n", value, (value == 0) ? "ie control" : "ie data");
    lcd->data_flag = (value != 0);
}

static void lcd_rstpin_changed_hook(struct avr_irq_t *irq, uint32_t value, void *param)
{
    struct pcd8544_t *lcd = (struct pcd8544_t *)param;

    //printf("LCD RSTPIN went %s\n", (value == 0) ? "low" : "high");
    lcd->reset = (value == 0);
    if (lcd->reset) {
        printf("LCD now inactive due to reset...\n");
        memset(lcd->pixel_state, 0, sizeof(lcd->pixel_state));
    } else {
        printf("LCD now active.\n");
    }
}

static void lcd_dinpin_changed_hook(struct avr_irq_t *irq, uint32_t value, void *param)
{
    struct pcd8544_t *lcd = (struct pcd8544_t *)param;
    //printf("LCD DINPIN changed -> %u\n", value);
    lcd->data_pin_value = (value != 0);
}

static void lcd_sckpin_changed_hook(struct avr_irq_t *irq, uint32_t value, void *param)
{
    struct pcd8544_t *lcd = (struct pcd8544_t *)param;
    //printf("LCD SCKPIN went %s\n", (value == 0) ? "low ie start of bit" : "high ie end of bit");
    
    /* datasheet page 11: "When SCE is HIGH, SCLK clock signals are ignored; during the HIGH time of SCE" */
    if (!lcd->chip_enable)
        return;
    
    /* datasheet page 11: SDIN is sampled at the positive edge of SCLK */
    if (value != 0) {
        lcd->data_shift_register <<= 1;
        lcd->data_shift_register |= (lcd->data_pin_value) ? 1 : 0;
        
        /* end of byte? */
        lcd->clock_count++;
        if (lcd->clock_count >= 8)
        {
            /* execute command */
            //printf("   value is 0x%02X / %u\n", lcd->data_shift_register, lcd->data_shift_register);
            if (lcd->data_flag) {
                lcd_data_handler(lcd, lcd->data_shift_register);
            } else {
                lcd_control_handler(lcd, lcd->data_shift_register);
            }
        
            lcd->data_shift_register = 0;
            lcd->clock_count = 0;
        }
    }
}

static void lcd_scepin_changed_hook(struct avr_irq_t *irq, uint32_t value, void *param)
{
    struct pcd8544_t *lcd = (struct pcd8544_t *)param;
    //printf("LCD SCEPIN changed -> %u %s\n", value, (value != 0) ? "ie end of data" : "start of data");
    lcd->chip_enable = (value == 0);
}

void pcd8544_init(struct avr_t *avr, struct pcd8544_t *lcd)
{
    /* reset data */
    lcd->position_x = 0;
    lcd->position_y = 0;
    memset(lcd->pixel_state, 0, sizeof(lcd->pixel_state));
    lcd->contrast = 0;
    lcd->pixels_changed = true;
    lcd->reset = false;
    lcd->chip_enable = true;
    lcd->clock_count = 0;
    lcd->data_shift_register = 0;
    lcd->data_pin_value = false;
    lcd->data_flag = false;
    lcd->extended_commands = false;
    lcd->invert_display = false;
    
    /* hook up lcd */
    avr_irq_register_notify(avr_io_getirq(avr, AVR_IOCTL_IOPORT_GETIRQ('F'), 7), lcd_sckpin_changed_hook, lcd);
    avr_irq_register_notify(avr_io_getirq(avr, AVR_IOCTL_IOPORT_GETIRQ('B'), 6), lcd_dinpin_changed_hook, lcd);
    avr_irq_register_notify(avr_io_getirq(avr, AVR_IOCTL_IOPORT_GETIRQ('B'), 5), lcd_dcpin_changed_hook, lcd);
    avr_irq_register_notify(avr_io_getirq(avr, AVR_IOCTL_IOPORT_GETIRQ('B'), 4), lcd_rstpin_changed_hook, lcd);
    avr_irq_register_notify(avr_io_getirq(avr, AVR_IOCTL_IOPORT_GETIRQ('D'), 7), lcd_scepin_changed_hook, lcd);
}

bool pcd8544_get_pixel(const struct pcd8544_t *lcd, unsigned char x, unsigned char y)
{
    assert(x < PCD8544_LCD_X && y < PCD8544_LCD_Y);
    
    /* calculate index */
    uint32_t idx = ((uint32_t)y * PCD8544_LCD_X) + (uint32_t)x;
    uint32_t byte = idx / 8;
    uint32_t bit = idx % 8;

    return (lcd->pixel_state[byte] & (1 << bit)) != 0;
}

void pcd8544_render_screen(const struct pcd8544_t *lcd, void *pixels, size_t pitch)
{
    /* this would be where to do inversion */
    uint8_t pixel_on_brightness = 127 - lcd->contrast;
    uint8_t pixel_off_brightness = 230;
    if (lcd->invert_display)
    {
        uint8_t swap = pixel_on_brightness;
        pixel_on_brightness = pixel_off_brightness;
        pixel_off_brightness = swap;
    }
    
    /* this could be seriously optimized... */
    for (size_t y = 0; y < PCD8544_LCD_Y; y++)
    {
        uint8_t *pixout = (uint8_t *)pixels + (y * pitch);
        for (size_t x = 0; x < PCD8544_LCD_X; x++)
        {
            bool pixon = pcd8544_get_pixel(lcd, x, y);
            uint8_t brightness = (pixon) ? pixel_on_brightness : pixel_off_brightness;
            *(pixout++) = brightness;   /* R */
            *(pixout++) = brightness;   /* G */    
            *(pixout++) = brightness;   /* B */
            *(pixout++) = 255;          /* A */
        }
    }
}

void pcd8544_render_luminance(const struct pcd8544_t *lcd, void *pixels)
{
    /* this would be where to do inversion */
    uint8_t pixel_on_brightness = 127 - lcd->contrast;
    uint8_t pixel_off_brightness = 230;
    if (lcd->invert_display)
    {
        uint8_t swap = pixel_on_brightness;
        pixel_on_brightness = pixel_off_brightness;
        pixel_off_brightness = swap;
    }
    
    /* this could be seriously optimized... */
    for (size_t y = 0; y < PCD8544_LCD_Y; y++)
    {
        uint8_t *pixout = (uint8_t *)pixels + (y * PCD8544_LCD_X);
        for (size_t x = 0; x < PCD8544_LCD_X; x++)
        {
            bool pixon = pcd8544_get_pixel(lcd, x, y);
            uint8_t brightness = (pixon) ? pixel_on_brightness : pixel_off_brightness;
            *(pixout++) = brightness;   /* L */
        }
    }
}