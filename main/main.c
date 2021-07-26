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
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "tinyusb.h"
#include "sdkconfig.h"
#include "math.h"
#include "soc/usb_periph.h"
#include "usb_audio_cb.h"

#define CFG_TUSB_DEBUG 2

static const char *TAG = "example";

//--------------------------------------------------------------------+
// MACRO CONSTANT TYPEDEF PROTYPES
//--------------------------------------------------------------------+


void audio_task(void);

void main_task(void *pvParamters)
{
    for (;;)
    {
        audio_task();
        vTaskDelay(1);
    }
    vTaskDelete(NULL);
}

void app_main(void)
{
    //Sine Wave 440HZ
    double delta = 1.0 / (double)current_sample_rate;
    double freq = 440.0;
    for (int i = 0; i < SINE_SAMPLES; i++)
    {
        double val = 0.02 * sin(2.0 * M_PI * freq * (double)i * delta);
        sine_buffer[i] = (int16_t) round(val * (double) INT16_MAX);
    }
    ESP_LOGI(TAG, "Audio EP IN Max: %u", CFG_TUD_AUDIO_FUNC_1_EP_SZ_IN);

    ESP_LOGI(TAG, "USB initialization");
    tinyusb_config_t tusb_cfg = {}; // the configuration using default values
    ESP_ERROR_CHECK(tinyusb_driver_install(&tusb_cfg));

    ESP_LOGI(TAG, "USB initialization DONE");

    //xTaskCreate(main_task, "Main", 4096, NULL, 4, NULL);
}

//--------------------------------------------------------------------+
// AUDIO Task
//--------------------------------------------------------------------+

void audio_task(void)
{
/*     ESP_EARLY_LOGI(TAG, "EP XFER - IN EP%d - txFIFO#: %X, Transfer Size/Packets/FIFO Free: %u/%u/%u",
        1,
        (USB0.in_ep_reg[1].diepctl & USB_D_TXFNUM0_M) >> 22,
        (USB0.in_ep_reg[1].dieptsiz & USB_D_XFERSIZE0_M) >> USB_D_XFERSIZE0_S,
        (USB0.in_ep_reg[1].dieptsiz & USB_D_PKTCNT0_M) >> USB_D_PKTCNT0_S,
        (USB0.in_ep_reg[1].dtxfsts & USB_D_INEPTXFSPCAVAIL0_M) >> USB_D_INEPTXFSPCAVAIL0_S
    ); */
    vTaskDelay(pdMS_TO_TICKS(5000));
}