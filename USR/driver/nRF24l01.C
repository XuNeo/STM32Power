#include "nrf24l01.h"

//board layer
/***********************************************/
#define NRF_SPI_CSN_L() GPIOB->BRR = GPIO_Pin_1
#define NRF_SPI_CSN_H() GPIOB->BSRR = GPIO_Pin_1

#define NRF_CE_L() GPIOA->BRR = GPIO_Pin_3
#define NRF_CE_H() GPIOA->BSRR = GPIO_Pin_3

#define NRF_IRQ_EN()
#define NRF_IRQ_DIS() 

unsigned char spi_read_write(unsigned char dat) 
{ 
	while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_TXE) == RESET); 
	//SPI_I2S_SendData(SPI1, dat); 
	SPI_SendData8(SPI1,dat);
	while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE) == RESET);  
	return SPI_ReceiveData8(SPI1); 
}

void nrf_hard_init(void)
{
	SPI_InitTypeDef SPI_InitStructure; 
	GPIO_InitTypeDef GPIO_InitStructure;
	EXTI_InitTypeDef EXTI_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1, ENABLE);
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOA|RCC_AHBPeriph_GPIOB,ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, ENABLE);
	
	NRF_CE_L();
	NRF_SPI_CSN_H();
	
	GPIO_PinAFConfig(GPIOA,GPIO_PinSource5,GPIO_AF_0);
	GPIO_PinAFConfig(GPIOA,GPIO_PinSource6,GPIO_AF_0);
	GPIO_PinAFConfig(GPIOA,GPIO_PinSource7,GPIO_AF_0);
	//spi SCLK MOSI MISO
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_Level_3;//50MHz
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5|GPIO_Pin_6|GPIO_Pin_7;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	
	//irq-->PA5
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_Level_3;//50MHz
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	
	//CE-->PA3,CS-->PB1
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_Level_2;//10MHz
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
	GPIO_Init(GPIOA,&GPIO_InitStructure);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;
	GPIO_Init(GPIOB,&GPIO_InitStructure);
	
	
	SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex; 
	SPI_InitStructure.SPI_Mode = SPI_Mode_Master;  
	SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;  
	SPI_InitStructure.SPI_CPOL = SPI_CPOL_Low;  
	SPI_InitStructure.SPI_CPHA = SPI_CPHA_1Edge; 
	SPI_InitStructure.SPI_NSS = SPI_NSS_Soft; 
	SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_8; 
	SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB; 
	SPI_InitStructure.SPI_CRCPolynomial = 7; 
	
	SPI_Init(SPI1, &SPI_InitStructure); 
	SPI_RxFIFOThresholdConfig(SPI1,SPI_RxFIFOThreshold_QF);
	SPI_Cmd(SPI1, ENABLE);
	
	SYSCFG_EXTILineConfig(EXTI_PortSourceGPIOA,EXTI_PinSource4);
	
	EXTI_InitStructure.EXTI_Line = EXTI_Line4;//PA4
  EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
  EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;
  EXTI_InitStructure.EXTI_LineCmd = ENABLE;
  EXTI_Init(&EXTI_InitStructure);
	
  NVIC_InitStructure.NVIC_IRQChannel = EXTI4_15_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);
}
//board layer end
/***********************************************/
static char flag_interrupt = 0;
static unsigned char nrf_read_buff[32];
static void (*nrf_rx_ok_indicate)(unsigned char *pdata,unsigned char num);
static void (*nrf_tx_ok_indicate)(radio_send_result_def result);
//bit define 
#define NRF_CONFIG_MASK_RX_DR 0x80
#define NRF_CONFIG_BIT_MASK_TX_DS 0x40
#define NRF_CONFIG_BIT_MASK_MAX_RT 0x10
#define NRF_CONFIG_BIT_EN_CRC 0x08
#define NRF_CONFIG_BIT_CRC_BYTE2 0x04
#define NRF_CONFIG_BIT_PWR_UP 0x02
#define NRF_CONFIG_BIT_PRX 0x01

#define NRF_STATUS_BIT_RX_DR 0x40
#define NRF_STATUS_BIT_TX_DS 0x20
#define NRF_STATUS_BIT_MAX_RT 0x10
#define NRF_STATUS_BIT_RX_P_NO 0x0e
#define NRF_STATUS_BIT_TX_FULL 0x01
#define NRF_STATUS_ALL_FLAG NRF_STATUS_BIT_RX_DR|NRF_STATUS_BIT_TX_DS|NRF_STATUS_BIT_MAX_RT

