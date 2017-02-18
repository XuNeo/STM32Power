#include "nrf24l01.h"
#include "string.h"
#include "xlink.h"
#include <stdbool.h>
#include <stdio.h>

#include "power.h"//led
#define XLINK_CMD_LIST_NUM 5

#define PARING_DATA_NUM 10

static struct
{
	char cmd;
	char buff[31];
	unsigned char data_in_buff;
}xlink_global_buff;
static xlink_state_def xlink_status=xlink_state_init;
static radio_send_result_def radio_sent_result = radio_tx_unkown;
static bool is_paired = false;
static unsigned char counter_timer_change = 0;
static unsigned char counter_timer_over = 100;//100mS
static bool is_timer_over = false;

static xlink_cmd_def xlink_curr_pending_cmd = xlink_cmd_nop;
//static unsigned char *pxlink_curr_cmd_data = NULL;
static unsigned char xlink_curr_cmd_data_len = 0;

void xlink_data_sent(radio_send_result_def result);
void xlink_data_received(unsigned char *pdata,unsigned char num);
void xlink_data_paser(unsigned char *data,unsigned char num);

void (*xlink_data_received_call)(xlink_cmd_def cmd,unsigned char*,unsigned char) = NULL;
bool (*xlink_data_request_call)(xlink_cmd_def*user_cmd,unsigned char*,unsigned char*) = NULL;

radio_config_def xlink_config=
{
	RADIO_TX_MODE,
	true,
	nrf_addr_width_3byte,
	nrf_data_rate_250K,
	{0,0,0},
	0,
	xlink_data_received,//rx call back
	xlink_data_sent,//tx call back
};

//! \brief random seed
static unsigned short s_hwRandomSeed = 0xAA55;
static unsigned char s_chRandomTable[] = {
                0x12,0x23,0x45,0x67,0x89,0xAB,0xCD,0xEF,
                0xF1,0xE2,0xD3,0xC4,0xB5,0xA6,0x97,0x88};
/*! \note set random generator seed 
*  \param hwSeed random seed
*  \return none
*/
void set_random_seed( unsigned short hwSeed )
{
    s_hwRandomSeed ^= hwSeed;
}

/*! \note get a random integer
*  \param none
*  \return random integer value
*/
unsigned short get_random_u16( void )
{
    unsigned short *phwResult = (unsigned short *)&s_chRandomTable[(s_hwRandomSeed & 0x0E)];
    
    *phwResult += s_hwRandomSeed;
    s_hwRandomSeed ^= *phwResult;
    
    return *phwResult;
}

/*! \note get a random byte
*  \param none
*  \return random integer value
*/
unsigned char get_random_u8( void )
{
    return get_random_u16();
}

void xlink_clear_timer(void)
{
	counter_timer_change = 0;
}
void xlink_set_timer(unsigned short timer)//mS
{
	counter_timer_over = timer/2;//;
	counter_timer_change = 0;
}

//tx call back
void xlink_data_sent(radio_send_result_def result)
{
	//cmd exe result
	radio_sent_result = result;
}

//rx call back
void xlink_data_received(unsigned char *pdata,unsigned char num)
{
	//data analyse
	xlink_data_paser(pdata,num);//after this funtion, data will be not available !!!
	LED_SWITCH();
}

void xlink_send_cmd(xlink_cmd_def cmd,unsigned char *pdata,unsigned char num)
{
	char i=1;
	unsigned char data_lenth = num+1;
	unsigned char temp[32];
	
	if(num >31)
		num = 31;
	
	temp[0] = (unsigned char)cmd;
	while(num--)
	{
		temp[i++] = *pdata++;
	}
	if(xlink_config.in_rx_mode == false)
		radio_send_package((const unsigned char *)temp,data_lenth);
	else
	{
		radio_send_respond_package((const unsigned char *)temp,data_lenth);
	}
	
	radio_sent_result = radio_tx_pending;
}

