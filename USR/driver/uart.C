#include "UART.H"

void uart_init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	
	//USART1 TXD-->PA2 RXD-->PA3
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOA,ENABLE);
	
	GPIO_PinAFConfig(GPIOA,GPIO_PinSource2,GPIO_AF_1);
	GPIO_PinAFConfig(GPIOA,GPIO_PinSource3,GPIO_AF_1);

	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;//50MHz
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2|GPIO_Pin_3;
	GPIO_Init(GPIOA,&GPIO_InitStructure);
	
	//USART
	USART_InitStructure.USART_BaudRate = 115200;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Rx|USART_Mode_Tx;
	USART_InitStructure.USART_Parity = USART_Parity_No;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_Init(USART1,&USART_InitStructure);
	USART_ITConfig(USART1,USART_IT_RXNE,ENABLE);
	
	//USART_SWAPPinCmd(USART1,ENABLE);
	
	USART_Cmd(USART1,ENABLE);
	//interrupt configure
	NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_InitStructure.NVIC_IRQChannelPriority = 0;
	NVIC_Init(&NVIC_InitStructure);//	USART_String("at\r\n");
}

//int fputc(int ch, FILE *f)
//{
//	USART_SendData(USART1, (unsigned char) ch);// USART1 ???? USART2 ?
//	while (!(USART1->ISR & USART_FLAG_TXE));
//	return (ch);
//}
void uart_char(char data)
{
	USART_SendData(USART1, (unsigned char) data);// USART1 ???? USART2 ?
	while (!(USART1->ISR & USART_FLAG_TXE));
}

void uart_string(const char *p)
{
	while(*p)
	{
		USART1->TDR = *p++;
		while (!(USART1->ISR & USART_FLAG_TXE));
	}
}
#include "CBuff.h"
CBuff_st cbuff;

void power_on(void);
void power_off(void);
void power_set(int volt);

void uart_event(void)
{
	static int volt = 100;
	char c;
	while(1)
	{
		if(CBuff_Read(&cbuff,&c) == CBuff_EOK)
		{
			switch(c)
			{
				case 'o':
					power_on();
					break;
				case 'f':
					power_off();
					break;
				case 'i':
					if(volt < 1000)
					volt += 100;
					power_set(volt);
					break;
				case 'd':
					if(volt >= 100)
					volt -= 100;
					power_set(volt);
					break;
				default:
					break;
			}
		}
		else
			break;
	}
}

void USART1_IRQHandler(void)
{
	volatile unsigned char c;
	
	if(USART_GetITStatus(USART1, USART_IT_RXNE) != RESET)
	{
		CBuff_Write(&cbuff,USART1->RDR);
	}
}

