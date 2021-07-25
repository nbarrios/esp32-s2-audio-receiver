// Copyright 2020 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#include "tusb.h"
#include "tinyusb_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define _PID_MAP(itf, n) ((CFG_TUD_##itf) << (n))

//------------- Configuration Descriptor -------------//
enum {
#   if CFG_TUD_CDC
    ITF_NUM_CDC = 0,
    ITF_NUM_CDC_DATA,
#   endif

#   if CFG_TUD_MSC
    ITF_NUM_MSC,
#   endif

#   if CFG_TUD_HID
    ITF_NUM_HID,
#   endif

#   if CFG_TUD_AUDIO
    ITF_NUM_AUDIO_CONTROL,
    ITF_NUM_AUDIO_STREAMING,
#   endif

    ITF_NUM_TOTAL
};

#define EPNUM_AUDIO   0x01

#define TUSB_AUDIO_MIC_ENTITY_CLOCK             0x04
#define TUSB_AUDIO_MIC_ENTITY_FEATURE_UNIT      0x02
#define TUSB_AUDIO_MIC_ENTITY_INPUT_TERMINAL    0x11
#define TUSB_AUDIO_MIC_ENTITY_OUTPUT_TERMINAL   0x13


// AUDIO simple descriptor (UAC2) for 1 microphone input
// - 1 Input Terminal, 1 Feature Unit (Mute and Volume Control), 1 Output Terminal, 1 Clock Source

#define TUSB_AUDIO_MIC_ONE_CH_DESC_LEN (TUD_AUDIO_DESC_IAD_LEN\
  + TUD_AUDIO_DESC_STD_AC_LEN\
  + TUD_AUDIO_DESC_CS_AC_LEN\
  + TUD_AUDIO_DESC_CLK_SRC_LEN\
  + TUD_AUDIO_DESC_INPUT_TERM_LEN\
  + TUD_AUDIO_DESC_OUTPUT_TERM_LEN\
  + TUD_AUDIO_DESC_FEATURE_UNIT_ONE_CHANNEL_LEN\
  + TUD_AUDIO_DESC_STD_AS_INT_LEN\
  + TUD_AUDIO_DESC_STD_AS_INT_LEN\
  + TUD_AUDIO_DESC_CS_AS_INT_LEN\
  + TUD_AUDIO_DESC_TYPE_I_FORMAT_LEN\
  + TUD_AUDIO_DESC_STD_AS_ISO_EP_LEN\
  + TUD_AUDIO_DESC_CS_AS_ISO_EP_LEN)

#define TUSB_AUDIO_MIC_ONE_CH_DESC_N_AS_INT 1 	// Number of AS interfaces

