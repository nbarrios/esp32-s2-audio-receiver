#include "usb_audio_cb.h"
#include "tinyusb.h"
#include "esp_log.h"
#include "freertos/ringbuf.h"
#include "war_espnow.h"

static const char *TAG = "USB Audio";

const uint32_t sample_rates[] = {44100, 48000};
uint32_t current_sample_rate = 48000;

#define N_SAMPLE_RATES TU_ARRAY_SIZE(sample_rates)

/* Blink pattern
 * - 250 ms  : device not mounted
 * - 1000 ms : device mounted
 * - 2500 ms : device is suspended
 */
enum
{
    BLINK_STREAMING = 25,
    BLINK_NOT_MOUNTED = 250,
    BLINK_MOUNTED = 1000,
    BLINK_SUSPENDED = 2500,
};

enum
{
    VOLUME_CTRL_0_DB = 0,
    VOLUME_CTRL_10_DB = 2560,
    VOLUME_CTRL_20_DB = 5120,
    VOLUME_CTRL_30_DB = 7680,
    VOLUME_CTRL_40_DB = 10240,
    VOLUME_CTRL_50_DB = 12800,
    VOLUME_CTRL_60_DB = 15360,
    VOLUME_CTRL_70_DB = 17920,
    VOLUME_CTRL_80_DB = 20480,
    VOLUME_CTRL_90_DB = 23040,
    VOLUME_CTRL_100_DB = 25600,
    VOLUME_CTRL_SILENCE = 0x8000,
};

static uint32_t blink_interval_ms = BLINK_NOT_MOUNTED;

// Audio controls
// Current states
int8_t mute[CFG_TUD_AUDIO_FUNC_1_N_CHANNELS_TX + 1];    // +1 for master channel 0
int16_t volume[CFG_TUD_AUDIO_FUNC_1_N_CHANNELS_TX + 1]; // +1 for master channel 0

// Range states
audio_control_range_2_n_t(1) volumeRng[CFG_TUD_AUDIO_FUNC_1_N_CHANNELS_TX + 1]; // Volume range state

// Audio test data
int16_t  sine_buffer[SINE_SAMPLES];
uint16_t sine_index = 0;
uint16_t test_buffer_audio[CFG_TUD_AUDIO_FUNC_1_EP_SZ_IN / 2];
uint16_t startVal = 0;
RingbufHandle_t rbuf = NULL;

const size_t audio_ringbuffer_len = 48 * 40 * sizeof(int16_t);

tu_fifo_t* ep_in_fifo = NULL;

void init_usb_audio_ringbuffer() {
    rbuf = xRingbufferCreate(audio_ringbuffer_len, RINGBUF_TYPE_BYTEBUF);
    if (rbuf == NULL) {
        ESP_LOGE(TAG, "Failed to create ringbuffer");
        return;
    }
    espnow_set_rbuf(rbuf, audio_ringbuffer_len);
}

//--------------------------------------------------------------------+
// Device callbacks
//--------------------------------------------------------------------+

// Invoked when device is mounted
void tud_mount_cb(void)
{
    blink_interval_ms = BLINK_MOUNTED;
    ep_in_fifo = tud_audio_get_ep_in_ff();
}

// Invoked when device is unmounted
void tud_umount_cb(void)
{
    blink_interval_ms = BLINK_NOT_MOUNTED;
    ep_in_fifo = NULL;
}

// Invoked when usb bus is suspended
// remote_wakeup_en : if host allow us  to perform remote wakeup
// Within 7ms, device must draw an average of current less than 2.5 mA from bus
void tud_suspend_cb(bool remote_wakeup_en)
{
    (void)remote_wakeup_en;
    blink_interval_ms = BLINK_SUSPENDED;
}

// Invoked when usb bus is resumed
void tud_resume_cb(void)
{
    blink_interval_ms = BLINK_MOUNTED;
}

//--------------------------------------------------------------------+
// Application Callback API Implementations
//--------------------------------------------------------------------+

// Invoked when audio class specific set request received for an EP
bool tud_audio_set_req_ep_cb(uint8_t rhport, tusb_control_request_t const *p_request, uint8_t *pBuff)
{
    ESP_LOGV(TAG, "Audio Set Req EP Callback");
    (void)rhport;
    (void)pBuff;

    // We do not support any set range requests here, only current value requests
    TU_VERIFY(p_request->bRequest == AUDIO_CS_REQ_CUR);

    // Page 91 in UAC2 specification
    uint8_t channelNum = TU_U16_LOW(p_request->wValue);
    uint8_t ctrlSel = TU_U16_HIGH(p_request->wValue);
    uint8_t ep = TU_U16_LOW(p_request->wIndex);

    (void)channelNum;
    (void)ctrlSel;
    (void)ep;

    return false; // Yet not implemented
}

