#ifndef __USB_AUDIO_CB_H__
#define __USB_AUDIO_CB_H__
#include <stdint.h>
#include "ringbuf_i16.h"

extern const uint32_t sample_rates[];
extern uint32_t current_sample_rate;

#define SINE_SAMPLES    109
extern int16_t sine_buffer[];
extern ringbuf_i16_handle_t rbuf;

#endif // __USB_AUDIO_CB_H__
