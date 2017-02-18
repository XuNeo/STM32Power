#include "wuart.h"
#include "xlink.h"
#include "uart.h"
#include "CBuff.h"
#include "pp_buff.h"
#include "power.h"

//gloable
wuart_status_def static wuart_status=wuart_status_reset;
static CBuff_st wuart_uart_rx;
static CBuff_st wuart_uart_tx;
static pp_buff_def uart_buffer_rx;

//auto baud rate dector
static bool wuart_baud_is_measured = false;
//static unsigned short wuart_edge_count=0;

static unsigned short wuart_timer_end;
static unsigned short wuart_timer_count;
static bool wuart_flag_timer_over = false;

//static bool wuart_is_first_edge=true;
//static unsigned short wuart_measured_time=0;

//static unsigned long send_data_count = 0;

//gloable functions
void wuart_set_timer(unsigned short time);
void wuart_clear_timer(void);
void wuart_send_data(char c);


//auto baud functions
void uart_edge_measure_reset(void)
{
//	wuart_edge_count = 0;
//	wuart_is_first_edge = true;
//	wuart_baud_is_measured = false;
//	EXTI_CR1 = (2<<6);//portD negedge
//	PD_CR2 |= (1<<6);//enable interrupt
//	UART1_CR2 = 0;
}

void wuart_cmd(void)
{
	static bool is_confirmed = false;
	char c;
	
	if(wuart_flag_timer_over)
	{
		wuart_flag_timer_over = false;
		if(is_confirmed)
		{//connectted 
			uart_string("TIME OVER turn to normal mode [OK]\n");
			wuart_status = wuart_status_transparent;
		}
		else
		{//not confirmed, return to reset status
			wuart_status = wuart_status_reset;
			uart_edge_measure_reset();
			wuart_set_timer(2);
		}
	}
	while(CBuff_Read(&wuart_uart_rx,&c) == CBuff_EOK)
	{
		if(is_confirmed)
		{
			switch(c)
			{
				case wuart_cmd_nop:
					break;
				case wuart_cmd_test_connect:
					uart_string("connect [OK]\n");
					wuart_clear_timer();
					break;
				case wuart_cmd_to_normal:
					uart_string("turn to normal mode [OK]\n");
					wuart_status = wuart_status_transparent;
					break;
				//case wuart_cmd_save_baud:
				//	break;
				case wuart_cmd_rf_status:
					{
						xlink_state_def status;
						status = xlink_get_status();
						uart_string("RF in "); 
						switch(status)
						{
							case xlink_state_init:uart_string("init");
								break;
							case xlink_state_pairing:
								uart_string("pairing");
								break;
							case xlink_state_configing:
								uart_string("configuring");
								break;
							case xlink_state_testing:
								uart_string("testing");
								break;
							case xlink_state_normal:
								uart_string("normal(connectted)");
								break;
						}
						uart_string(" state [OK]\n");
						wuart_clear_timer();
					}
					break;
				case wuart_cmd_rf_reset:
					uart_string("RF reset [OK]\n");
					wuart_clear_timer();
					break;
				case wuart_cmd_rf2rx:
					uart_string("RF to RX [OK]\n");
					wuart_clear_timer();
					break;
				case wuart_cmd_rf2tx:
					uart_string("RF to TX [OK]\n");
					wuart_clear_timer();
					break;
				default:
						if(c&0x80)
						{
							if(c&0x40)
							{
								//set rf address
							}
							else
							{
								//set rf channel
							}
							wuart_clear_timer();
						}
						else
						{
							uart_string("CMD not supportted:");
							uart_char(c);
							uart_string("-end\n");
						}
					break;
			}
		}
		else
		{
			if(c == 0x55)
			{
				is_confirmed = true;
			}
		}
	}
}


void wuart_check(void)
{//1mS
	switch(wuart_status)
	{
		case wuart_status_reset:
			//check baud rate
			if(wuart_baud_is_measured)
			{
				wuart_set_timer(1000);//1S waiting confirm
				wuart_status = wuart_status_waiting_cmd;
			}
			else if(wuart_flag_timer_over)
			{
				uart_edge_measure_reset();
				wuart_flag_timer_over = false;
			}
			break;
		case wuart_status_waiting_cmd:
			wuart_cmd();
			break;
		case wuart_status_transparent:
			{
				char c;
				while(CBuff_Read(&wuart_uart_tx,&c) == CBuff_EOK)
				{//send data to uart
					uart_char(c);
				}
			}
			break;
		default:
			break;
	}
	if(wuart_status != wuart_status_transparent)
	if(pp_buff_change(&uart_buffer_rx) == PP_BUFF_EOK)
	{
		unsigned char c;
		while(pp_buff_read(&uart_buffer_rx,&c))
		{
			CBuff_Write(&wuart_uart_rx,c);
		}
	}
}


void wuart_set_timer(unsigned short time)
{
	wuart_timer_end = time;
}

void wuart_clear_timer(void)
{
	wuart_timer_count = 0;
}

void wuart_time_isr(void)
{
	//1mS
	wuart_timer_count++;//cleared in edge interrupt
	
	if(wuart_timer_end == wuart_timer_count)
	{//timer 只能有一个任务使用，并由该任务清零标志
		wuart_timer_count = 0;
		wuart_flag_timer_over = true;
	}
}

void wuart_io_isr(void)
{

}

//transparent functions
void wuart_rx_callback(xlink_cmd_def cmd,unsigned char*pdata,unsigned char num)
{
	while(num--)
	{
		CBuff_Write(&wuart_uart_tx,*pdata++);
	}
}
bool wuart_request_callback(xlink_cmd_def *user_cmd,unsigned char*ppdata,unsigned char *pnum)
{
	//fill data in ppdata max size is 31
	//unsigned char data_num = 0;
	//char c;
	*user_cmd = xlink_cmd_user;
	if(pp_buff_change(&uart_buffer_rx) == PP_BUFF_EOK)
	{
		*pnum = pp_buff_nread(&uart_buffer_rx,ppdata,31);
	}
	else
	{
		*pnum = 0;
	}
	/*
	while(CBuff_Read(&wuart_uart_rx,&c) == CBuff_EOK)
	{//send data to uart
		*ppdata++ = c;
		data_num ++;
		if(data_num == 31)
		 break;
		
		send_data_count++;
	}
	*pnum = data_num;*/
	return *pnum;
}

void wuart_init(void)
{
	CBuff_Init(&wuart_uart_rx);
	CBuff_Init(&wuart_uart_tx);
	xlink_init(wuart_rx_callback,wuart_request_callback);
	//PD6 -->Rx as input 
	//turn on interrupt, falling trigger to measure baudrate
//	PD_DDR &= ~(1<<6);//input
//	PD_CR1 |= (1<<6);// pull up 
//	EXTI_CR1 = (2<<6);//portD negedge
//	PD_CR2 |= (1<<6);//enable interrupt
//	
//	TIM2_PSCR = 0;
//	TIM2_ARRH =(65535)>>8;					
//	TIM2_ARRL = 65535;
//	TIM2_CR1 |= BIT0;//CEN：计数器使能位
	wuart_set_timer(2);//2mS reset
	pp_buff_init(&uart_buffer_rx);
}


static unsigned long uart_data_count = 0;
void UART_ISR (void)
{
	//UART1_SR = 0;
	uart_data_count++;
	//pp_buff_write(&uart_buffer_rx,UART1_DR);
	
}
