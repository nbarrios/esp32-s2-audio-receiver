/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Ha Thach (tinyusb.org),
 * Additions Copyright (c) 2020, Espressif Systems (Shanghai) PTE LTD
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#pragma once
#include "tusb_option.h"
#include "sdkconfig.h"

#ifdef __cplusplus
extern "C" {
#endif

//   Calculate wMaxPacketSize of Endpoints
#define TUD_AUDIO_EP_SIZE(_maxFrequency, _nBytesPerSample, _nChannels) \
    ((((_maxFrequency + ((CFG_TUSB_RHPORT0_MODE & OPT_MODE_HIGH_SPEED) ? 7999 : 999)) / ((CFG_TUSB_RHPORT0_MODE & OPT_MODE_HIGH_SPEED) ? 8000 : 1000)) + 1) * _nBytesPerSample * _nChannels)

/*                      */
/* COMMON CONFIGURATION */
/*                      */

#define CFG_TUSB_RHPORT0_MODE       OPT_MODE_DEVICE
#define CFG_TUSB_OS                 OPT_OS_FREERTOS

/* USB DMA on some MCUs can only access a specific SRAM region with restriction on alignment.
 * Tinyusb use follows macros to declare transferring memory so that they can be put
 * into those specific section.
 * e.g
 * - CFG_TUSB_MEM SECTION : __attribute__ (( section(".usb_ram") ))
 * - CFG_TUSB_MEM_ALIGN   : __attribute__ ((aligned(4)))
 */
#ifndef CFG_TUSB_MEM_SECTION
#   define CFG_TUSB_MEM_SECTION
#endif

#ifndef CFG_TUSB_MEM_ALIGN
#   define CFG_TUSB_MEM_ALIGN          TU_ATTR_ALIGNED(4)
#endif

/*                      */
/* DRIVER CONFIGURATION */
/*                      */

#define CFG_TUD_MAINTASK_SIZE 4096
#define CFG_TUD_ENDOINT0_SIZE 64

// CDC FIFO size of TX and RX
#define CFG_TUD_CDC_RX_BUFSIZE CONFIG_USB_CDC_RX_BUFSIZE
#define CFG_TUD_CDC_TX_BUFSIZE CONFIG_USB_CDC_TX_BUFSIZE

// MSC Buffer size of Device Mass storage:
#define CFG_TUD_MSC_BUFSIZE CONFIG_USB_MSC_BUFSIZE

// HID buffer size Should be sufficient to hold ID (if any) + Data
#define CFG_TUD_HID_BUFSIZE CONFIG_USB_HID_BUFSIZE

#define CFG_TUD_CDC CONFIG_USB_CDC_ENABLED
#define CFG_TUD_MSC CONFIG_USB_MSC_ENABLED
#define CFG_TUD_HID CONFIG_USB_HID_ENABLED
#define CFG_TUD_MIDI CONFIG_USB_MIDI_ENABLED
#define CFG_TUD_AUDIO CONFIG_USB_AUDIO_ENABLED
#define CFG_TUD_CUSTOM_CLASS CONFIG_USB_CUSTOM_CLASS_ENABLED

//--------------------------------------------------------------------
// AUDIO CLASS DRIVER CONFIGURATION
//--------------------------------------------------------------------

// Have a look into audio_device.h for all configurations

#define CFG_TUD_AUDIO_FUNC_1_DESC_LEN                               TUD_AUDIO_MIC_ONE_CH_DESC_LEN

#define CFG_TUD_AUDIO_FUNC_1_MAX_SAMPLE_RATE                        48000     // 24bit/96kHz is the best quality for full-speed, high-speed is needed beyond this
#define CFG_TUD_AUDIO_FUNC_1_N_CHANNELS_TX                          1         // Driver gets this info from the descriptors - we define it here to use it to setup the descriptors and to do calculations with it below - be aware: for different number of channels you need another descriptor!

#define CFG_TUD_AUDIO_FUNC_1_N_BYTES_PER_SAMPLE_TX                  2                                       // Driver gets this info from the descriptors - we define it here to use it to setup the descriptors and to do calculations with it below
#define CFG_TUD_AUDIO_FUNC_1_FORMAT_1_RESOLUTION_TX                 16

#define CFG_TUD_AUDIO_ENABLE_EP_IN                                  1

#define CFG_TUD_AUDIO_FUNC_1_EP_SZ_IN                               TUD_AUDIO_EP_SIZE(CFG_TUD_AUDIO_FUNC_1_MAX_SAMPLE_RATE, CFG_TUD_AUDIO_FUNC_1_N_BYTES_PER_SAMPLE_TX, CFG_TUD_AUDIO_FUNC_1_N_CHANNELS_TX)

#define CFG_TUD_AUDIO_FUNC_1_EP_IN_SW_BUF_SZ                        CFG_TUD_AUDIO_FUNC_1_EP_SZ_IN
#define CFG_TUD_AUDIO_FUNC_1_EP_IN_SZ_MAX                           CFG_TUD_AUDIO_FUNC_1_EP_SZ_IN                  // Maximum EP IN size for all AS alternate settings used

#define CFG_TUD_AUDIO_FUNC_1_N_AS_INT                               2                                       // Number of Standard AS Interface Descriptors (4.9.1) defined per audio function - this is required to be able to remember the current alternate settings of these interfaces - We restrict us here to have a constant number for all audio functions (which means this has to be the maximum number of AS interfaces an audio function has and a second audio function with less AS interfaces just wastes a few bytes)

#define CFG_TUD_AUDIO_FUNC_1_CTRL_BUF_SZ                            64                                      // Size of control request buffer

/*         */
/* KCONFIG */
/*         */

#ifndef CONFIG_USB_CDC_ENABLED
#   define CONFIG_USB_CDC_ENABLED 0
#endif

#ifndef CONFIG_USB_MSC_ENABLED
#   define CONFIG_USB_MSC_ENABLED 0
#endif

#ifndef CONFIG_USB_HID_ENABLED
#   define CONFIG_USB_HID_ENABLED 0
#endif

#ifndef CONFIG_USB_MIDI_ENABLED
#   define CONFIG_USB_MIDI_ENABLED 0
#endif

#ifndef CONFIG_USB_AUDIO_ENABLED
#   define CONFIG_USB_AUDIO_ENABLED 0
#endif

#ifndef CONFIG_USB_CUSTOM_CLASS_ENABLED
#   define CONFIG_USB_CUSTOM_CLASS_ENABLED 0
#endif

#ifdef __cplusplus
}
#endif
