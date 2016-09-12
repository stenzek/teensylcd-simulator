SELF_DIR := $(dir $(lastword $(MAKEFILE_LIST)))

SRCFILES = \
    simavr/cores/sim_mega32u4.c \
    simavr/sim/avr_adc.c \
    simavr/sim/avr_clkpr.c \
    simavr/sim/avr_bitbang.c \
    simavr/sim/avr_eeprom.c \
    simavr/sim/avr_extint.c \
    simavr/sim/avr_flash.c \
    simavr/sim/avr_ioport.c \
    simavr/sim/avr_lin.c \
    simavr/sim/avr_spi.c \
    simavr/sim/avr_timer.c \
    simavr/sim/avr_twi.c \
    simavr/sim/avr_uart.c \
    simavr/sim/avr_usb.c \
    simavr/sim/avr_watchdog.c \
    simavr/sim/run_avr.c \
    simavr/sim/sim_avr.c \
    simavr/sim/sim_core.c \
    simavr/sim/sim_cycle_timers.c \
    simavr/sim/sim_elf.c \
    simavr/sim/sim_gdb.c \
    simavr/sim/sim_hex.c \
    simavr/sim/sim_interrupts.c \
    simavr/sim/sim_io.c \
    simavr/sim/sim_irq.c \
    simavr/sim/sim_vcd_file.c

print-%:
	@echo '$*=$($*)'

LIBNAME := libsimavr
INCLUDE := -I$(SELF_DIR)simavr -I$(SELF_DIR)simavr/sim

include ../Makefile.library

