/* USB Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

// DESCRIPTION:
// This example contains minimal code to make ESP32-S2 based device
// recognizable by USB-host devices as a USB Serial Device.

#include <stdint.h>
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "tinyusb.h"
#include "sdkconfig.h"
#include "math.h"
#include "soc/usb_periph.h"
#include "usb_audio_cb.h"
#include "war_wifi.h"
#include "war_espnow.h"
#include "driver/timer.h"

static const char *TAG = "WAR Main";

//--------------------------------------------------------------------+
// MACRO CONSTANT TYPEDEF PROTYPES
//--------------------------------------------------------------------+

static TaskHandle_t xMainTaskNotify = NULL;

void IRAM_ATTR timer_group0_isr(void *para)
{
    timer_spinlock_take(TIMER_GROUP_0);

    timer_group_clr_intr_status_in_isr(TIMER_GROUP_0, TIMER_0);
    timer_group_enable_alarm_in_isr(TIMER_GROUP_0, TIMER_0);

    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    if (xMainTaskNotify)
        vTaskNotifyGiveFromISR(xMainTaskNotify, &xHigherPriorityTaskWoken);

    timer_spinlock_give(TIMER_GROUP_0);

    if (xHigherPriorityTaskWoken) portYIELD_FROM_ISR();
}

void main_timer_init()
{
    timer_config_t config;
    config.divider = 16,
    config.counter_dir = TIMER_COUNT_UP;
    config.counter_en = TIMER_PAUSE;
    config.alarm_en = TIMER_ALARM_EN;
    config.auto_reload = TIMER_AUTORELOAD_EN;
    config.intr_type = TIMER_INTR_LEVEL;

    timer_init(TIMER_GROUP_0, TIMER_0, &config);
    timer_set_counter_value(TIMER_GROUP_0, TIMER_0, 0x00000000ULL);

    const uint64_t alarm_value = (0.5f / 1000.f) * (TIMER_BASE_CLK / 16);
    timer_set_alarm_value(TIMER_GROUP_0, TIMER_0, alarm_value);
    timer_enable_intr(TIMER_GROUP_0, TIMER_0);
    timer_isr_register(TIMER_GROUP_0, TIMER_0, timer_group0_isr,
        (void*) TIMER_0, ESP_INTR_FLAG_IRAM, NULL);

    timer_start(TIMER_GROUP_0, TIMER_0);
}

void main_task(void *pvParamters)
{
    main_timer_init();

    xMainTaskNotify = xTaskGetCurrentTaskHandle();
    uint32_t notification_val;
    for (;;)
    {
        espnow_tick();
    }
    vTaskDelete(NULL);
}

void app_main(void)
{
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK( nvs_flash_erase() );
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK( ret );

    //Sine Wave 440HZ
    double delta = 1.0 / (double)current_sample_rate;
    double freq = 440.0;
    for (int i = 0; i < SINE_SAMPLES; i++)
    {
        double val = 0.02 * sin(2.0 * M_PI * freq * (double)i * delta);
        sine_buffer[i] = (int16_t) round(val * (double) INT16_MAX);
    }
    int16_t* backing_buffer = (int16_t*) malloc(1024 * sizeof(int16_t));
    memset(backing_buffer, 0, 1024 * sizeof(int16_t));
    rbuf = ringbuf_i16_init(backing_buffer, 1024);

    ESP_LOGI(TAG, "USB initialization");
    tinyusb_config_t tusb_cfg = {}; // the configuration using default values
    ESP_ERROR_CHECK(tinyusb_driver_install(&tusb_cfg));
    ESP_LOGI(TAG, "USB initialization DONE");

    war_wifi_init();
    ESP_ERROR_CHECK( espnow_init(true, rbuf) );

    //xTaskCreate(main_task, "Main", 4096, NULL, 4, NULL);
}