// Invoked when audio class specific set request received for an interface
bool tud_audio_set_itf_cb(uint8_t rhport, tusb_control_request_t const *p_request)
{
    (void)rhport;
    uint8_t const itf = tu_u16_low(tu_le16toh(p_request->wIndex));
    uint8_t const alt = tu_u16_low(tu_le16toh(p_request->wValue));

    ESP_LOGI(TAG, "Set interface %d alt %d\r\n", itf, alt);
    if (itf == 1 && alt == 1) {
        espnow_set_rbuf_state(ESPNOW_RBUF_ACTIVE);
    }

    return true;
}

// Helper for feature unit set requests
static bool tud_audio_feature_unit_set_request(uint8_t rhport, audio_control_request_t const *request, uint8_t const *buf)
{
    (void)rhport;

    TU_ASSERT(request->bEntityID == 0x02);
    TU_VERIFY(request->bRequest == AUDIO_CS_REQ_CUR);

    if (request->bControlSelector == AUDIO_FU_CTRL_MUTE)
    {
        TU_VERIFY(request->wLength == sizeof(audio_control_cur_1_t));

        mute[request->bChannelNumber] = ((audio_control_cur_1_t *)buf)->bCur;

        ESP_LOGV(TAG, "Set channel %d Mute: %d\r\n", request->bChannelNumber, mute[request->bChannelNumber]);

        return true;
    }
    else if (request->bControlSelector == AUDIO_FU_CTRL_VOLUME)
    {
        TU_VERIFY(request->wLength == sizeof(audio_control_cur_2_t));

        volume[request->bChannelNumber] = ((audio_control_cur_2_t const *)buf)->bCur;

        ESP_LOGV(TAG, "Set channel %d volume: %d dB\r\n", request->bChannelNumber, volume[request->bChannelNumber] / 256);

        return true;
    }
    else
    {
        ESP_LOGV(TAG, "Feature unit set request not supported, entity = %u, selector = %u, request = %u\r\n",
                 request->bEntityID, request->bControlSelector, request->bRequest);
        return false;
    }
}

// Invoked when audio class specific set request received for an entity
bool tud_audio_set_req_entity_cb(uint8_t rhport, tusb_control_request_t const *p_request, uint8_t *pBuff)
{
    ESP_LOGV(TAG, "Audio Set Req Entity Callback");
    audio_control_request_t const *request = (audio_control_request_t const *)p_request;

    // If request is for our feature unit
    if (request->bEntityID == 0x02)
        return tud_audio_feature_unit_set_request(rhport, request, pBuff);

    ESP_LOGV(TAG, "Set request not handled, entity = %d, selector = %d, request = %d\r\n",
             request->bEntityID, request->bControlSelector, request->bRequest);

    return false; // Yet not implemented
}

// Invoked when audio class specific get request received for an EP
bool tud_audio_get_req_ep_cb(uint8_t rhport, tusb_control_request_t const *p_request)
{
    ESP_LOGV(TAG, "Audio Get Req EP Callback");
    (void)rhport;

    // Page 91 in UAC2 specification
    uint8_t channelNum = TU_U16_LOW(p_request->wValue);
    uint8_t ctrlSel = TU_U16_HIGH(p_request->wValue);
    uint8_t ep = TU_U16_LOW(p_request->wIndex);

    (void)channelNum;
    (void)ctrlSel;
    (void)ep;

    //	return tud_control_xfer(rhport, p_request, &tmp, 1);

    return false; // Yet not implemented
}

// Invoked when audio class specific get request received for an interface
bool tud_audio_get_req_itf_cb(uint8_t rhport, tusb_control_request_t const *p_request)
{
    ESP_LOGV(TAG, "Audio Get Req ITF Callback");
    (void)rhport;

    // Page 91 in UAC2 specification
    uint8_t channelNum = TU_U16_LOW(p_request->wValue);
    uint8_t ctrlSel = TU_U16_HIGH(p_request->wValue);
    uint8_t itf = TU_U16_LOW(p_request->wIndex);

    (void)channelNum;
    (void)ctrlSel;
    (void)itf;

    return false; // Yet not implemented
}