#define NRF_CMD_R_REG 					0x00
#define NRF_CMD_W_REG 					0x20
#define NRF_CMD_R_RX_PL 				0x61
#define NRF_CMD_W_TX_PL 				0xa0
#define NRF_CMD_FLUSH_TX 				0xe1
#define NRF_CMD_FLUSH_RX 				0xe2
#define NRF_CMD_REUSE_TX_PL 		0xe3
#define NRF_CMD_R_RX_PL_WID	 		0x60
#define NRF_CMD_W_ACK_PL(pipe) 	(0xa8|(pipe<5?pipe:0))
#define NRF_CMD_W_TX_PL_NO_ACK 	0xb0
#define NRF_CMD_NOP 						0xff


/* Registers address definition */
#define NRF_REG_CONFIG 0x00
#define NRF_REG_EN_AA 0x01
#define NRF_REG_EN_RXADDR 0x02
#define NRF_REG_SETUP_AW 0x03
#define NRF_REG_SETUP_RETR 0x04
#define NRF_REG_RF_CH 0x05
#define NRF_REG_RF_SETUP 0x06
#define NRF_REG_STATUS 0x07
#define NRF_REG_OBSERVE_TX 0x08
#define NRF_REG_RPD 0x09
#define NRF_REG_RX_ADDR_P0 0x0A
#define NRF_REG_RX_ADDR_P1 0x0B
#define NRF_REG_RX_ADDR_P2 0x0C
#define NRF_REG_RX_ADDR_P3 0x0D
#define NRF_REG_RX_ADDR_P4 0x0E
#define NRF_REG_RX_ADDR_P5 0x0F
#define NRF_REG_TX_ADDR 0x10
#define NRF_REG_RX_PW_P0 0x11
#define NRF_REG_RX_PW_P1 0x12
#define NRF_REG_RX_PW_P2 0x13
#define NRF_REG_RX_PW_P3 0x14
#define NRF_REG_RX_PW_P4 0x15
#define NRF_REG_RX_PW_P5 0x16
#define NRF_REG_FIFO_STATUS 0x17
#define NRF_REG_DYNPD 0x1C
#define NRF_REG_FEATURE 0x1D

typedef enum
{
	nrf_reg_rxaddr_pipe0=NRF_REG_RX_ADDR_P0,
	nrf_reg_rxaddr_pipe1=NRF_REG_RX_ADDR_P1,
	nrf_reg_rxaddr_pipe2=NRF_REG_RX_ADDR_P2,
	nrf_reg_rxaddr_pipe3=NRF_REG_RX_ADDR_P3,
	nrf_reg_rxaddr_pipe4=NRF_REG_RX_ADDR_P4,
	nrf_reg_rxaddr_pipe5=NRF_REG_RX_ADDR_P5
}nrf_rx_addr_def;

typedef enum
{
	nrf_reg_rx_pw_pipe0=NRF_REG_RX_PW_P0,
	nrf_reg_rx_pw_pipe1=NRF_REG_RX_PW_P1,
	nrf_reg_rx_pw_pipe2=NRF_REG_RX_PW_P2,
	nrf_reg_rx_pw_pipe3=NRF_REG_RX_PW_P3,
	nrf_reg_rx_pw_pipe4=NRF_REG_RX_PW_P4,
	nrf_reg_rx_pw_pipe5=NRF_REG_RX_PW_P5
}nrf_rx_pl_width_def;



//basic operation
void nrf_rf_power_on(void)
{
	NRF_CE_H();
}
void nrf_rf_power_off(void)
{
	NRF_CE_L();
}
//middle
unsigned char nrf_write_reg(unsigned char reg_addr,unsigned char value)
{	
	unsigned char status;

	NRF_SPI_CSN_L();					 
	status = spi_read_write(reg_addr|NRF_CMD_W_REG);  
	spi_read_write(value);		 
	NRF_SPI_CSN_H();

  return 	status;
}

unsigned char nrf_write_nreg(unsigned char reg_addr,const unsigned char *pdata,unsigned char num)
{	
	unsigned char status;
	
	NRF_SPI_CSN_L();					 
	status = spi_read_write(reg_addr|NRF_CMD_W_REG); 
	while(num--)
	{
		spi_read_write(*pdata++);	
	}	 
	NRF_SPI_CSN_H();		
		
  return 	status;
}

unsigned char nrf_read_reg(unsigned char reg_addr,unsigned char *out_data)
{
	unsigned char status;
	
	reg_addr &= 0x1f;
	
	NRF_SPI_CSN_L();
	status = spi_read_write(reg_addr|NRF_CMD_R_REG);
	*out_data = spi_read_write(NRF_CMD_NOP);
	NRF_SPI_CSN_H();

	return status;
}