void xlink_to_tx(void)
{
	radio_power_off();
	radio_flush_rx();
	radio_flush_tx();
	xlink_config.in_rx_mode = false;
	radio_config(&xlink_config);
	LED_OFF();
	radio_sent_result = radio_tx_unkown;
	radio_power_on();
}
void xlink_to_rx(void)
{
	radio_power_off();
	radio_flush_rx();
	radio_flush_tx();
	xlink_config.in_rx_mode = true;
	radio_config(&xlink_config);
	LED_ON();
	radio_sent_result = radio_tx_unkown;
	radio_power_on();
}

void xlink_pair(void)
{	
	//time to change role?
	if(xlink_status == xlink_state_pairing)
	{
		if(is_timer_over)
		{
			if(radio_sent_result == radio_tx_pending && xlink_config.in_rx_mode==false)
			{
				//?y?迆﹞⊿?赤
			}
			else
			{
				is_timer_over = 0;
				xlink_set_timer(get_random_u8());
				if(xlink_config.in_rx_mode)
				{
					xlink_to_tx();
				}
				else
				{
					xlink_to_rx();
				}
			}
		}
		else
		{
			if(xlink_config.in_rx_mode)
			{
				//nothing to do
				//do status transform in rx data parser
			}
			else
			{
				if(radio_sent_result == radio_tx_ok)
				{
					//send successful
					radio_sent_result = radio_tx_unkown;//clear radio status
					xlink_status = xlink_state_configing;
				}
				else if(radio_sent_result != radio_tx_pending)
				{//tx 
					xlink_send_cmd(xlink_cmd_nop,0,0);
				}
			}
		}
	}
	else if(xlink_status == xlink_state_configing)
	{
		if(is_timer_over)
		{
			is_timer_over = 0;
			xlink_config.rf_channel = 0;
			radio_config(&xlink_config);
			xlink_status = xlink_state_pairing;//o邦????車D那?米?cmd_config㏒???米?pairing ℅∩足?
		}
		else 
		{
			if(xlink_config.in_rx_mode)
			{
				//nothing to do
				//do status transform in rx data parser
			}
			else
			{
				//tx mode
				if(radio_sent_result == radio_tx_ok)
				{
					//send successful
					xlink_status = xlink_state_testing;
					//config rf
					radio_config(&xlink_config);
					radio_sent_result = radio_tx_unkown;//clear
				}
				else if(radio_sent_result != radio_tx_pending)
				{//tx 
					xlink_config.rf_channel = 70;
					xlink_send_cmd(xlink_cmd_conf_rf,(unsigned char*)&xlink_config,sizeof(xlink_config));
				}
			}
		}
	}
	else if(xlink_status == xlink_state_testing)
	{
		if(is_timer_over)
		{
			is_timer_over = 0;
			xlink_config.rf_channel = 0;
			radio_config(&xlink_config);
			xlink_status = xlink_state_pairing;//o邦????車D那?米?test㏒???米?pairing ℅∩足?
		}
		else 
		{
			if(xlink_config.in_rx_mode)
			{
				//nothing to do
				//do status transform in rx data parser
			}
			else
			{
				//tx mode
				if(radio_sent_result == radio_tx_ok)
				{
					//send successful
					xlink_status = xlink_state_normal;
					//config rf
					radio_sent_result = radio_tx_unkown;//clear
				}
				else if(radio_sent_result != radio_tx_pending)
				{//tx 
					xlink_send_cmd(xlink_cmd_test,0,0);
				}
			}
		}
	}
	else if(xlink_status == xlink_state_normal)
	{//normal mode
		is_paired = true;
		xlink_set_timer(100);
		if(xlink_config.in_rx_mode)
		{
			printf("connected:in rx mode\n");
		}
		else
		{
			printf("connected:in tx mode\n");
		}
	}
}

