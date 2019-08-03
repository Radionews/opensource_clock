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

#define ALL
//#define DS
//#define RDA

//режим работы
//ALL - микросхема времени DS3231 и RDA5807 подключены
//DS - только микросхема DS3231
//RDA - только микросхема RDA5807

volatile uint8_t anod = 1;
volatile uint32_t time_100ms = 0;
volatile uint32_t display = 0;
uint16_t old_time = 0;
uint32_t time_from_rds = 0;
volatile unsigned char done;
volatile unsigned char IDX;
char buffer[LEN];

uint8_t hour;
uint8_t min;
uint8_t sec;
uint8_t sec1 = 0;

void counter_init();
void indicators_init();
void next_anod();
void catod(uint8_t number);
void anti_catod();
//void change_display(uint16_t newvalue); прототип функции смены цифры, пока недоработано

#ifdef RDA
//если подключен только FM модуль то будет работать внутренний счетчик секунд на счетчике
ISR (TIMER1_COMPA_vect)
{
	time_100ms++;
	if(time_100ms%600 == 0)
	{
		if((time_100ms/600)>=1440)
			time_100ms = 0;
		display = time_100ms/600;
		if((display%60) == 30)
			anti_catod(); //каждые пол часа антиотравление катодов
	}
}
#endif

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

int main(void)
{
	char* string;
	string = malloc(80);
		
	UART_Init();
	UART_Send_Str("Starting...\n\r");
	i2cInit();
	indicators_init();
	counter_init();

#ifdef RDA
	UART_Send_Str("Only RDA MODE!\n\r");
	FM__init();
	uint16_t temp = SEEK_FOR_RDS();//сканирование FM диапазона и посик самой сильной станции с RDS
	FM_setFrequency(temp);//настройка модуля на найденную станцию
#endif	

#ifdef DS
	UART_Send_Str("Only DS MODE!\n\r");
	rtc_init();
#endif

#ifdef ALL
	UART_Send_Str("ANY MODE!\n\r");
	FM__init();
	uint16_t temp = SEEK_FOR_RDS();//сканирование FM диапазона и посик самой сильной станции с RDS
	FM_setFrequency(temp);//настройка модуля на найденную станцию
	
	old_time = FM_setTime(old_time); //получение времени с RDS
	rtc_get_time_s(&hour,&min,&sec);
	if(old_time!=(hour*60 + min))
	{
		UART_Send_Str("Calibrate time!");
		rtc_set_time_s(old_time/60, old_time%60, 0);
	}
#endif

	IDX=0;
	done=0;
	sei();
	
	UART_Send_Str("Init complite...\n\r");

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
		
#ifdef ALL
		rtc_get_time_s(&hour,&min,&sec);
		_delay_ms(50);
		if(sec != sec1)
		{
			display = hour*60 + min;
			/*UART_Send_Str("Time ");
			string = itoa(hour, string, 10);
			UART_Send_Str(string);
			UART_Send_Str(":");
			itoa(min, string, 10);
			UART_Send_Str(string);
			UART_Send_Str(":");
			itoa(sec, string, 10);
			UART_Send_Str(string);
			UART_Send_Str("\n\r");*/
			sec1 = sec;
			if (min == 30)
			{
				anti_catod();
			}
			if((hour==2) && (min==45) && (sec==0))//время сверки времени RDS с микросхемой времени
			{
				old_time = FM_setTime(old_time); //получение времени с RDS
				rtc_get_time_s(&hour,&min,&sec);
				/*UART_Send_Str("Time ");
				string = itoa(hour, string, 10);
				UART_Send_Str(string);
				UART_Send_Str(":");
				itoa(min, string, 10);
				UART_Send_Str(string);
				UART_Send_Str(":");
				itoa(sec, string, 10);
				UART_Send_Str(string);
				UART_Send_Str("\n\r");*/
					
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
#endif
#ifdef DS
		//только микросхема времени
			rtc_get_time_s(&hour,&min,&sec);
			_delay_ms(50);
			if(sec != sec1)
			{
				display = hour*60 + min;
				/*UART_Send_Str("Time ");
				string = itoa(hour, string, 10);
				UART_Send_Str(string);
				UART_Send_Str(":");
				itoa(min, string, 10);
				UART_Send_Str(string);
				UART_Send_Str(":");
				itoa(sec, string, 10);
				UART_Send_Str(string);
				UART_Send_Str("\n\r");*/
				sec1 = sec;
				
				if (min == 30)
				{
					anti_catod();
				}
			}
#endif
					
#ifdef RDA
		//только микросхема FM тюнера
			old_time = FM_setTime(old_time); //получение времени с RDS
			
			UART_Send_Str("Time ");
			string = itoa(old_time, string, 10);
			UART_Send_Str(string);
			UART_Send_Str("\n\r");
			if(old_time == 3333)
			{
				temp = SEEK_FOR_RDS();
				FM_setFrequency(temp);
			}
			else
			{
				time_from_rds = old_time;
				if((time_100ms/600) != time_from_rds) //калибровка времени, если часы не совпадают
				{
					UART_Send_Str("Time calibrate RDA MODE\n\r");
					time_100ms = time_from_rds*600;
					//time_100ms = 0; //обнуление тиков таймера
				}
			}
#endif
		
	}
}

void counter_init()
{

#ifdef RDA
//счетчик настраивается только когда нет микросхемы времени
		//счетчик секунд
		TCCR1B |= (1<<WGM12);
		TIMSK |= (1<<OCIE1A);
		OCR1AH = 0x30;
		OCR1AL = 0xD4;
		TCCR1B |= (1<<CS11);
#endif
		//счетчик ШИМ
		TCCR2 |= (1<<WGM21)|(1<<CS22);
		TIMSK |= (1<<OCIE2);
		OCR2 = 0x30;//частота динамической индюиации, чем больше число тем медленней
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
	if(anod<4)
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
			//PORTD &= ~(1<<3);
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
			//PORTD &= ~(1<<2);
			PORTD |= (1<<2);
			break;
		default:
			//PORTD &= ~(1<<3);
			//при ошибке выводить 0
			PORTD |= (1<<3);
			break;
	}
}

void anti_catod()
{
	cli();
	for(uint8_t a=0;a<4;a++)
	{
		next_anod();
		for(uint8_t i=0;i<10;i++)
		{
			catod(i);
			_delay_ms(100);
		}
	}
	sei();
}

/*
void change_display(uint16_t newvalue)
{
	//заготовка для функции эффектов смены цифры
}*/