#define TUSB_AUDIO_MIC_ONE_CH_DESCRIPTOR(_itfnum, _stridx, _nBytesPerSample, _nBitsUsedPerSample, _epin, _epsize) \
  /* Standard Interface Association Descriptor (IAD) */\
  TUD_AUDIO_DESC_IAD(/*_firstitfs*/ (uint8_t)ITF_NUM_AUDIO_CONTROL, /*_nitfs*/ ITF_NUM_TOTAL, /*_stridx*/ 0x00),\
  /* Standard AC Interface Descriptor(4.7.1) */\
  TUD_AUDIO_DESC_STD_AC(/*_itfnum*/ (uint8_t)ITF_NUM_AUDIO_CONTROL, /*_nEPs*/ 0x00, /*_stridx*/ _stridx),\
  /* Class-Specific AC Interface Header Descriptor(4.7.2) */\
  TUD_AUDIO_DESC_CS_AC(/*_bcdADC*/ 0x0200, /*_category*/ AUDIO_FUNC_MICROPHONE, /*_totallen*/ TUD_AUDIO_DESC_CLK_SRC_LEN+TUD_AUDIO_DESC_INPUT_TERM_LEN+TUD_AUDIO_DESC_OUTPUT_TERM_LEN+TUD_AUDIO_DESC_FEATURE_UNIT_ONE_CHANNEL_LEN, /*_ctrl*/ AUDIO_CS_AS_INTERFACE_CTRL_LATENCY_POS),\
  /* Clock Source Descriptor(4.7.2.1) */\
  TUD_AUDIO_DESC_CLK_SRC(/*_clkid*/ TUSB_AUDIO_MIC_ENTITY_CLOCK, /*_attr*/ AUDIO_CLOCK_SOURCE_ATT_INT_FIX_CLK, /*_ctrl*/ (AUDIO_CTRL_R << AUDIO_CLOCK_SOURCE_CTRL_CLK_FRQ_POS), /*_assocTerm*/ 0x00,  /*_stridx*/ 0x00),\
  /* Input Terminal Descriptor(4.7.2.4) */\
  TUD_AUDIO_DESC_INPUT_TERM(/*_termid*/ TUSB_AUDIO_MIC_ENTITY_INPUT_TERMINAL, /*_termtype*/ AUDIO_TERM_TYPE_IN_GENERIC_MIC, /*_assocTerm*/ 0x00, /*_clkid*/ TUSB_AUDIO_MIC_ENTITY_CLOCK, /*_nchannelslogical*/ 0x01, /*_channelcfg*/ AUDIO_CHANNEL_CONFIG_NON_PREDEFINED, /*_idxchannelnames*/ 0x00, /*_ctrl*/ 0 * (AUDIO_CTRL_R << AUDIO_IN_TERM_CTRL_CONNECTOR_POS), /*_stridx*/ 0x00),\
  /* Output Terminal Descriptor(4.7.2.5) */\
  TUD_AUDIO_DESC_OUTPUT_TERM(/*_termid*/ TUSB_AUDIO_MIC_ENTITY_OUTPUT_TERMINAL, /*_termtype*/ AUDIO_TERM_TYPE_USB_STREAMING, /*_assocTerm*/ 0x00, /*_srcid*/ TUSB_AUDIO_MIC_ENTITY_INPUT_TERMINAL, /*_clkid*/ TUSB_AUDIO_MIC_ENTITY_CLOCK, /*_ctrl*/ 0x0000, /*_stridx*/ 0x00),\
  /* Feature Unit Descriptor(4.7.2.8) */\
  TUD_AUDIO_DESC_FEATURE_UNIT_ONE_CHANNEL(/*_unitid*/ TUSB_AUDIO_MIC_ENTITY_FEATURE_UNIT, /*_srcid*/ TUSB_AUDIO_MIC_ENTITY_INPUT_TERMINAL, /*_ctrlch0master*/ AUDIO_CTRL_RW << AUDIO_FEATURE_UNIT_CTRL_MUTE_POS | AUDIO_CTRL_RW << AUDIO_FEATURE_UNIT_CTRL_VOLUME_POS, /*_ctrlch1*/ AUDIO_CTRL_RW << AUDIO_FEATURE_UNIT_CTRL_MUTE_POS | AUDIO_CTRL_RW << AUDIO_FEATURE_UNIT_CTRL_VOLUME_POS, /*_stridx*/ 0x00),\
  /* Standard AS Interface Descriptor(4.9.1) */\
  /* Interface 1, Alternate 0 - default alternate setting with 0 bandwidth */\
  TUD_AUDIO_DESC_STD_AS_INT(/*_itfnum*/ (uint8_t)ITF_NUM_AUDIO_STREAMING, /*_altset*/ 0x00, /*_nEPs*/ 0x00, /*_stridx*/ 0x00),\
  /* Standard AS Interface Descriptor(4.9.1) */\
  /* Interface 1, Alternate 1 - alternate interface for data streaming */\
  TUD_AUDIO_DESC_STD_AS_INT(/*_itfnum*/ (uint8_t)ITF_NUM_AUDIO_STREAMING, /*_altset*/ 0x01, /*_nEPs*/ 0x01, /*_stridx*/ 0x00),\
  /* Class-Specific AS Interface Descriptor(4.9.2) */\
  TUD_AUDIO_DESC_CS_AS_INT(/*_termid*/ TUSB_AUDIO_MIC_ENTITY_OUTPUT_TERMINAL, /*_ctrl*/ AUDIO_CTRL_NONE, /*_formattype*/ AUDIO_FORMAT_TYPE_I, /*_formats*/ AUDIO_DATA_FORMAT_TYPE_I_PCM, /*_nchannelsphysical*/ 0x01, /*_channelcfg*/ AUDIO_CHANNEL_CONFIG_NON_PREDEFINED, /*_stridx*/ 0x00),\
  /* Type I Format Type Descriptor(2.3.1.6 - Audio Formats) */\
  TUD_AUDIO_DESC_TYPE_I_FORMAT(_nBytesPerSample, _nBitsUsedPerSample),\
  /* Standard AS Isochronous Audio Data Endpoint Descriptor(4.10.1.1) */\
  TUD_AUDIO_DESC_STD_AS_ISO_EP(/*_ep*/ _epin, /*_attr*/ (TUSB_XFER_ISOCHRONOUS | TUSB_ISO_EP_ATT_ASYNCHRONOUS | TUSB_ISO_EP_ATT_DATA), /*_maxEPsize*/ _epsize, /*_interval*/ 0x01),\
  /* Class-Specific AS Isochronous Audio Data Endpoint Descriptor(4.10.1.2) */\
  TUD_AUDIO_DESC_CS_AS_ISO_EP(/*_attr*/ AUDIO_CS_AS_ISO_DATA_EP_ATT_NON_MAX_PACKETS_OK, /*_ctrl*/ AUDIO_CTRL_NONE, /*_lockdelayunit*/ AUDIO_CS_AS_ISO_DATA_EP_LOCK_DELAY_UNIT_UNDEFINED, /*_lockdelay*/ 0x0000)

extern tusb_desc_device_t descriptor_tinyusb;
extern tusb_desc_strarray_device_t descriptor_str_tinyusb;

extern tusb_desc_device_t descriptor_kconfig;
extern tusb_desc_strarray_device_t descriptor_str_kconfig;

#ifdef __cplusplus
}
#endif
