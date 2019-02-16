#include <avr/interrupt.h>
#include <util/delay.h>
#include <avr/pgmspace.h>
#include "i2c.h"
#include "uart.h"

#define uint16_t unsigned int
#define uint8_t unsigned char

#define SEARCH_ITER 20

#define RDA5807M_I2C_ADDR_W 0x11
#define RDA5807M_I2C_ADDR_R 0x10
#define RDA5807M_REG_CONFIG 0x02
#define RDA5807M_FLG_DHIZ 0x8000
#define RDA5807M_FLG_DMUTE 0x4000
#define RDA5807M_FLG_BASS 0x1000
#define RDA5807M_FLG_SEEKUP 0x0200
#define RDA5807M_FLG_SEEK 0x0100
#define RDA5807M_FLG_SKMODE (uint16_t)0x0080
#define RDA5807M_FLG_RDS (uint16_t)0x0008
#define RDA5807M_FLG_NEW (uint16_t)0x0004
#define RDA5807M_REG_TUNING 0x03
#define RDA5807M_REG_RSSI 0x0B
#define RDA5807M_BAND_MASK (uint16_t)0x000C
#define RDA5807M_FLG_ENABLE (uint16_t)0x0001
#define RDA5807M_SPACE_MASK (uint16_t)0x0003
#define RDA5807M_REG_BLEND 0x07
#define RDA5807M_FLG_EASTBAND65M 0x0200
#define RDA5807M_BAND_SHIFT 2
#define RDA5807M_REG_STATUS 0x0A
#define RDA5807M_READCHAN_MASK 0x03FF
#define RDA5807M_CHAN_MASK 0xFFC0
#define RDA5807M_FLG_TUNE (uint16_t)0x0010
#define RDA5807M_CHAN_SHIFT 6
#define BAND_EAST 3
#define RDA5807M_RSSI_MASK 0xFE00
#define RDA5807M_RSSI_SHIFT 9

static volatile uint8_t temp=0x00,a[2],b[14];

struct max_station
{
	uint16_t freq;
	uint8_t rssi;
};

void FM_setRegister(uint8_t reg, uint16_t value);
uint16_t FM_getRegister(uint8_t reg);
void FM_updateRegister(uint8_t reg,uint16_t mask, uint16_t value);
void FM__init (void);
uint16_t FM_setTime(uint16_t old_time);
uint16_t FM_getBandAndSpacing(void);
uint8_t FM_setFrequency(uint16_t frequency);
uint16_t FM_getFrequency(void);
uint8_t FM_getRSSI(void);
uint16_t SEEK_FOR_RDS();