unsigned char nrf_read_status(void)
{
	unsigned char status;
	
	NRF_SPI_CSN_L();
	status = spi_read_write(NRF_CMD_NOP);
	NRF_SPI_CSN_H();
	
	return status;
}

unsigned char nrf_read_fifo_status(void)
{
	unsigned char  status;
	
	nrf_read_reg(NRF_REG_FIFO_STATUS,&status);
	return status;
}
unsigned char nrf_clear_status(unsigned char bit_mask)
{
	unsigned char status;
	
	status = nrf_write_reg(NRF_REG_STATUS,bit_mask);
	return status;
}

unsigned char nrf_set_reg_bit(unsigned char reg_addr,unsigned char which_bit,bool enable)
{
	unsigned char temp=0,status;
	
	if(which_bit>8)
		which_bit = 8;
	nrf_read_reg(reg_addr,&temp);
	if(enable)
	{
		temp |= 0x01<<which_bit;
	}
	else
	{
		temp &= ~(0x01<<which_bit);
	}
	status = nrf_write_reg(reg_addr,temp);
	return status;
}
//highner
unsigned char nrf_set_tx_address(const unsigned char *paddr,unsigned char num)
{
	unsigned char status;
	if(num>5)
	{
		num = 5;
	}
	status = nrf_write_nreg(NRF_REG_TX_ADDR,paddr,num);
	return status;
}

unsigned char nrf_set_rx_address(nrf_rx_addr_def pipe,const unsigned char *paddr,unsigned char num)
{
	unsigned char status;
	if(num>5)
	{
		num = 5;
	}
	status = nrf_write_nreg(pipe,paddr,num);
	return status;
}
unsigned char nrf_set_address_width(nrf_addr_width_def width)
{
	unsigned char status;
	
	status = nrf_write_reg(NRF_REG_SETUP_AW,width);
	return status;
}

//width =0 means this pipe is not used
unsigned char nrf_set_rx_pl_num(nrf_rx_pl_width_def pw_reg,unsigned char width)
{
	unsigned char status;
	
	status = nrf_write_reg(pw_reg,width);
	return status;
}

unsigned char nrf_enable_aa(unsigned char which_pipe,bool enable)
{
	unsigned char status;
	
	if(which_pipe>5)
		which_pipe = 5;
	status = nrf_set_reg_bit(NRF_REG_EN_AA,which_pipe,enable);
	return status;
}

unsigned char nrf_enable_rx_pipe(unsigned char which_pipe,bool enable)
{
	unsigned char status;
	
	if(which_pipe>5)
		which_pipe = 5;
	status = nrf_set_reg_bit(NRF_REG_EN_RXADDR,which_pipe,enable);
	return status;
}
unsigned char nrf_set_rf_channel(unsigned char channel)
{
	unsigned char status;
	channel &= 0x7f;
	status = nrf_write_reg(NRF_REG_RF_CH,channel);
	return status;
}
unsigned char nrf_rf_set(bool const_wave,nrf_data_rate_def data_rate)
{
	unsigned char temp,status;
	if(const_wave)
	{
		temp = 0x80;
	}
	else
	{
		temp = 0x00;
	}
	temp |= data_rate;
	
	status = nrf_write_reg(NRF_REG_RF_SETUP,temp);
	return status;
}

unsigned char nrf_set_retr(unsigned char retr_delay_nx250us,unsigned char retr_count)
{
	unsigned char status;
	
	retr_delay_nx250us &= 0x0f;
	retr_count &= 0x0f;
	retr_delay_nx250us = (retr_delay_nx250us<<4)|retr_count;
	status = nrf_write_reg(NRF_REG_SETUP_RETR,retr_delay_nx250us);
	
	return status;
}

unsigned char nrf_set_config(unsigned char config)
{
	unsigned char status;
	
	status = nrf_write_reg(NRF_REG_CONFIG,config);
	
	return status;	
}
unsigned char nrf_flush_tx(void)
{
	unsigned char status;
	
	NRF_SPI_CSN_L();
	status = spi_read_write(NRF_CMD_FLUSH_TX);
	NRF_SPI_CSN_H();
	return status;	
}
unsigned char nrf_flush_rx(void)
{
	unsigned char status;
	
	NRF_SPI_CSN_L();
	status = spi_read_write(NRF_CMD_FLUSH_RX);
	NRF_SPI_CSN_H();
	return status;	
}

