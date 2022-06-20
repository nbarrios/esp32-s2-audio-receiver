#ifndef __WAR_I2S_AUDIO_H__
#define __WAR_I2S_AUDIO_H__

#include "freertos/FreeRTOS.h"

void war_i2s_audio_init();
void war_i2s_audio_task(void *pvParam);

#endif // __WAR_I2S_AUDIO_H__