// Helper for clock get requests
static bool tud_audio_clock_get_request(uint8_t rhport, audio_control_request_t const *request)
{
    TU_ASSERT(request->bEntityID == 0x04);
    ESP_LOGV(TAG, "Audio Clock Get Request");

    // Example supports only single frequency, same value will be used for current value and range
    if (request->bControlSelector == AUDIO_CS_CTRL_SAM_FREQ)
    {
        if (request->bRequest == AUDIO_CS_REQ_CUR)
        {
            ESP_LOGI(TAG, "Clock get current freq %u\r\n", current_sample_rate);

            audio_control_cur_4_t curf = {tu_htole32(current_sample_rate)};
            return tud_audio_buffer_and_schedule_control_xfer(rhport, (tusb_control_request_t const *)request, &curf, sizeof(curf));
        }
        else if (request->bRequest == AUDIO_CS_REQ_RANGE)
        {
            audio_control_range_4_n_t(N_SAMPLE_RATES) rangef =
            {
                    .wNumSubRanges = tu_htole16(N_SAMPLE_RATES)
            };
            TU_LOG1("Clock get %d freq ranges\r\n", N_SAMPLE_RATES);
            for (uint8_t i = 0; i < N_SAMPLE_RATES; i++)
            {
                rangef.subrange[i].bMin = sample_rates[i];
                rangef.subrange[i].bMax = sample_rates[i];
                rangef.subrange[i].bRes = 0;
                TU_LOG1("Range %d (%d, %d, %d)\r\n", i, (int)rangef.subrange[i].bMin, (int)rangef.subrange[i].bMax, (int)rangef.subrange[i].bRes);
            }

            return tud_audio_buffer_and_schedule_control_xfer(rhport, (tusb_control_request_t const *)request, &rangef, sizeof(rangef));
        }
    }
    else if (request->bControlSelector == AUDIO_CS_CTRL_CLK_VALID &&
             request->bRequest == AUDIO_CS_REQ_CUR)
    {
        audio_control_cur_1_t cur_valid = {.bCur = 1};
        ESP_LOGI(TAG, "Clock get is valid %u\r\n", cur_valid.bCur);
        return tud_audio_buffer_and_schedule_control_xfer(rhport, (tusb_control_request_t const *)request, &cur_valid, sizeof(cur_valid));
    }
    ESP_LOGI(TAG, "Clock get request not supported, entity = %u, selector = %u, request = %u\r\n",
             request->bEntityID, request->bControlSelector, request->bRequest);
    return false;
}

// Helper for feature unit get requests
static bool tud_audio_feature_unit_get_request(uint8_t rhport, audio_control_request_t const *request)
{
    TU_ASSERT(request->bEntityID == 0x02);
    ESP_LOGV(TAG, "Audio Get Feature Unit");

    if (request->bControlSelector == AUDIO_FU_CTRL_MUTE && request->bRequest == AUDIO_CS_REQ_CUR)
    {
        audio_control_cur_1_t mute1 = {.bCur = mute[request->bChannelNumber]};
        ESP_LOGI(TAG, "Get channel %u mute %d\r\n", request->bChannelNumber, mute1.bCur);
        return tud_audio_buffer_and_schedule_control_xfer(rhport, (tusb_control_request_t const *)request, &mute1, sizeof(mute1));
    }
    else if (0x02 && request->bControlSelector == AUDIO_FU_CTRL_VOLUME)
    {
        if (request->bRequest == AUDIO_CS_REQ_RANGE)
        {
            audio_control_range_2_n_t(1) range_vol = {
                .wNumSubRanges = tu_htole16(1),
                .subrange[0] = {.bMin = tu_htole16(-VOLUME_CTRL_50_DB), tu_htole16(VOLUME_CTRL_0_DB), tu_htole16(256)}};
            ESP_LOGI(TAG, "Get channel %u volume range (%d, %d, %u) dB\r\n", request->bChannelNumber,
                     range_vol.subrange[0].bMin / 256, range_vol.subrange[0].bMax / 256, range_vol.subrange[0].bRes / 256);
            return tud_audio_buffer_and_schedule_control_xfer(rhport, (tusb_control_request_t const *)request, &range_vol, sizeof(range_vol));
        }
        else if (request->bRequest == AUDIO_CS_REQ_CUR)
        {
            audio_control_cur_2_t cur_vol = {.bCur = tu_htole16(volume[request->bChannelNumber])};
            ESP_LOGI(TAG, "Get channel %u volume %u dB\r\n", request->bChannelNumber, cur_vol.bCur);
            return tud_audio_buffer_and_schedule_control_xfer(rhport, (tusb_control_request_t const *)request, &cur_vol, sizeof(cur_vol));
        }
    }
    TU_LOG1("Feature unit get request not supported, entity = %u, selector = %u, request = %u\r\n",
            request->bEntityID, request->bControlSelector, request->bRequest);

    return false;
}

