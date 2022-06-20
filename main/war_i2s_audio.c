#include "war_i2s_audio.h"
#include "driver/i2s.h"
#include "freertos/ringbuf.h"
#include "war_espnow.h"
#include "esp_log.h"
#include "math.h"

static RingbufHandle_t rbuf;

#define SINE_SAMPLES    109
int16_t sine_buffer[SINE_SAMPLES];
uint16_t sine_index = 0;

void war_i2s_audio_init()
{
    //I2S Periph Config
    i2s_config_t i2s_num0_config = {
        .mode = (i2s_mode_t) (I2S_MODE_MASTER | I2S_MODE_TX),
        .sample_rate = 48000,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags = 1,
        .dma_buf_count = 4,
        .dma_buf_len = 240,
        .use_apll = true,
        .tx_desc_auto_clear = true,
        .fixed_mclk = 0,
    };

    i2s_driver_install(I2S_NUM_0, &i2s_num0_config, 0, NULL);
    i2s_pin_config_t pin_config = {
        .bck_io_num = 27,
        .ws_io_num = 25,
        .data_out_num = 26,
        .data_in_num = 35
    };
    i2s_set_pin(I2S_NUM_0, &pin_config);

    //MCLK Output
    WRITE_PERI_REG(PIN_CTRL, READ_PERI_REG(PIN_CTRL)&0xFFFFFFF0);
    PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO0_U, FUNC_GPIO0_CLK_OUT1);

    //Sine Wave 440HZ
    double delta = 1.0 / (double)48000.0;
    double freq = 440.0;
    for (int i = 0; i < SINE_SAMPLES; i++)
    {
        double val = 0.02 * sin(2.0 * M_PI * freq * (double)i * delta);
        sine_buffer[i] = (int16_t) round(val * (double) INT16_MAX);
    }

    //Ringbuffer
    size_t len = 48 * 10 * sizeof(int16_t);
    rbuf = xRingbufferCreate(len, RINGBUF_TYPE_BYTEBUF);
    espnow_set_rbuf(rbuf, len);
    espnow_set_rbuf_state(ESPNOW_RBUF_ACTIVE);

    xTaskCreatePinnedToCore(war_i2s_audio_task, "WAR I2S Audio", 2048, NULL, 4,
        NULL, 1);
}

void war_i2s_audio_task(void *pvParam)
{
    const bool sine_test = true;
    size_t bytes_rcvd;
    size_t bytes_written;
    int16_t* i2s_buffer = malloc(48 * 2 * sizeof(int16_t) * 2);
    ESP_LOGI("I2S", "Audio Task Started");
    for (;;) {
        int16_t* buf = xRingbufferReceiveUpTo(rbuf, &bytes_rcvd, portMAX_DELAY,
            48 * 2 * sizeof(int16_t));
        if (buf != NULL) {
            for (int i = 0; i < bytes_rcvd/sizeof(int16_t); i++) {
                i2s_buffer[i*2] = buf[i]; 
                i2s_buffer[i*2+1] = buf[i]; 
            }
            esp_err_t err = i2s_write(I2S_NUM_0, i2s_buffer, bytes_rcvd*sizeof(int16_t), &bytes_written, portMAX_DELAY);
            ESP_ERROR_CHECK_WITHOUT_ABORT(err);
            vRingbufferReturnItem(rbuf, buf);
         } else {
            ESP_LOGI("I2S", "Received Audio Data");
        }
    }
    vTaskDelete(NULL);
}