/*
    sim_mega32u4.c

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

#include "sim_avr.h"
#include "sim_core_declare.h"
#include "avr_eeprom.h"
#include "avr_flash.h"
#include "avr_watchdog.h"
#include "avr_extint.h"
#include "avr_ioport.h"
#include "avr_uart.h"
#include "avr_adc.h"
#include "avr_timer.h"
#include "avr_spi.h"
#include "avr_twi.h"
#include "avr_clkpr.h"

#define _AVR_IO_H_
#define __ASSEMBLER__
#include "avr/iom32u4.h"

static void m32u4_init(struct avr_t * avr);
static void m32u4_reset(struct avr_t * avr);

//#ifndef EFUSE_DEFAULT
//#define EFUSE_DEFAULT 0xff
//#endif

struct mcu_t {
    avr_t          core;
    avr_clkpr_t     clkpr;
    avr_eeprom_t     eeprom;
    avr_flash_t     selfprog;
    avr_watchdog_t    watchdog;
    avr_extint_t    extint;
    avr_ioport_t    portb, portc, portd, portf;
    avr_uart_t        uart;
    avr_adc_t        adc;
    avr_timer_t        timer0,timer1,timer3, timer4;
    avr_spi_t        spi;
    avr_twi_t        twi;
} mcu_mega32u4 = {
    .core = {
        .mmcu = "atmega32u4",
        DEFAULT_CORE(4),

        .init = m32u4_init,
        .reset = m32u4_reset,
    },
    AVR_CLKPR_DECLARE(16000000, CLKPR),
    AVR_EEPROM_DECLARE(EE_READY_vect),
    AVR_SELFPROG_DECLARE(SPMCSR, SPMEN, SPM_READY_vect),
    AVR_WATCHDOG_DECLARE(WDTCSR, _VECTOR(0)),
    .extint = {
        AVR_EXTINT_DECLARE(0, 'D', PORTD0),
        AVR_EXTINT_DECLARE(1, 'D', PORTD1),
        AVR_EXTINT_DECLARE(2, 'D', PORTD2),
        AVR_EXTINT_DECLARE(3, 'D', PORTD3),
        //AVR_EXTINT_DECLARE(6, 'E', PORTE6)  // accesses nullptr
#ifdef INT2
        //AVR_ASYNC_EXTINT_DECLARE(2, 'B', PB2),
#endif
    },
    .portb = {
        .name = 'B', .r_port = PORTB, .r_ddr = DDRB, .r_pin = PINB,
        .pcint = {
            .enable = AVR_IO_REGBIT(PCICR, PCIE0),
            .raised = AVR_IO_REGBIT(PCIFR, PCIF0),
            .vector = PCINT0_vect
        },
        .r_pcint = PCMSK0
    },
    AVR_IOPORT_DECLARE(c, 'C', C),
    AVR_IOPORT_DECLARE(d, 'D', D),
    AVR_IOPORT_DECLARE(f, 'F', F),
    .uart = {
        .disabled = AVR_IO_REGBIT(PRR1,PRUSART1),
        .name = '1',
        .r_udr = UDR1,

        .txen = AVR_IO_REGBIT(UCSR1B, TXEN1),
        .rxen = AVR_IO_REGBIT(UCSR1B, RXEN1),
        .usbs = AVR_IO_REGBIT(UCSR1C, USBS1),
        .ucsz = AVR_IO_REGBITS(UCSR1C, UCSZ10, 0x3), // 2 bits
        .ucsz2 = AVR_IO_REGBIT(UCSR1B, UCSZ12),     // 1 bits

        .r_ucsra = UCSR1A,
        .r_ucsrb = UCSR1B,
        .r_ucsrc = UCSR1C,
        .r_ubrrl = UBRR1L,
        .r_ubrrh = UBRR1H,
        .rxc = {
            .enable = AVR_IO_REGBIT(UCSR1B, RXCIE1),
            .raised = AVR_IO_REGBIT(UCSR1A, RXC1),
            .vector = USART1_RX_vect,
        },
        .txc = {
            .enable = AVR_IO_REGBIT(UCSR1B, TXCIE1),
            .raised = AVR_IO_REGBIT(UCSR1A, TXC1),
            .vector = USART1_TX_vect,
        },
        .udrc = {
            .enable = AVR_IO_REGBIT(UCSR1B, UDRIE1),
            .raised = AVR_IO_REGBIT(UCSR1A, UDRE1),
            .vector = USART1_UDRE_vect,
        },
    },
    .adc = {
        .r_admux = ADMUX,
        .mux = { AVR_IO_REGBIT(ADMUX, MUX0), AVR_IO_REGBIT(ADMUX, MUX1),
                    AVR_IO_REGBIT(ADMUX, MUX2), AVR_IO_REGBIT(ADMUX, MUX3),},
        .ref = { AVR_IO_REGBIT(ADMUX, REFS0), AVR_IO_REGBIT(ADMUX, REFS1)},
        .ref_values = { [1] = ADC_VREF_AVCC, [3] = ADC_VREF_V256 },

        .adlar = AVR_IO_REGBIT(ADMUX, ADLAR),
        .r_adcsra = ADCSRA,
        .aden = AVR_IO_REGBIT(ADCSRA, ADEN),
        .adsc = AVR_IO_REGBIT(ADCSRA, ADSC),
        // no ADATE .adate = AVR_IO_REGBIT(ADCSRA, ADATE),
        .adps = { AVR_IO_REGBIT(ADCSRA, ADPS0), AVR_IO_REGBIT(ADCSRA, ADPS1), AVR_IO_REGBIT(ADCSRA, ADPS2),},

        .r_adch = ADCH,
        .r_adcl = ADCL,

        .r_adcsrb = ADCSRB,
        .adts = { AVR_IO_REGBIT(ADCSRB, ADTS0), AVR_IO_REGBIT(ADCSRB, ADTS1), AVR_IO_REGBIT(ADCSRB, ADTS2), AVR_IO_REGBIT(ADCSRB, ADTS3) },
        .adts_op = {
            [0] = avr_adts_free_running,
            [1] = avr_adts_analog_comparator_0,
            [2] = avr_adts_external_interrupt_0,
            [3] = avr_adts_timer_0_compare_match_a,
            [4] = avr_adts_timer_0_overflow,
            [5] = avr_adts_timer_1_compare_match_b,
            [6] = avr_adts_timer_1_overflow,
            [7] = avr_adts_timer_1_capture_event,
            //[8] = avr_adts_timer_4_overflow,
            //[9] = avr_adts_timer_4_compare_match_a,
            //[10] = avr_adts_timer_4_compare_match_b,
            //[11] = avr_adts_timer_4_compare_match_d
        },

        .muxmode = {
            [0] = AVR_ADC_SINGLE(0), [1] = AVR_ADC_SINGLE(1),
            [2] = AVR_ADC_SINGLE(2), [3] = AVR_ADC_SINGLE(3),
            [4] = AVR_ADC_SINGLE(4), [5] = AVR_ADC_SINGLE(5),
            [6] = AVR_ADC_SINGLE(6), [7] = AVR_ADC_SINGLE(7),

            [14] = AVR_ADC_REF(1300),    // 1.30V
            [15] = AVR_ADC_REF(0),        // GND
        },

        .adc = {
            .enable = AVR_IO_REGBIT(ADCSRA, ADIE),
            .raised = AVR_IO_REGBIT(ADCSRA, ADIF),
            .vector = ADC_vect,
        },
    },
    .timer0 = {
        .name = '0',
        .wgm = { AVR_IO_REGBIT(TCCR0A, WGM00), AVR_IO_REGBIT(TCCR0A, WGM01), AVR_IO_REGBIT(TCCR0B, WGM02) },
        .wgm_op = {
            [0] = AVR_TIMER_WGM_NORMAL8(),
            // 1: PWM Phase Correct
            [2] = AVR_TIMER_WGM_CTC(),
            [3] = AVR_TIMER_WGM_FASTPWM8(),
            // 5: PWM Phase Correct
            [7] = AVR_TIMER_WGM_OCPWM(),
        },
        .cs = { AVR_IO_REGBIT(TCCR0B, CS00), AVR_IO_REGBIT(TCCR0B, CS01), AVR_IO_REGBIT(TCCR0B, CS02) },
        .cs_div = { 0, 0, 3 /* 8 */, 6 /* 64 */, 8 /* 256 */, 10 /* 1024 */},

        .r_tcnt = TCNT0,

        .overflow = {
            .enable = AVR_IO_REGBIT(TIMSK0, TOIE0),
            .raised = AVR_IO_REGBIT(TIFR0, TOV0),
            .vector = TIMER0_OVF_vect,
        },
        .comp = {
            [AVR_TIMER_COMPA] = {
                .r_ocr = OCR0A,
                .com = AVR_IO_REGBITS(TCCR0A, COM0A0, 0x3),
                .com_pin = AVR_IO_REGBIT(PORTB, PORTB7), // same as timer1C
                .interrupt = {
                    .enable = AVR_IO_REGBIT(TIMSK0, OCIE0A),
                    .raised = AVR_IO_REGBIT(TIFR0, OCF0A),
                    .vector = TIMER0_COMPA_vect,
                },
            },
            [AVR_TIMER_COMPB] = {
                .r_ocr = OCR0B,
                .com = AVR_IO_REGBITS(TCCR0B, COM0B0, 0x3),
                .com_pin = AVR_IO_REGBIT(PORTD, PORTD7),
                .interrupt = {
                    .enable = AVR_IO_REGBIT(TIMSK0, OCIE0B),
                    .raised = AVR_IO_REGBIT(TIFR0, OCF0B),
                    .vector = TIMER0_COMPB_vect,
                },
            },
        },
    },
    .timer1 = {
        .name = '1',
        .wgm = { AVR_IO_REGBIT(TCCR1A, WGM10), AVR_IO_REGBIT(TCCR1A, WGM11),
                    AVR_IO_REGBIT(TCCR1B, WGM12), AVR_IO_REGBIT(TCCR1B, WGM13) },
        .wgm_op = {
            [0] = AVR_TIMER_WGM_NORMAL16(),
            // TODO: 1 PWM phase correct 8bit
            //          2 PWM phase correct 9bit
            //       3 PWM phase correct 10bit
            [4] = AVR_TIMER_WGM_CTC(),
            [5] = AVR_TIMER_WGM_FASTPWM8(),
            [6] = AVR_TIMER_WGM_FASTPWM9(),
            [7] = AVR_TIMER_WGM_FASTPWM10(),
            // TODO: 8, 9 PWM phase and freq correct ICR & 10, 11
            [12] = AVR_TIMER_WGM_ICCTC(),
            [14] = AVR_TIMER_WGM_ICPWM(),
            [15] = AVR_TIMER_WGM_OCPWM(),
        },
        .cs = { AVR_IO_REGBIT(TCCR1B, CS10), AVR_IO_REGBIT(TCCR1B, CS11), AVR_IO_REGBIT(TCCR1B, CS12) },
        .cs_div = { 0, 0, 3 /* 8 */, 6 /* 64 */, 8 /* 256 */, 10 /* 1024 */  /* TODO: 2 External clocks */},

        .r_tcnt = TCNT1L,
        .r_icr = ICR1L,
        .r_icrh = ICR1H,
        .r_tcnth = TCNT1H,

        .ices = AVR_IO_REGBIT(TCCR1B, ICES1),
        //.icp = AVR_IO_REGBIT(ICP_PORT, ICP_PIN),

        .overflow = {
            .enable = AVR_IO_REGBIT(TIMSK1, TOIE1),
            .raised = AVR_IO_REGBIT(TIFR1, TOV1),
            .vector = TIMER1_OVF_vect,
        },
        /*.icr = {
            .enable = AVR_IO_REGBIT(TIMSK1, TICIE1),
            .raised = AVR_IO_REGBIT(TIFR1, ICF1),
            .vector = TIMER1_CAPT_vect,
        },*/
        .comp = {
            [AVR_TIMER_COMPA] = {
                .r_ocr = OCR1AL,
                .r_ocrh = OCR1AH,    // 16 bits timers have two bytes of it
                .com = AVR_IO_REGBITS(TCCR1A, COM1A0, 0x3),
                .com_pin = AVR_IO_REGBIT(PORTB, PORTB5),
                .interrupt = {
                    .enable = AVR_IO_REGBIT(TIMSK1, OCIE1A),
                    .raised = AVR_IO_REGBIT(TIFR1, OCF1A),
                    .vector = TIMER1_COMPA_vect,
                },
            },
            [AVR_TIMER_COMPB] = {
                .r_ocr = OCR1BL,
                .r_ocrh = OCR1BH,
                .com = AVR_IO_REGBITS(TCCR1A, COM1B0, 0x3),
                .com_pin = AVR_IO_REGBIT(PORTB, PORTB6),
                .interrupt = {
                    .enable = AVR_IO_REGBIT(TIMSK1, OCIE1B),
                    .raised = AVR_IO_REGBIT(TIFR1, OCF1B),
                    .vector = TIMER1_COMPB_vect,
                },
            },
            [AVR_TIMER_COMPC] = {
                .r_ocr = OCR1CL,
                .r_ocrh = OCR1CH,
                .com = AVR_IO_REGBITS(TCCR1A, COM1B0, 0x3),
                .com_pin = AVR_IO_REGBIT(PORTB, PORTB7),
                .interrupt = {
                    .enable = AVR_IO_REGBIT(TIMSK1, OCIE1C),
                    .raised = AVR_IO_REGBIT(TIFR1, OCF1C),
                    .vector = TIMER1_COMPC_vect,
                },
            },
        },

    },
    .timer3 = {
        .name = '3',
        .wgm = { AVR_IO_REGBIT(TCCR3A, WGM30), AVR_IO_REGBIT(TCCR3A, WGM31),
                    AVR_IO_REGBIT(TCCR3B, WGM32), AVR_IO_REGBIT(TCCR3B, WGM33) },
        .wgm_op = {
            [0] = AVR_TIMER_WGM_NORMAL16(),
            // TODO: 1 PWM phase correct 8bit
            //       2 PWM phase correct 9bit
            //       3 PWM phase correct 10bit
            [4] = AVR_TIMER_WGM_CTC(),
            [5] = AVR_TIMER_WGM_FASTPWM8(),
            [6] = AVR_TIMER_WGM_FASTPWM9(),
            [7] = AVR_TIMER_WGM_FASTPWM10(),
            // TODO: 8, 9 PWM phase and freq correct ICR & 10, 11
            [12] = AVR_TIMER_WGM_ICCTC(),
            [14] = AVR_TIMER_WGM_ICPWM(),
            [15] = AVR_TIMER_WGM_OCPWM(),
        },
        .cs = { AVR_IO_REGBIT(TCCR3B, CS30), AVR_IO_REGBIT(TCCR3B, CS31), AVR_IO_REGBIT(TCCR3B, CS32) },
        .cs_div = { 0, 0, 3 /* 8 */, 6 /* 64 */, 8 /* 256 */, 10 /* 1024 */  /* TODO: 2 External clocks */},

        .r_tcnt = TCNT3L,
        .r_icr = ICR3L,
        .r_icrh = ICR3H,
        .r_tcnth = TCNT3H,

        .ices = AVR_IO_REGBIT(TCCR3B, ICES3),
        //.icp = AVR_IO_REGBIT(ICP_PORT, ICP_PIN),

        .overflow = {
            .enable = AVR_IO_REGBIT(TIMSK3, TOIE3),
            .raised = AVR_IO_REGBIT(TIFR3, TOV3),
            .vector = TIMER3_OVF_vect,
        },
        .comp = {
            [AVR_TIMER_COMPA] = {
                .r_ocr = OCR3AL,
                .r_ocrh = OCR3AH,   // 16 bits timers have two bytes of it
                .com = AVR_IO_REGBITS(TCCR3A, COM3A0, 0x3),
                .com_pin = AVR_IO_REGBIT(PORTC, PORTC6),
                .interrupt = {
                    .enable = AVR_IO_REGBIT(TIMSK3, OCIE3A),
                    .raised = AVR_IO_REGBIT(TIFR3, OCF3A),
                    .vector = TIMER3_COMPA_vect,
                },
            },
            [AVR_TIMER_COMPB] = {
                .r_ocr = OCR3BL,
                .r_ocrh = OCR3BH,
                .com = AVR_IO_REGBITS(TCCR3A, COM3B0, 0x3),
                .interrupt = {
                    .enable = AVR_IO_REGBIT(TIMSK3, OCIE3B),
                    .raised = AVR_IO_REGBIT(TIFR3, OCF3B),
                    .vector = TIMER3_COMPB_vect,
                },
            },
            [AVR_TIMER_COMPC] = {
                .r_ocr = OCR3CL,
                .r_ocrh = OCR3CH,
                .com = AVR_IO_REGBITS(TCCR3A, COM3C0, 0x3),
                .interrupt = {
                    .enable = AVR_IO_REGBIT(TIMSK3, OCIE3C),
                    .raised = AVR_IO_REGBIT(TIFR3, OCF3C),
                    .vector = TIMER3_COMPC_vect,
                },
            },
        },
    },
    .timer4 = {
        .name = '4',
        .wgm = { AVR_IO_REGBIT(TCCR4D, WGM40), AVR_IO_REGBIT(TCCR4D, WGM41) },
        .wgm_op = {
            [0] = AVR_TIMER_WGM_NORMAL8()
            // 0: fast pwm, 01: phase/freq correct pwn, 10/11: pwm6
        },
        .cs = { AVR_IO_REGBIT(TCCR4B, CS40), AVR_IO_REGBIT(TCCR4B, CS41), AVR_IO_REGBIT(TCCR4B, CS42), AVR_IO_REGBIT(TCCR4B, CS43) },
        .cs_div = { 0, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14 },

        .r_tcnt = TCNT4,

        // asynchronous timer source bit.. if set, use 32khz frequency
        //.as2 = AVR_IO_REGBIT(ASSR, AS2),

        .overflow = {
            .enable = AVR_IO_REGBIT(TIMSK4, TOIE4),
            .raised = AVR_IO_REGBIT(TIFR4, TOV4),
            .vector = TIMER4_OVF_vect,
        },
        .comp = {
            [AVR_TIMER_COMPA] = {
                .r_ocr = OCR4A,
                .com = AVR_IO_REGBITS(TCCR4A, COM4A0, 0x3),
                .com_pin = AVR_IO_REGBIT(PORTC, PORTC7),
                .interrupt = {
                    .enable = AVR_IO_REGBIT(TIMSK4, OCIE4A),
                    .raised = AVR_IO_REGBIT(TIFR4, OCF4A),
                    .vector = TIMER4_COMPA_vect,
                },
            },
            [AVR_TIMER_COMPB] = {
                .r_ocr = OCR4B,
                .com = AVR_IO_REGBITS(TCCR4B, COM4B0, 0x3),
                .com_pin = AVR_IO_REGBIT(PORTB, PORTB6), // also PORTB7 in PWM mode.
                .interrupt = {
                    .enable = AVR_IO_REGBIT(TIMSK4, OCIE4B),
                    .raised = AVR_IO_REGBIT(TIFR4, OCF4B),
                    .vector = TIMER4_COMPB_vect,
                },
            },
            [AVR_TIMER_COMPD] = {
                .r_ocr = OCR4D,
                .com = AVR_IO_REGBITS(TCCR4D, COM4D0, 0x3),
                .com_pin = AVR_IO_REGBIT(PORTD, PORTD7), // also PORTD7 in PWM mode.
                .interrupt = {
                    .enable = AVR_IO_REGBIT(TIMSK4, OCIE4D),
                    .raised = AVR_IO_REGBIT(TIFR4, OCF4D),
                    .vector = TIMER4_COMPD_vect,
                },
            },
        },
    },
    AVR_SPI_DECLARE(0, 0),
    .twi = {

        .r_twcr = TWCR,
        .r_twsr = TWSR,
        .r_twbr = TWBR,
        .r_twdr = TWDR,
        .r_twar = TWAR,
        // no .r_twamr = TWAMR,

        .twen = AVR_IO_REGBIT(TWCR, TWEN),
        .twea = AVR_IO_REGBIT(TWCR, TWEA),
        .twsta = AVR_IO_REGBIT(TWCR, TWSTA),
        .twsto = AVR_IO_REGBIT(TWCR, TWSTO),
        .twwc = AVR_IO_REGBIT(TWCR, TWWC),

        .twsr = AVR_IO_REGBITS(TWSR, TWS3, 0x1f),    // 5 bits
        .twps = AVR_IO_REGBITS(TWSR, TWPS0, 0x3),    // 2 bits

        .twi = {
            .enable = AVR_IO_REGBIT(TWCR, TWIE),
            .raised = AVR_IO_REGBIT(TWCR, TWINT),
            .raise_sticky = 1,
            .vector = TWI_vect,
        },
    }
};

