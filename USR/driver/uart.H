#ifndef _USART_H_
#define _USART_H_

extern void uart_init(void);
extern void uart_char(char data);
extern void uart_string(const char *p);
void uart_event(void);

#endif