// Invoked when audio class specific get request received for an entity
bool tud_audio_get_req_entity_cb(uint8_t rhport, tusb_control_request_t const *p_request)
{
    (void)rhport;

    ESP_LOGV(TAG, "Audio Get Req Entity Callback");
    audio_control_request_t *request = (audio_control_request_t *)p_request;

    // Page 91 in UAC2 specification
    uint8_t ctrlSel = TU_U16_HIGH(p_request->wValue);
    uint8_t entityID = TU_U16_HIGH(p_request->wIndex);

    // Input terminal (Microphone input)
    if (entityID == 1)
    {
        switch (ctrlSel)
        {
        case AUDIO_TE_CTRL_CONNECTOR:
        {
            // The terminal connector control only has a get request with only the CUR attribute.
            audio_desc_channel_cluster_t ret;

            // Those are dummy values for now
            ret.bNrChannels = 1;
            ret.bmChannelConfig = 0;
            ret.iChannelNames = 0;

            ESP_LOGV(TAG, "    Get terminal connector\r\n");

            return tud_audio_buffer_and_schedule_control_xfer(rhport, p_request, (void *)&ret, sizeof(ret));
        }
        break;
            // Unknown/Unsupported control selector
        default:
            TU_BREAKPOINT();
            return false;
        }
    }

    // Feature unit
    if (request->bEntityID == 0x02)
    {
        return tud_audio_feature_unit_get_request(rhport, request);
    }

    // Clock Source unit
    if (request->bEntityID == 0x04)
    {
        return tud_audio_clock_get_request(rhport, request);
    }

    ESP_LOGV(TAG, "  Unsupported entity: %d\r\n", entityID);
    return false; // Yet not implemented
}

bool tud_audio_set_itf_close_EP_cb(uint8_t rhport, tusb_control_request_t const *p_request)
{
    (void)rhport;

    uint8_t const itf = tu_u16_low(tu_le16toh(p_request->wIndex));
    uint8_t const alt = tu_u16_low(tu_le16toh(p_request->wValue));

    ESP_LOGV(TAG, "Audio Set ITF Close EP: %u, %u\n", itf, alt);

    espnow_set_rbuf_state(ESPNOW_RBUF_INACTIVE);
    if (rbuf) {
        size_t bytes_recv;
        void* data = xRingbufferReceiveUpTo(rbuf, &bytes_recv, 0, audio_ringbuffer_len);
        if (data) {
            vRingbufferReturnItem(rbuf, data);
        }
    }

    return true;
}

bool tud_audio_tx_done_pre_load_cb(uint8_t rhport, uint8_t itf, uint8_t ep_in, uint8_t cur_alt_setting)
{
    //ESP_LOGI(TAG, "Audio TX Done Preload");
    (void)rhport;
    (void)itf;
    (void)ep_in;
    (void)cur_alt_setting;

    if (ep_in_fifo) {
        size_t remaining = tu_fifo_remaining(ep_in_fifo);

        if (remaining > 0) {
            size_t bytes_recv;
            uint8_t* data = xRingbufferReceiveUpTo(rbuf, &bytes_recv, 0, remaining);
            if (data != NULL) {
                tud_audio_write(data, bytes_recv);
                vRingbufferReturnItem(rbuf, data);
            }
        }
    }

    return true;
}

bool tud_audio_tx_done_post_load_cb(uint8_t rhport, uint16_t n_bytes_copied, uint8_t itf, uint8_t ep_in, uint8_t cur_alt_setting)
{
    //ESP_LOGI(TAG, "Audio TX Done Postload");
    (void)rhport;
    (void)n_bytes_copied;
    (void)itf;
    (void)ep_in;
    (void)cur_alt_setting;

    return true;
}