#include "time.h"
#include "uart.h"

static volatile uint8_t a[2];

uint8_t dec2bcd(uint8_t d)
{
	return ((d/10 * 16) + (d % 10));
}

uint8_t bcd2dec(uint8_t b)
{
	return ((b/16 * 10) + (b % 16));
}

uint8_t rtc_read_byte(uint8_t offset)
{
	i2cMasterUploadBuf(offset);
	i2cMasterReceive(RTC_ADDR, 1);
	i2cMasterDownloadBufIndex((uint8_t*)&a[0] , 0);
	return a[0];
}

void rtc_write_byte(uint8_t b, uint8_t offset)
{
	i2cMasterUploadBuf(offset);
	i2cMasterUploadBuf(b);
	i2cMasterSendBuf(RTC_ADDR);
}

void rtc_init()
{
	uint8_t temp1 = rtc_read_byte(0x11);
	uint8_t temp2 = rtc_read_byte(0x12);
		
	rtc_write_byte(0xee, 0x11);
	rtc_write_byte(0xdd, 0x12);

	if (rtc_read_byte(0x11) == 0xee && rtc_read_byte(0x12) == 0xdd) 
	{
		// restore values
		rtc_write_byte(temp1, 0x11);
		rtc_write_byte(temp2, 0x12);
	}
}

void rtc_get_time_s(uint8_t* hour, uint8_t* min, uint8_t* sec)
{
	uint8_t rtc[9];

	// read 7 bytes starting from register 0
	// sec, min, hour, day-of-week, date, month, year
	i2cMasterUploadBuf(0);
	i2cMasterReceive(RTC_ADDR, 7);
	for(uint8_t i=0;i<7;i++)
	i2cMasterDownloadBufIndex((uint8_t*)&rtc[i] , i);
	
	*sec =  bcd2dec(rtc[0]);
	//UART_Send_Char(sec);
	*min =  bcd2dec(rtc[1]);
	*hour = bcd2dec(rtc[2]);
}

void rtc_set_time_s(uint8_t hour, uint8_t min, uint8_t sec)
{
	i2cMasterUploadBuf(0);

	// clock halt bit is 7th bit of seconds: this is always cleared to start the clock
	i2cMasterUploadBuf(dec2bcd(sec)); // seconds
	i2cMasterUploadBuf(dec2bcd(min)); // minutes
	i2cMasterUploadBuf(dec2bcd(hour)); // hours
	i2cMasterSendBuf(RTC_ADDR);
}