unsigned char nrf_reuse_tx_pl(void)
{
	unsigned char status;
	
	NRF_SPI_CSN_L();
	status = spi_read_write(NRF_CMD_REUSE_TX_PL);
	NRF_SPI_CSN_H();
	return status;	
}

// Flush RX FIFO if the read value is larger than 32 bytes
unsigned char nrf_read_rx_pl_width(unsigned char *out_width)
{
	unsigned char status;
	NRF_SPI_CSN_L();
	status = spi_read_write(NRF_CMD_R_RX_PL_WID);
	*out_width = spi_read_write(NRF_CMD_NOP);
	NRF_SPI_CSN_H();
	
	return status;	
}

unsigned char nrf_fill_tx_buff(const unsigned char *pdata,unsigned char num)
{
	unsigned char status;
	if(num>32)
	{
		num = 32;
	}
	
	NRF_SPI_CSN_L();
	status = spi_read_write(NRF_CMD_W_TX_PL);
	while(num--)
	{
		spi_read_write(*pdata++);
	}
	NRF_SPI_CSN_H();
	
	return status;
}

/*used in rx mode
Used in RX mode.
Write Payload to be transmitted together with 
ACK packet on PIPE PPP. (PPP valid in the 
range from 000 to 101)
*/
unsigned char nrf_fill_ack_pl(unsigned char pipe,const unsigned char *pdata,unsigned char num)
{
	unsigned char status;
	if(num>32)
	{
		num = 32;
	}
	
	NRF_SPI_CSN_L();
	status = spi_read_write(NRF_CMD_W_ACK_PL(pipe));
	while(num--)
	{
		spi_read_write(*pdata++);
	}
	NRF_SPI_CSN_H();
	
	return status;
}
unsigned char nrf_fill_no_ack_pl(const unsigned char *pdata,unsigned char num)
{
	unsigned char status;
	if(num>32)
	{
		num = 32;
	}
	
	NRF_SPI_CSN_L();
	status = spi_read_write(NRF_CMD_W_TX_PL_NO_ACK);
	while(num--)
	{
		spi_read_write(*pdata++);
	}
	NRF_SPI_CSN_H();
		
	return status;
}

unsigned char nrf_read_rx_buff(unsigned char *pdata,unsigned char num)
{
	unsigned char status;
	if(num>32)
	{
		num = 32;
	}
	
	NRF_SPI_CSN_L();
	status = spi_read_write(NRF_CMD_R_RX_PL);
	while(num--)
	{
		*pdata++ = spi_read_write(NRF_CMD_NOP);
	}
	NRF_SPI_CSN_H();
		
	return status;
}

unsigned char nrf_set_dynpd(unsigned char pipe)
{
	unsigned char status;
	
	status = nrf_write_reg(NRF_REG_DYNPD,pipe);
	
	return status;
}

unsigned char nrf_set_feature(unsigned char feature)
{
	unsigned char status;
	
	status = nrf_write_reg(NRF_REG_FEATURE,feature);
	
	return status;
}
//end

//default setting:
//rx use: pipe 0 
//address width 5bytes
//address tx=rx=
//payload len: 32bytes
//data rate 1Mbps
//auto ack: disable
//rf channel: 0
//crc byte: 2bytes
//crc: enable
//power down

void nrf_defaut_init(void)
{
	const unsigned char addr[RADIO_ADDRESS_WIDTH]={0x68,0x94,0xa6};//0x01,0x02,0x03,0x04,0x05};
	nrf_set_address_width(nrf_addr_width_3byte);
	nrf_set_rx_address(nrf_reg_rxaddr_pipe0,addr,RADIO_ADDRESS_WIDTH);
	nrf_set_tx_address(addr,RADIO_ADDRESS_WIDTH); 
	nrf_set_rx_pl_num(nrf_reg_rx_pw_pipe0,32);
	nrf_enable_rx_pipe(0,true);
	nrf_enable_aa(0,true);
	nrf_set_rf_channel(0x1c);
	nrf_rf_set(false,nrf_data_rate_250K);
	nrf_set_retr(0xff,0xff);

	nrf_set_dynpd(0x01);
	nrf_set_feature(0x07);

	nrf_set_config(NRF_CONFIG_BIT_EN_CRC|NRF_CONFIG_BIT_CRC_BYTE2|NRF_CONFIG_BIT_PWR_UP);
	nrf_flush_tx();
	nrf_flush_rx();
	nrf_clear_status(NRF_STATUS_ALL_FLAG);
}

