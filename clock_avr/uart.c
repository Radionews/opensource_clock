#include <avr/io.h>
#include "uart.h"

void UART_Init(void)
{
	UBRRL = MYUBRR;
	UBRRH = MYUBRR>>8;
	UCSRB = (1<<RXEN)|(1<<TXEN);
	UCSRC |=(1<< URSEL)|(1<< UCSZ0)|(1<< UCSZ1);
}

void UART_Send_Char(unsigned char c)
{
	while (!(UCSRA&(1<<UDRE)));
	UDR = c;
}

void UART_Send_Str(unsigned char* str)
{
	while (*str != 0) UART_Send_Char(*str++);
}