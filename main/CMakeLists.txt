set(SOURCES 
    "main.c"
    "war_wifi.c"
    "war_espnow.c" 
    "ringbuf_i16.c"
    "es8388_i2c.c"
    "war_i2s_audio.c"
)
if(CONFIG_USB_AUDIO_ENABLED)
set(SOURCES ${SOURCES} "usb_audio_cb.c")
endif()

idf_component_register(SRCS
    ${SOURCES}
    INCLUDE_DIRS .)
