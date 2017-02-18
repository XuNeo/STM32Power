#ifndef __NRF24L01_H__
#define __NRF24L01_H__
#include <stdbool.h>


#define RADIO_ADDRESS_WIDTH 3

typedef enum
{
	nrf_data_rate_250K=0x26,
	nrf_data_rate_1M	=0x06,
	nrf_data_rate_2M	=0x06
}nrf_data_rate_def;

typedef enum
{
	nrf_addr_width_3byte	=1,
	nrf_addr_width_4byte	=2,
	nrf_addr_width_5byte	=3
}nrf_addr_width_def;

#define RADIO_RX_MODE true
#define RADIO_TX_MODE false

typedef enum
{
	radio_tx_ok = 1,
	radio_tx_error = 2,
	radio_tx_pending = 3,
	radio_tx_unkown = 4
}radio_send_result_def;


typedef struct
{
	unsigned char in_rx_mode;
	unsigned char en_aa;
	nrf_addr_width_def addr_width;
	nrf_data_rate_def data_rate;
	unsigned char tx_addr[RADIO_ADDRESS_WIDTH];
	unsigned char rf_channel;
	void (*rx_callback)(unsigned char *pdata,unsigned char num);
	void (*tx_complete_call_back)(radio_send_result_def result);
}radio_config_def;

void radio_config(radio_config_def *config);
void radio_init(void);
void radio_power_on(void);
void radio_power_off(void);
void radio_config(radio_config_def *config);
int radio_send_package(const unsigned char *pdata,unsigned char num);
int radio_send_respond_package(const unsigned char *pdata,unsigned char num);
bool radio_is_tx_fifo_full(void);
int radio_flush_tx(void);
int radio_flush_rx(void);
void radio_check(void);


#endif

