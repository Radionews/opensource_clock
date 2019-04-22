#include <stdlib.h>
#include <stdio.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <avr/pgmspace.h>
#include "i2c.h"
#include "rda5807.h"
#include "uart.h"
#include "time.h"

#define F_CPU 1000000
#define LEN 16

uint8_t mode = 0;
uint8_t anod = 1;
volatile uint16_t time_100ms = 0, hard_time = 0;
volatile uint16_t display = 0;
uint16_t old_time = 0, time_from_rds = 0;
volatile unsigned char done;
volatile unsigned char IDX;
char buffer[LEN];

void counter_init();
void indicators_init();
void next_anod();
void catod(uint8_t number);
void anti_catod();
void check_i2c();
void change_display(uint16_t newvalue);

ISR (TIMER1_COMPA_vect)
{
	time_100ms++;
	if(time_100ms == 600)
	{
		hard_time++;
		time_100ms = 0;
		if(hard_time>=1440)
			hard_time = 0;
		change_display(hard_time);
		if((display%60) == 30)
			anti_catod(); //каждые пол часа антиотравление катодов
	}
}

ISR (TIMER2_COMP_vect)
{
	next_anod();
	switch(anod)
	{
		case 1:
		catod((display%60)%10);
		break;
		case 2:
		catod((display%60)/10);
		break;
		case 3:
		catod((display/60)%10);
		break;
		case 4:
		catod((display/60)/10);
		break;
	}		
}

ISR(USART_RXC_vect)
{
	char bf= UDR;
	buffer[IDX]=bf;
	IDX++;

	if (bf == ';' || IDX >= LEN)
	{
		IDX=0;
		done=1;
	}

}

uint8_t hour;
uint8_t min;
uint8_t sec;
uint8_t sec1 = 0;

int main(void)
{
	char* string;
	string = malloc(80);
	
	UART_Init();
	i2cInit();
	FM__init();
	indicators_init();
	check_i2c();
	counter_init();
	rtc_init();
	UART_Send_Str("Initialization complete!\n\r");
	
	if(mode != 3)
	{
		//uint16_t temp = SEEK_FOR_RDS();//сканирование FM диапазона и посик самой сильной станции с RDS
		//FM_setFrequency(temp);//настройка модуля на найденную станцию
	}
	
	IDX=0;
	done=0;
	sei();
	

	while(1)
	{
		if(done)
		{//при приеме времени с uart
			hour = (buffer[0]-48)*10+(buffer[1]-48);
			min = (buffer[2]-48)*10+(buffer[3]-48);
			sec = (buffer[4]-48)*10+(buffer[5]-48);
			if(hour>23)
			{
				hour = 0;
				UART_Send_Str("Bad hour!\n\r");
			}
			if((min>60)||(sec>60))
			{
				min = 0;
				sec = 0;
				UART_Send_Str("Bad min or sec!\n\r");
			}
			done = 0;
			rtc_set_time_s(hour, min, sec);
			UART_Send_Str("Time set!\n\r");
		}
		
		switch(mode)
		{
			case 0:
			//обе микросхемы присутствуют
			rtc_get_time_s(&hour,&min,&sec);
			_delay_ms(50);
			if(sec != sec1)
			{
				change_display(hour*60 + min);
				UART_Send_Str("Time ");
				string = itoa(hour, string, 10);
				UART_Send_Str(string);
				UART_Send_Str(":");
				itoa(min, string, 10);
				UART_Send_Str(string);
				UART_Send_Str(":");
				itoa(sec, string, 10);
				UART_Send_Str(string);
				UART_Send_Str("\n\r");
				sec1 = sec;
				if((hour==2) && (min==45) && (sec==0))
				{
					old_time = FM_setTime(old_time); //получение времени с RDS
					rtc_get_time_s(&hour,&min,&sec);
					UART_Send_Str("Time ");
					string = itoa(hour, string, 10);
					UART_Send_Str(string);
					UART_Send_Str(":");
					itoa(min, string, 10);
					UART_Send_Str(string);
					UART_Send_Str(":");
					itoa(sec, string, 10);
					UART_Send_Str(string);
					UART_Send_Str("\n\r");
					
					if(old_time<1440)
					{
						if(old_time!=(hour*60 + min))
						{
							UART_Send_Str("Time is not correct!");
							rtc_set_time_s(old_time/60, old_time%60, 0);
						}
					}
				}
			}
			break;
		
			case 1:
			//нет микросхемы времени
			old_time = FM_setTime(old_time); //получение времени с RDS
			if(old_time == 3333)
			{
				temp = SEEK_FOR_RDS();
				FM_setFrequency(temp);
			}
			else
			{
				time_from_rds = old_time;
				if(hard_time != time_from_rds) //калибровка времени, если часы не совпадают
				{
					hard_time = time_from_rds;
					time_100ms = 0; //обнуление тиков таймера
				}
			}
			break;
		
			case 3:
			//нет микросхемы тюнера
			rtc_get_time_s(&hour,&min,&sec);
			_delay_ms(50);
			if(sec != sec1)
			{
				change_display(hour*60 + min);
				
				UART_Send_Str("Time ");
				string = itoa(hour, string, 10);
				UART_Send_Str(string);
				UART_Send_Str(":");
				itoa(min, string, 10);
				UART_Send_Str(string);
				UART_Send_Str(":");
				itoa(sec, string, 10);
				UART_Send_Str(string);
				UART_Send_Str("\n\r");
				sec1 = sec;
			}
			break;
		
			default:
				UART_Send_Str("Fuck!\n\r");
			break;
		}
	}
}

