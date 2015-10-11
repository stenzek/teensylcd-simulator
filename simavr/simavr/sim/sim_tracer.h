#ifndef __SIM_TRACER_H
#define __SIM_TRACER_H

typedef enum avr_tracer_event
{
    avr_tracer_event_ioport,        // p1 = ioport, p2 = bit, p3 = old value, p4 = new value
    avr_tracer_event_ddr,           // p1 = ioport, p2 = old value, p3 = new value
    avr_tracer_event_interrupt,     // p1 = interrupt
} avr_tracer_event;

typedef void(*avr_tracer_callback_t)(struct avr_t *avr, void *param, avr_tracer_event event, uint32_t p1, uint32_t p2, uint32_t p3, uint32_t p4);

#define AVR_TRACER_EVENT(avr, event, p1, p2, p3, p4) \
    do { \
        if (avr->tracer_callback) { \
            avr->tracer_callback(avr, avr->tracer_callback_param, (avr_tracer_event)event, (uint32_t)(p1), (uint32_t)(p2), (uint32_t)(p3), (uint32_t)(p4)); \
        } \
    } while (0)

#endif      // __SIM_TRACER_H

