#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <avr/pgmspace.h>
#include "i2c.h"
#include "rda5807.h"
#include "uart.h"

#define F_CPU 1000000

uint8_t anod = 1;
volatile uint16_t time_100ms = 0, hard_time = 0;
volatile uint16_t display = 0;
uint16_t old_time = 0, time_from_rds = 0;

void counter_init()
{
	//счетчик секунд
	TCCR1B |= (1<<WGM12);
	TIMSK |= (1<<OCIE1A);
	OCR1AH = 0x30;
	OCR1AL = 0xD4;
	TCCR1B |= (1<<CS11);
	
	//счетчик ШИМ
	TCCR2 |= (1<<WGM21)|(1<<CS22);
	TIMSK |= (1<<OCIE2);
	OCR2 = 0x40;
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

void catod( uint8_t number)
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
		next_anod();
		for(int i=0;i<10;i++)
		{
			catod(i);
			_delay_ms(100);
		}
	}
}
ISR (TIMER1_COMPA_vect)
{
	time_100ms++;
	if(time_100ms == 600)
	{
		hard_time++;
		time_100ms = 0;
		display = hard_time;
		if((display%60) == 30)
			anti_catod();//каждые пол часа антиотравление катодов
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

int main(void)
{
	UART_Init();
	i2cInit();
	FM__init();
	indicators_init();
	counter_init();
	UART_Send_Str("Initialization complete!\n\r");
	sei();
	//FM_setFrequency(9110);
    
	uint16_t temp = SEEK_FOR_RDS();//сканирование FM диапазона и посик самой сильной станции с RDS
	FM_setFrequency(temp);//настройка модуля на найденную станцию
	
	while(1)
	{
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

	}
}

