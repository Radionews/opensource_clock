#define FOSC 8000000// Clock Speed
#define BAUD 9600
#define MYUBRR FOSC/16/BAUD-1

void UART_Init(void);
void UART_Send_Char(unsigned char c);
void UART_Send_Str(unsigned char* str);