static avr_t *make()
{
    return avr_core_allocate(&mcu_mega32u4.core, sizeof(struct mcu_t));
}

avr_kind_t mega32u4 = {
    .names = { "atmega32u4" },
    .make = make
};

static void m32u4_init(struct avr_t * avr)
{
    struct mcu_t * mcu = (struct mcu_t*)avr;

    avr_clkpr_init(avr, &mcu->clkpr);
    avr_eeprom_init(avr, &mcu->eeprom);
    avr_flash_init(avr, &mcu->selfprog);
    avr_watchdog_init(avr, &mcu->watchdog);
    avr_extint_init(avr, &mcu->extint);
    avr_ioport_init(avr, &mcu->portb);
    avr_ioport_init(avr, &mcu->portc);
    avr_ioport_init(avr, &mcu->portd);
    avr_ioport_init(avr, &mcu->portf);
    avr_uart_init(avr, &mcu->uart);
    avr_adc_init(avr, &mcu->adc);
    avr_timer_init(avr, &mcu->timer0);
    avr_timer_init(avr, &mcu->timer1);
    avr_timer_init(avr, &mcu->timer3);
    avr_timer_init(avr, &mcu->timer4);
    avr_spi_init(avr, &mcu->spi);
    avr_twi_init(avr, &mcu->twi);
}

static void m32u4_reset(struct avr_t * avr)
{
//  struct mcu_t * mcu = (struct mcu_t*)avr;
}

