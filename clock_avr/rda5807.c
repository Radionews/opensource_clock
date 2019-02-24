#include "rda5807.h"

const int RDA5807M_BandLowerLimits[5] PROGMEM = {8700, 7600, 7600, 6500, 5000};
const int RDA5807M_BandHigherLimits[5] PROGMEM = {10800, 9100, 10800, 7600,	6500};
const int RDA5807M_ChannelSpacings[4] PROGMEM = {100, 200, 50, 25};
	
void FM_setRegister(uint8_t reg, uint16_t value) {
	i2cMasterUploadBuf(reg);
	i2cMasterUploadBuf(value>>8);
	reg = (0x00FF & value);
	i2cMasterUploadBuf(reg);
	i2cMasterSendBuf(RDA5807M_I2C_ADDR_W);
}

uint16_t FM_getRegister(uint8_t reg) {
	uint16_t result;
	i2cMasterUploadBuf(reg);
	i2cMasterReceive(RDA5807M_I2C_ADDR_W, 2);
	for(uint8_t i=0;i<2;i++)
	i2cMasterDownloadBufIndex((uint8_t*)&a[i] , i);
	result = (uint16_t)a[0] << 8;
	result |= a[1];
	return result;
}

void FM_updateRegister(uint8_t reg,uint16_t mask, uint16_t value) {
	FM_setRegister(reg, FM_getRegister(reg) & ~mask | value);
}

void FM__init (void) {
	FM_setRegister(RDA5807M_REG_CONFIG, RDA5807M_FLG_DHIZ | RDA5807M_FLG_DMUTE | RDA5807M_FLG_BASS | RDA5807M_FLG_SEEKUP | RDA5807M_FLG_RDS | RDA5807M_FLG_NEW | RDA5807M_FLG_ENABLE);
}


uint16_t FM_setTime(uint16_t old_time) {
	
	uint16_t time_rds[3];
	unsigned int RDS[6];
	char grp, ver;
	uint16_t offset = 0, mins = 0;
	uint16_t bad_paket = 0;
	
	for(int h=0;h<3;h++)
	{
		i2cMasterUploadBuf(0x0A);
		i2cMasterReceive(RDA5807M_I2C_ADDR_R, 12);
		for(uint8_t i=0;i<12;i++)
		i2cMasterDownloadBufIndex((uint8_t*)&b[i] , i);
		
		for (uint8_t i = 0; i < 6; i++)
		{
			RDS[i] = 256 * b[i*2] + b[i*2+1];
		}
		uint16_t block1 = RDS[2];
		uint16_t block2 = RDS[3];
		uint16_t block3 = RDS[4];
		uint16_t block4 = RDS[5];	
		uint16_t rdsready = RDS[0] & 0x8000;
		uint16_t rdssynchro = RDS[0] & 0x1000;
		uint16_t rdsblockerror = RDS[1] & 0x000C;
		
		uint16_t rdsGroupType = 0x0A | ((block2 & 0xF000) >> 8) | ((block2 & 0x0800) >> 11);
		
		if (rdssynchro != 0x1000){  // RDS not synchronised or tuning changed, reset all the RDS info.
			
			rdsGroupType = 0;
			mins = 0;
		}
		
		switch (rdsGroupType) {
			case 0x4A:
			// время и дата
			if(rdsblockerror<3)
			{ // limit RDS data errors as we have no correction code
				offset = (block4) & 0x003F; // 6 bits
				mins = (block4 >> 6) & 0x003F; // 6 bits
				mins += 60 * (((block3 & 0x0001) << 4) | ((block4 >> 12) & 0x000F));
				
				if (offset & 0x20)//знак сдвига часового пояса, свиг задается получасами
				{
					mins -= 30 * (offset & 0x1F);
				}
				else
				{
					mins += 30 * (offset & 0x1F);
				}
				
				if(!((mins>480) & (mins<=1440)))
				{
					mins -= 1440;
				}
			}

			if(old_time!=mins)
			{
				UART_Send_Str("QTime: ");
				UART_Send_Char((mins/60)/10+0x30);
				UART_Send_Char((mins/60)%10+0x30);
				UART_Send_Str(":");
				UART_Send_Char((mins%60)/10+0x30);
				UART_Send_Char((mins%60)%10+0x30);
				UART_Send_Char(13);
				time_rds[h] = mins;
				old_time=mins;
				
				if((h == 2)&((((int)(time_rds[1] - time_rds[0]) == 1)&((int)(time_rds[2] - time_rds[1]) == 1))))
				{
					bad_paket = 0;
					return mins;
				}
				else
				{
					if(h==2)
					{
						time_rds[0]=0;
						time_rds[1]=0;
						time_rds[2]=0;
						h=0;
						bad_paket++;
						if(bad_paket == 10)
						{
							bad_paket = 0;
							return 3333;	
						}
							
					}
				}
			}
			else
				h--;			
				break;

			default:
				h--;
				break;
		}
	}
	return 9999;
}

