// Copyright 2020 Espressif Systems (Shanghai) Co. Ltd.
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

#ifdef __cplusplus
extern "C" {
#endif

#define USB_ESPRESSIF_VID 0x303A
#define USB_STRING_DESCRIPTOR_ARRAY_SIZE 9

typedef enum{
    TINYUSB_USBDEV_0,
} tinyusb_usbdev_t;

typedef char *tusb_desc_strarray_device_t[USB_STRING_DESCRIPTOR_ARRAY_SIZE];

#ifdef __cplusplus
}
#endif