void xlink_paired(void)
{
	//if(xlink_config.in_rx_mode)
	{
		//fill tx buff
		while(radio_is_tx_fifo_full() == false)
		{
			if(xlink_data_request_call&&
				xlink_data_request_call(&xlink_curr_pending_cmd,(unsigned char *)xlink_global_buff.buff,&xlink_curr_cmd_data_len))
			{
				
			}
			else
			{
				xlink_curr_pending_cmd = xlink_cmd_nop;
				xlink_curr_cmd_data_len = 32;
			}
			xlink_send_cmd(xlink_curr_pending_cmd,(unsigned char *)xlink_global_buff.buff,xlink_curr_cmd_data_len);
		}
	}/*
	else
	{
		if(radio_sent_result == radio_tx_ok)
		{
			if(xlink_data_request_call&&
				xlink_data_request_call(&xlink_curr_pending_cmd,&pxlink_curr_cmd_data,&xlink_curr_cmd_data_len))
			{
				
			}
			else
			{
				xlink_curr_pending_cmd = xlink_cmd_nop;
				xlink_curr_cmd_data_len = 0;
			}
			xlink_clear_timer();
		}
		if(radio_sent_result != radio_tx_pending)
		{
			xlink_send_cmd(xlink_curr_pending_cmd,pxlink_curr_cmd_data,xlink_curr_cmd_data_len);
		}
	}*/
	
	if(radio_sent_result != radio_tx_pending)
	{
		xlink_clear_timer();
	}
		
		
	if(is_timer_over)
	{//那∫豕ㄓ芍??車
		is_timer_over = 0;
		#if 0
		xlink_config.rf_channel = 0;
		radio_config(&xlink_config);
		xlink_status = xlink_state_pairing;//o邦????車D那?米?test㏒???米?pairing ℅∩足?
		is_paired = false;
		#endif
	}
}

//0:cmd ...:data
//received cmd paser
void xlink_data_paser(unsigned char *data,unsigned char num)
{
	xlink_cmd_def cmd;

	if(num==0)
		return;
	
	cmd = *(xlink_cmd_def*)data;
	data++;//move to data section
	switch(cmd)
	{
		case xlink_cmd_nop:
			if(xlink_status == xlink_state_pairing)
			{
				if(xlink_config.in_rx_mode)
				{
					xlink_status = xlink_state_configing;//那?米?那y?Y
					xlink_set_timer(200);//10mS to receive config_rf package
				}
			}
			else
			{
				xlink_clear_timer();
			}
			break;
		case xlink_cmd_conf_rf:
			if(xlink_status == xlink_state_configing)
			{
				if(xlink_config.in_rx_mode)
				{
					//config rf
					xlink_config.rf_channel = 70;
					radio_config(&xlink_config);
					xlink_status = xlink_state_testing;//那?米?那y?Y
					xlink_set_timer(200);//10mS to receive test package
				}
			}
			break;
		case xlink_cmd_test:
			if(xlink_status == xlink_state_testing)
			{
				if(xlink_config.in_rx_mode)
				{
					xlink_status = xlink_state_normal;
				}
			}
			break;
		default://user cmd
			xlink_clear_timer();
			if(xlink_data_received_call)
			{
				xlink_data_received_call(cmd,data,num-1);
			}
			break;
	}
}

void xlink_routin(void)
{
	radio_check();
	if(is_paired == false)
	{
		xlink_pair();
	}
	else
	{
		xlink_paired();
	}
}

void xlink_timer_isr(void)
{
	//2mS
	counter_timer_change ++;
	if(counter_timer_change == counter_timer_over)
	{
		counter_timer_change = 0;
		is_timer_over = true;
	}
}


//public
 
void xlink_init(void(*preceived_func)(xlink_cmd_def cmd,unsigned char*,unsigned char),
								bool(*prequest_func)(xlink_cmd_def*user_cmd,unsigned char*,unsigned char*))
{
	radio_init();
	radio_config(&xlink_config);//tx mode
	radio_power_on();
	LED_OFF();
	xlink_status = xlink_state_pairing;
	xlink_data_received_call = preceived_func;
	xlink_data_request_call = prequest_func;
}

bool xlink_is_busy(void)
{
	if(xlink_curr_pending_cmd == xlink_cmd_nop)
	{
		return false;
	}
	else
		return true;
}
/*
bool xlink_write_data(unsigned char*pdata,unsigned char data_num)
{
	if(!is_paired)
	{
		return false;
	}
	if(xlink_is_busy())
		return false;
	
	xlink_curr_pending_cmd = xlink_cmd_user;
	pxlink_curr_cmd_data = pdata;
	xlink_curr_cmd_data_len = data_num;
	return true;
}*/
xlink_state_def xlink_get_status(void)
{
	return xlink_status;
}
