#FILE(GLOB HEADER_FILES simavr/cores/*.h simavr/sim/*.h)
#FILE(GLOB SOURCE_FILES simavr/cores/*.c simavr/sim/*.c)

set(HEADER_FILES 
    simavr/cores/sim_core_declare.h
    simavr/sim/avr/avr_mcu_section.h
    simavr/sim/avr_adc.h
    simavr/sim/avr_bitbang.h
    simavr/sim/avr_eeprom.h
    simavr/sim/avr_extint.h
    simavr/sim/avr_flash.h
    simavr/sim/avr_ioport.h
    simavr/sim/avr_lin.h
    simavr/sim/avr_spi.h
    simavr/sim/avr_timer.h
    simavr/sim/avr_twi.h
    simavr/sim/avr_uart.h
    simavr/sim/avr_usb.h
    simavr/sim/avr_watchdog.h
    simavr/sim/fifo_declare.h
    simavr/sim/sim_avr.h
    simavr/sim/sim_avr_types.h
    simavr/sim/sim_core.h
    simavr/sim/sim_cycle_timers.h
    simavr/sim/sim_elf.h
    simavr/sim/sim_gdb.h
    simavr/sim/sim_hex.h
    simavr/sim/sim_interrupts.h
    simavr/sim/sim_io.h
    simavr/sim/sim_irq.h
    simavr/sim/sim_network.h
    simavr/sim/sim_regbit.h
    simavr/sim/sim_time.h
    simavr/sim/sim_vcd_file.h
    simavr/sim_core_config.h
    simavr/sim_core_decl.h
) 

set(SOURCE_FILES 
    simavr/cores/sim_mega32u4.c
    simavr/sim/avr_adc.c
    simavr/sim/avr_bitbang.c
    simavr/sim/avr_eeprom.c
    simavr/sim/avr_extint.c
    simavr/sim/avr_flash.c
    simavr/sim/avr_ioport.c
    simavr/sim/avr_lin.c
    simavr/sim/avr_spi.c
    simavr/sim/avr_timer.c
    simavr/sim/avr_twi.c
    simavr/sim/avr_uart.c
    simavr/sim/avr_usb.c
    simavr/sim/avr_watchdog.c
    simavr/sim/run_avr.c
    simavr/sim/sim_avr.c
    simavr/sim/sim_core.c
    simavr/sim/sim_cycle_timers.c
    simavr/sim/sim_elf.c
    simavr/sim/sim_gdb.c
    simavr/sim/sim_hex.c
    simavr/sim/sim_interrupts.c
    simavr/sim/sim_io.c
    simavr/sim/sim_irq.c
    simavr/sim/sim_vcd_file.c
)

# can't build with elf support for web
set(EXTRA_INCLUDE_DIRS "")
set(EXTRA_LIBRARIES "")
if(NOT EMSCRIPTEN)
    find_package(ELF REQUIRED)
    list(APPEND EXTRA_INCLUDE_DIRS ${ELF_INCLUDE_DIR})
    list(APPEND EXTRA_LIBRARIES ${ELF_LIBRARY})
endif()

add_library(simavr ${HEADER_FILES} ${SOURCE_FILES})
target_include_directories(simavr PRIVATE ${EXTRA_INCLUDE_DIRS} simavr simavr/sim)
target_include_directories(simavr PUBLIC simavr/sim)
target_link_libraries(simavr ${EXTRA_LIBRARIES})

