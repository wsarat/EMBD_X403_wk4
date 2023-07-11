#include <stdlib.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/adc.h"
#include "esp_adc_cal.h"
#include <string.h>

#include "usb_hid.h"
#include "servo.h"
#include "lcd1602_i2c.h"
#include "ds18b20_lowlevel.h"

static esp_adc_cal_characteristics_t adc1_chars;

void joystick_init() {
    // ADC for joystick
    esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_11, ADC_WIDTH_BIT_DEFAULT, 0, &adc1_chars);
    (adc1_config_width(ADC_WIDTH_BIT_DEFAULT));
    (adc1_config_channel_atten(ADC1_CHANNEL_8, ADC_ATTEN_DB_11));
    (adc1_config_channel_atten(ADC1_CHANNEL_9, ADC_ATTEN_DB_11));

    // GPIO for joystick click
    const gpio_config_t boot_button_config = {
        .pin_bit_mask = BIT64(GPIO_NUM_11),
        .mode = GPIO_MODE_INPUT,
        .intr_type = GPIO_INTR_ANYEDGE,
        .pull_up_en = true,
        .pull_down_en = false,
    };
    ESP_ERROR_CHECK(gpio_config(&boot_button_config));
}

TaskHandle_t joystickTaskHandle = NULL;

void joystickTask(void *arg)
{
    uint32_t volt_8;
    uint32_t volt_9;
    int8_t delta_x = 0;
    int8_t delta_y = 0;

    int click = 0;
    int angle = 0;

    while(1){
        volt_8 = esp_adc_cal_raw_to_voltage(adc1_get_raw(ADC1_CHANNEL_8), &adc1_chars);
        volt_9 = esp_adc_cal_raw_to_voltage(adc1_get_raw(ADC1_CHANNEL_9), &adc1_chars);
        click = 0x0; //gpio_get_level(GPIO_NUM_11);

        delta_x = volt_9/100-16;
        delta_y = volt_8/100-16-1; // offset 1 fix drift
        angle = delta_x * (90/16);
        if (usb_mounted()) {
            usb_mouse_report(click, delta_x, delta_y);
        } 
        //printf("volt9: %d, delta_x: %d, angle: %d\n", (int)volt_9, delta_x, angle);
        servo_set(angle);

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void app_main(void)
{
    i2c_init();
    lcd_init();
    ds18b20_init(GPIO_NUM_12);

    servo_init();
    usb_init();
    joystick_init();

    scan_i2c();

    char chBuf[17];

    lcd_ser_curser(0,0);
    snprintf(chBuf, 17, "i2c -> LCD1602");
    lcd_send_buf(chBuf, strlen(chBuf));

    float tempC = -127;

    xTaskCreate(joystickTask, "Joystick_Task", 4096, NULL, 10, &joystickTaskHandle);

    while (1) {
        tempC = ds18b20_cmd_read_temp();
        if (tempC > -127) {
                lcd_ser_curser(1,0);
                snprintf(chBuf, 17, "DS18B20: %.02f C", tempC);
                lcd_send_buf(chBuf, strlen(chBuf));
        }

        vTaskDelay(pdMS_TO_TICKS(1500));
    }
}
