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

    ESP_LOGI(TAG, "USB initialization");
    tinyusb_config_t tusb_cfg = {}; // the configuration using default values
    ESP_ERROR_CHECK(tinyusb_driver_install(&tusb_cfg));
    ESP_LOGI(TAG, "USB initialization DONE");

    init_usb_audio_ringbuffer();
    war_wifi_init();
    ESP_ERROR_CHECK( espnow_init(true) );
}