void counter_init()
{
	if(mode == 1)
	{//счетчик настраивается только когда нет микросхемы времени
		//счетчик секунд
		TCCR1B |= (1<<WGM12);
		TIMSK |= (1<<OCIE1A);
		OCR1AH = 0x30;
		OCR1AL = 0xD4;
		TCCR1B |= (1<<CS11);
	}
		//счетчик ШИМ
		TCCR2 |= (1<<WGM21)|(1<<CS22);
		TIMSK |= (1<<OCIE2);
		OCR2 = 0x20;//0x40
}
void indicators_init()
{
	DDRD |= (1<<7)|(1<<6)|(1<<5)|(1<<4)|(1<<2)|(1<<3); //настройка анодных и катодных выходов
	PORTD = 0;
	DDRB = 0xFF; //весь порт на выход
	PORTB = 0;
}
void next_anod()
{
	PORTD &= ~(1<<(anod+3));
	PORTB = 0;
	//_delay_ms(100);
	PORTD &= ~((1<<2)|(1<<3));
	
	if(anod!=4)
	{
		anod++;
	}
	else
	anod = 1;
	
	PORTD |= (1<<(anod+3));
}
void catod(uint8_t number)
{
	switch(number)
	{
		case 0:
		PORTD &= ~(1<<3);
		PORTD |= (1<<3);
		break;
		case 1:
		PORTB = 1;
		break;
		case 2:
		PORTB = 2;
		break;
		case 3:
		PORTB = 4;
		break;
		case 4:
		PORTB = 8;
		break;
		case 5:
		PORTB = 16;
		break;
		case 6:
		PORTB = 32;
		break;
		case 7:
		PORTB = 64;
		break;
		case 8:
		PORTB = 128;
		break;
		case 9:
		PORTD &= ~(1<<2);
		PORTD |= (1<<2);
		break;
		default:
		PORTB = 0;
		PORTD &=~((1<<2)|(1<<3));
	}
}
void anti_catod()
{
	for(int a=0;a<4;a++)
	{
		for(int i=0;i<10;i++)
		{
			next_anod();
			catod(i);
			_delay_ms(100);
		}
	}
}
void check_i2c()
{
	UART_Send_Str("Starting check I2C modules!\n\r");
	i2cMasterBufReset();
	mode = 0;
	if(i2cMasterSendBuf(RTC_ADDR) != 0)
	{
		UART_Send_Str("DS3231 is absent!\n\r");
		mode |= 1;
	}
	
	if(i2cMasterSendBuf(RDA5807M_I2C_ADDR_W) != 0)
	{
		UART_Send_Str("RDA5807M is absent!\n\r");
		mode |= (1<<1);
	}
	
	UART_Send_Str("Check complite!\n\r");
	UART_Send_Str("Mode is ");
	UART_Send_Char(mode+30);
	UART_Send_Str("\n\r");
}

void change_display(uint16_t newvalue)
{
	//1us эффект глюка
	
	uint16_t temp = display;
	for(unsigned int ii = 0; ii <255; ii++)
	{
		display = newvalue;
		for(unsigned int ij = 0; ij<ii; ij++)
			_delay_us(1);
		
		display = temp;
		for(unsigned int ij = ii; ij<255; ij++)
			_delay_us(1);
	}
	display = newvalue;
}


