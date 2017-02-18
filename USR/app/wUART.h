#ifndef _WUART_H_
#define _WUART_H_

typedef enum
{
	wuart_status_reset = 0,
	wuart_status_waiting_cmd = 1,
	wuart_status_transparent = 2
}wuart_status_def;

#define wuart_cmd_nop  0								//cmd 0000 0000:NOP
#define wuart_cmd_test_connect 0xa1			//cmd 0110 0001:connect test
#define wuart_cmd_to_normal 0xa2				//cmd 0110 0010:set to normal mode
#define wuart_cmd_save_baud 0xa2				//cmd 0110 0010:save current baud
#define wuart_cmd_rf_status 0xa3				//cmd 0110 0011:RF status
#define wuart_cmd_rf_reset 	0xa4				//cmd 0110 0100:RF reset
#define wuart_cmd_rf2rx 		0xa5				//cmd 0110 0101:set RF in rx mode
#define wuart_cmd_rf2tx			0xa6				//cmd 0110 0110:set RF in tx mode
#define wuart_cmd_rfchannel 0x80				//cmd 10xx xxxx:set RF channel
#define wuart_cmd_rfaddress 0xc0				//cmd 11xx xxxx:set RF address

void wuart_init(void);
void wuart_time_isr(void);
void wuart_check(void);
#endif
