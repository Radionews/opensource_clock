#include <avr/interrupt.h>
#include <util/delay.h>

#include "i2c.h"

#define RTC_ADDR 0x68 // I2C address
#define CH_BIT 7 // clock halt bit

uint8_t dec2bcd(uint8_t d);
uint8_t bcd2dec(uint8_t b);

uint8_t rtc_read_byte(uint8_t offset);
void rtc_write_byte(uint8_t b, uint8_t offset);
void rtc_init();
void rtc_get_time_s(uint8_t* hour, uint8_t* min, uint8_t* sec);
void rtc_set_time_s(uint8_t hour, uint8_t min, uint8_t sec);