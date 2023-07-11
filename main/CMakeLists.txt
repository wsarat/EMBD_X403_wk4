idf_component_register(
    SRCS "lcd1602_i2c.c" "ds18b20_lowlevel.c" "usb_hid.c" "servo.c" "main.c" 
    INCLUDE_DIRS "."
    REQUIRES driver esp_adc
    )
