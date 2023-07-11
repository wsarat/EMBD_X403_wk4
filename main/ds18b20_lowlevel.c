#include <stdio.h>
#include <driver/gpio.h>
#include <unistd.h>
#include "rom/ets_sys.h"    // for ets_delay_us()
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

u_int8_t ds18b20_pin;

#define tRSTL   480
#define tRSTH   480
#define tPDIH   15  // min 15, max 60
#define tPDLOW  60  // min 60, max 240
#define tSLOT   60  // min 60, max 120
#define tREC    20   // min 1, max -
#define tRDV    5   // min -, max 15
#define tLOW0   60  // min 60, max 120
#define tLOW1   10   // min 1, max 15

// reset, check if sensor present
uint8_t ds18b20_reset() 
{
    gpio_set_level(ds18b20_pin, 0);
    ets_delay_us(tRSTL);
    gpio_set_level(ds18b20_pin, 1);
    //ets_delay_us(tPDIH); // skip delay, we are so late!!!

    int read = gpio_get_level(ds18b20_pin);
    if (read != 0) {
        // no 1-wite pulled down
        gpio_set_level(ds18b20_pin, 1);
        printf("Device not found!\n");

        return 1;
    }
    ets_delay_us(tRSTH);
    gpio_set_level(ds18b20_pin, 1);
    //printf("ds18b20 pull low %d => device present!\n", read);
    
    return 0;
}

void ds18b20_write_bit(bool bit) 
{
    gpio_set_level(ds18b20_pin, 0);
    //ets_delay_us(tLOW1);
    if (bit)
        gpio_set_level(ds18b20_pin, 1);
    ets_delay_us(tSLOT); 

    gpio_set_level(ds18b20_pin, 1);
    //ets_delay_us(tREC);
}

void ds18b20_write_byte(uint8_t byte)
{
    for (int i=0; i<8; i++) {
        ds18b20_write_bit((byte>>i)&0x1);
    }
}

bool ds18b20_read_bit(void) 
{
    bool bit = 0;

    gpio_set_level(ds18b20_pin, 0);
    //ets_delay_us(tRDV); // skip delay, we are so late!!!
    
    gpio_set_level(ds18b20_pin, 1);
    ets_delay_us(tRDV);

    bit = gpio_get_level(ds18b20_pin);
    ets_delay_us(tSLOT - tRDV);

    //gpio_set_level(ds18b20_pin, 1);

    return bit;
}

uint8_t ds18b20_read_byte(void)
{
    uint8_t byte = 0;
    for (int i=0; i<8; i++) {
        byte |= (ds18b20_read_bit()<<i);
    }    
    return byte;
}

void ds18b20_cmd_readrom(void)
{
    if (ds18b20_reset())
        return;

    ds18b20_write_byte(0x33);

    uint8_t byte[8];
    for (int i=0; i<8; i++) {
        byte[i] = ds18b20_read_byte();
    }

    printf("DS18B20 FAMILY: 0x%02X\n", byte[0]);
    printf("Serial Number: ");
    for (int i=1; i<7; i++)
        printf("%02X ", byte[i]);
    printf("\n");
    printf("CRC: 0x%02X\n", byte[7]);
}

void ds18b20_cmd_read_power_supply(void)
{
    if (ds18b20_reset())
        return;

    ds18b20_write_byte(0xCC);
    ds18b20_write_byte(0xB4);
    uint8_t byte = ds18b20_read_byte();

    if (byte & 0x1)
        printf("External Power.\n");
    else
        printf("Parasite Power.\n");
}

float ds18b20_cmd_read_temp(void)
{
    uint8_t temp_high_byte;
    uint8_t temp_low_byte;
    int32_t temperature = 0;;

    if (ds18b20_reset())
        return -127.0;

    // Start conversion
    ds18b20_write_byte(0xCC);
    ds18b20_write_byte(0x44);

    //temp_low_byte = ds18b20_read_byte();
    //printf("recv: %X\n", temp_low_byte);
    vTaskDelay(pdMS_TO_TICKS(1000));

    ds18b20_reset();
    // Read scratchpad
    ds18b20_write_byte(0xCC);
    ds18b20_write_byte(0xBE);

    temp_low_byte = ds18b20_read_byte();
    temp_high_byte = ds18b20_read_byte();
    
    temperature = temp_high_byte << 8;
    temperature |= temp_low_byte;

    //printf("Temp: %X %X\n", temp_high_byte, temp_low_byte);
    printf("Temp: %.02f C\n", (float)temperature/16);

    return (float)temperature/16;
}

void ds18b20_init(u_int8_t pin)
{
    ds18b20_pin = pin;
    gpio_reset_pin(ds18b20_pin);
    gpio_set_pull_mode(ds18b20_pin, GPIO_PULLUP_ONLY);
    gpio_set_direction(ds18b20_pin, GPIO_MODE_INPUT_OUTPUT_OD); //GPIO_MODE_OUTPUT

    gpio_set_level(ds18b20_pin, 1);
    ds18b20_reset();

    ds18b20_cmd_readrom();
    ds18b20_cmd_read_power_supply();
}