uint16_t FM_getBandAndSpacing(void) {
	uint8_t band = FM_getRegister(RDA5807M_REG_TUNING) & (RDA5807M_BAND_MASK | RDA5807M_SPACE_MASK);
	// Separate channel spacing
	uint8_t space = band & RDA5807M_SPACE_MASK;
	if (band & RDA5807M_BAND_MASK == BAND_EAST && !(FM_getRegister(RDA5807M_REG_BLEND) & RDA5807M_FLG_EASTBAND65M))
	// Lower band limit is 50MHz
	band = (band >> RDA5807M_BAND_SHIFT) + 1;
	else
	band >>= RDA5807M_BAND_SHIFT;
	return ((uint16_t)space<<8 | band);
}

uint8_t FM_setFrequency(uint16_t frequency) {
	uint16_t spaceandband = FM_getBandAndSpacing();
	uint16_t origin = pgm_read_word(&RDA5807M_BandLowerLimits[spaceandband & 0x00FF]);
	// Check that specified frequency falls within our current band limits
	if (frequency < origin || frequency > pgm_read_word(&RDA5807M_BandHigherLimits[spaceandband & 0x00FF]))
	return 1;
	// Adjust start offset
	frequency -= origin;
	uint8_t spacing = pgm_read_byte(&RDA5807M_ChannelSpacings[spaceandband>>8]);
	// Check that the given frequency can be tuned given current channel spacing
	if (frequency * 10 % spacing) return 1;
	// Attempt to tune to the requested frequency
	FM_updateRegister(RDA5807M_REG_TUNING, RDA5807M_CHAN_MASK | RDA5807M_FLG_TUNE,((frequency * 10 / spacing) << RDA5807M_CHAN_SHIFT) | RDA5807M_FLG_TUNE);
	return 0;
}

uint16_t FM_getFrequency(void) {
	uint16_t spaceandband = FM_getBandAndSpacing();
	return pgm_read_word(&RDA5807M_BandLowerLimits[spaceandband & 0x00FF]) +
	(FM_getRegister(RDA5807M_REG_STATUS) & RDA5807M_READCHAN_MASK) *
	pgm_read_byte(&RDA5807M_ChannelSpacings[spaceandband>>8]) /	10;
}

uint8_t FM_getRSSI(void) {
	return (FM_getRegister(RDA5807M_REG_RSSI) & RDA5807M_RSSI_MASK) >> RDA5807M_RSSI_SHIFT;
}

struct max_station station = {0,0};
	
uint16_t SEEK_FOR_RDS()
{
	uint16_t tempw, tempq;
	char grp, ver;
	uint8_t ending = 0;
	uint16_t iter = 0;
	while(ending==0)
	{
		FM_updateRegister(RDA5807M_REG_CONFIG,(RDA5807M_FLG_SEEKUP | RDA5807M_FLG_SEEK | RDA5807M_FLG_SKMODE), (RDA5807M_FLG_SEEKUP | RDA5807M_FLG_SEEK | RDA5807M_FLG_SKMODE));
		tempw = 0;
		tempq = 0;
		while(!(((tempw&0x4000)>>14)&((tempq&0x0080)>>7)))
		{
			_delay_ms(100);
			tempw = FM_getRegister(0x0A); //ожидание завершения настройки
			tempq = FM_getRegister(0x0B);
		}
		UART_Send_Str("found! ");
		tempw = FM_getFrequency();
			
		uint16_t temp1 = tempw%100000;
		uint16_t temp2 = temp1;
		tempw = temp1/1000;
		UART_Send_Char(tempw/10 + 0x30);
		UART_Send_Char(tempw%10 + 0x30);
		temp1 = temp1 % 1000;
		tempw = temp1/100;
		UART_Send_Char(tempw + 0x30);
		tempw = temp1 % 100;
		UART_Send_Char(tempw/10 + 0x30);
		UART_Send_Char(tempw%10 + 0x30);
			
		UART_Send_Str(" MHz ");
			
		tempw = FM_getRSSI();
		temp1 = tempw;
		tempw = temp1/100;
		UART_Send_Char(tempw + 0x30);
		tempw = temp1 % 100;
		UART_Send_Char(tempw/10 + 0x30);
		UART_Send_Char(tempw%10 + 0x30);
		UART_Send_Str(" LEVEL \r\n");
		
		tempw = FM_getRegister(0x0A);
		if((tempw&0x8000)>>15)
		{
			UART_Send_Str("RDS! \r\n");
			iter++;
			if(temp1>station.rssi)
			{
				station.rssi = temp1;
				station.freq = temp2;
			}
				
			if(iter == SEARCH_ITER)
			{
				UART_Send_Str("\nSet: ");
				UART_Send_Char(station.freq/10000 + 0x30);
				UART_Send_Char(((station.freq%10000)/1000) + 0x30);
				UART_Send_Char(((station.freq%10000)%1000)/100 + 0x30);
				UART_Send_Char((((station.freq%10000)%1000)%100)/10 + 0x30);
				UART_Send_Char((((station.freq%10000)%1000)%100)%10 + 0x30);
				UART_Send_Str(" MHz\n");
				ending = 1;
				iter = 0;
			}
			
		}
	}
	return station.freq;
}