void nrf24l01_init(void)
{
	NRF_CE_L();
	NRF_SPI_CSN_H();
	nrf_hard_init();
	nrf_defaut_init();
}

/**********************************************************
hal layer :this is a radio link
1) it can transmit data
2) when it receiced data, report it
3) we can config its tx address and rx address 
4) data rate is configurable

**********************************************************/
void radio_config(radio_config_def *config)
{
	if(config->in_rx_mode)
	{//in rx mode
		nrf_set_config(NRF_CONFIG_BIT_EN_CRC|NRF_CONFIG_BIT_CRC_BYTE2|NRF_CONFIG_BIT_PWR_UP|NRF_CONFIG_BIT_PRX);
	}
	else
	{//in tx mode
		nrf_set_config(NRF_CONFIG_BIT_EN_CRC|NRF_CONFIG_BIT_CRC_BYTE2|NRF_CONFIG_BIT_PWR_UP);
	}
	if(config->en_aa)
	{
		nrf_enable_aa(0,true);
	}
	else
	{
		nrf_enable_aa(0,false);
	}
	nrf_set_address_width(config->addr_width);
	nrf_rf_set(false,config->data_rate);
	nrf_set_tx_address(config->tx_addr,config->addr_width);
	//set rx_address to tx_address so we can receive ack data
	nrf_set_rx_address(nrf_reg_rxaddr_pipe0,config->tx_addr,config->addr_width);
	
	nrf_set_rf_channel(config->rf_channel);
	nrf_rx_ok_indicate = config->rx_callback;
	nrf_tx_ok_indicate = config->tx_complete_call_back;
}

int radio_send_package(const unsigned char *pdata,unsigned char num)
{
	char temp;
	
	temp = nrf_read_fifo_status();
	if(temp&(1<<5))//TX fifo full
	{
		return 0;
	}
	nrf_fill_tx_buff(pdata,num);
	return 1;
}

int radio_send_respond_package(const unsigned char *pdata,unsigned char num)
{
	char temp;
	
	temp = nrf_read_fifo_status();
	if(temp&(1<<5))//TX fifo full
	{
		return 0;
	}
	
	nrf_fill_ack_pl(0,pdata,num);
	
	return 1;
}

bool radio_is_tx_fifo_full(void)
{
	char temp;
	temp = nrf_read_fifo_status();
	if(temp&(1<<5))//TX fifo full
	{
		return true;
	}
	return false;
}


int radio_flush_tx(void)
{;
	nrf_flush_tx();
	return 1;
}
int radio_flush_rx(void)
{
	nrf_flush_rx();
	return 1;
}
void radio_init(void)
{
	nrf24l01_init();
}


void radio_power_on(void)
{
	nrf_rf_power_on();
}
void radio_power_off(void)
{
	nrf_rf_power_off();
}

void radio_check(void)
{	
	unsigned char status,width;
	
	if(flag_interrupt)
	{
		flag_interrupt = 0;
		//status = nrf_read_status();
		
		status = nrf_read_status();
		NRF_IRQ_EN();//enbale interrupt
		
		if(status&NRF_STATUS_BIT_TX_DS)
		{
			//data has sucessfully transmitted
			if(nrf_tx_ok_indicate)
				nrf_tx_ok_indicate(radio_tx_ok);//ok
		}
		status = nrf_read_status();
		if(status&NRF_STATUS_BIT_RX_DR)
		{
			//nrf_clear_status(NRF_STATUS_BIT_RX_DR);
			char temp;
			nrf_read_rx_pl_width(&width);
			if(width > 32 || width==0)
			{
				//error
				nrf_flush_rx();
			}
			else
			{
				do
				{
					nrf_read_rx_buff(nrf_read_buff,width);
					if(nrf_rx_ok_indicate)
					{// process data
						nrf_rx_ok_indicate(nrf_read_buff,width);
					}
					nrf_clear_status(NRF_STATUS_BIT_RX_DR);
					temp = nrf_read_fifo_status();
				}while((temp&0x01) == 0);//fifo have data
			}
		}
		status = nrf_clear_status(NRF_STATUS_ALL_FLAG);
		if(status&NRF_STATUS_BIT_MAX_RT)
		{
			nrf_flush_tx();
			if(nrf_tx_ok_indicate)
				nrf_tx_ok_indicate(radio_tx_error);//error
		}
	}
}
void EXTI4_15_IRQHandler(void)
{
	if (EXTI_GetITStatus(EXTI_Line4)==SET)
  {
		flag_interrupt = 1;
    EXTI_ClearITPendingBit(EXTI_Line4);
  }
}
