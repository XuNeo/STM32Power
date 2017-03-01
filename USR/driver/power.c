#include "power.h"
static char flag_power_on = 0;
void timer_init(void)
{
	TIM_TimeBaseInitTypeDef TIM_InitStructure;
	NVIC_InitTypeDef NVIC_InitStruct;
//general timer	
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3,ENABLE);
	
	TIM_InitStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_InitStructure.TIM_Prescaler = 2000-1;//96MHz/2000 = 48KHz
	TIM_InitStructure.TIM_Period = 48000-1;//
	TIM_InitStructure.TIM_RepetitionCounter = 0;
	TIM_InitStructure.TIM_ClockDivision = TIM_CKD_DIV1;
	
	TIM_TimeBaseInit(TIM3,&TIM_InitStructure);
	
	TIM_ITConfig(TIM3,TIM_IT_Update,ENABLE);
	
	NVIC_InitStruct.NVIC_IRQChannel = TIM3_IRQn;
	NVIC_InitStruct.NVIC_IRQChannelPriority = 0;
	NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
	
	NVIC_Init(&NVIC_InitStruct);
	
	TIM_Cmd(TIM3,ENABLE);
	
}

void pwm_init(void)
{
	#define TIMCLK 64000000
	#define PWM_FREQ 50000
	#define PWM_PERIOD_CODE (TIMCLK/PWM_FREQ-1)
	#define PWM_HALFPERIOD_CODE (TIMCLK/PWM_FREQ/2-1)
	
	GPIO_InitTypeDef GPIO_InitStructure;
	TIM_TimeBaseInitTypeDef TIM_InitStructure;
  TIM_OCInitTypeDef  TIM_OCInitStructure;
	TIM_BDTRInitTypeDef TIM_BDTRInitStruct;
//GPIO	
  RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOA|RCC_AHBPeriph_GPIOB, ENABLE);
	GPIO_ResetBits(GPIOA,GPIO_Pin_10);
	GPIO_ResetBits(GPIOB,GPIO_Pin_1);
	//PA10--PWM,TIM1_CH3, PB1-->#PWM,TIM1_CH3N
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10|GPIO_Pin_9;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP ;
  GPIO_Init(GPIOA, &GPIO_InitStructure);
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;
  GPIO_Init(GPIOB, &GPIO_InitStructure); 
	GPIO_ResetBits(GPIOA,GPIO_Pin_10);
	GPIO_ResetBits(GPIOB,GPIO_Pin_1);
  
  GPIO_PinAFConfig(GPIOA, GPIO_PinSource10, GPIO_AF_2);
  GPIO_PinAFConfig(GPIOA, GPIO_PinSource9, GPIO_AF_2);
  GPIO_PinAFConfig(GPIOB, GPIO_PinSource1, GPIO_AF_2);
//PWM timer
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM1,ENABLE);
	TIM_InitStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_InitStructure.TIM_Prescaler = 0;//96MHz/1 = 96MHz
	TIM_InitStructure.TIM_Period = TIMCLK/PWM_FREQ - 1;//
	TIM_InitStructure.TIM_RepetitionCounter = 0;
	TIM_InitStructure.TIM_ClockDivision = TIM_CKD_DIV1;
	TIM_TimeBaseInit(TIM1,&TIM_InitStructure);
	//TIM_ITConfig(TIM1,TIM_IT_Update,ENABLE);
	//NVIC_InitStruct.NVIC_IRQChannel = TIM1_BRK_UP_TRG_COM_IRQn;
	//NVIC_Init(&NVIC_InitStruct);
	
	  /* Channel 1, 2,3 and 4 Configuration in PWM mode */
  TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
  TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
  TIM_OCInitStructure.TIM_OutputNState = TIM_OutputNState_Enable;
  TIM_OCInitStructure.TIM_Pulse = 0;
  TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
  TIM_OCInitStructure.TIM_OCNPolarity = TIM_OCNPolarity_High;
  TIM_OCInitStructure.TIM_OCIdleState = TIM_OCIdleState_Set;
  TIM_OCInitStructure.TIM_OCNIdleState = TIM_OCIdleState_Reset;

  TIM_OC3Init(TIM1, &TIM_OCInitStructure);
	TIM_OC3PreloadConfig(TIM1,TIM_OCPreload_Enable);
	
  /* Output Compare PWM Mode configuration */         
  TIM_OCInitStructure.TIM_Pulse = 1;
  TIM_OC4Init(TIM1, &TIM_OCInitStructure);
	TIM_OC4PreloadConfig(TIM1,TIM_OCPreload_Enable);
  TIM_OC2Init(TIM1, &TIM_OCInitStructure);//for wathing the trigger signal.
	TIM_OC2PreloadConfig(TIM1,TIM_OCPreload_Enable);
  
	TIM_BDTRInitStruct.TIM_OSSRState = TIM_OSSRState_Disable;
  TIM_BDTRInitStruct.TIM_OSSIState = TIM_OSSIState_Disable;
  TIM_BDTRInitStruct.TIM_LOCKLevel = TIM_LOCKLevel_OFF;
  TIM_BDTRInitStruct.TIM_DeadTime = 0x08;
  TIM_BDTRInitStruct.TIM_Break = TIM_Break_Disable;
  TIM_BDTRInitStruct.TIM_BreakPolarity = TIM_BreakPolarity_Low;
  TIM_BDTRInitStruct.TIM_AutomaticOutput = TIM_AutomaticOutput_Disable;
	TIM_BDTRConfig(TIM1,&TIM_BDTRInitStruct);
  /* TIM1 counter enable */
  TIM_Cmd(TIM1, ENABLE);
  /* TIM1 Main Output Enable */
  TIM_CtrlPWMOutputs(TIM1, ENABLE);
}


void adc_init(void)
{
	extern uint16_t adc_buffer[];
  DMA_InitTypeDef   DMA_InitStructure;
	ADC_InitTypeDef     ADC_InitStructure;
  GPIO_InitTypeDef    GPIO_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
  /* ADC1 DeInit */  
  ADC_DeInit(ADC1);
  
	//PA5-->ADC_I, PA6-->ADC_V
  RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOA, ENABLE);
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);
  
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5|GPIO_Pin_6 ;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AN;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL ;
  GPIO_Init(GPIOA, &GPIO_InitStructure);
  
  ADC_StructInit(&ADC_InitStructure);
  
  /* Configure the ADC1 in continuous mode withe a resolution equal to 12 bits  */
  ADC_InitStructure.ADC_Resolution = ADC_Resolution_12b;
  ADC_InitStructure.ADC_ContinuousConvMode = DISABLE; 
  ADC_InitStructure.ADC_ExternalTrigConvEdge = ADC_ExternalTrigConvEdge_Falling;
	ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_T1_CC4;
  ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
  ADC_InitStructure.ADC_ScanDirection = ADC_ScanDirection_Upward;
  ADC_Init(ADC1, &ADC_InitStructure); 
	ADC_ClockModeConfig(ADC1,ADC_ClockMode_SynClkDiv4);//ADC clock = 48MHz/4 = 12MHz
	
  ADC_ChannelConfig(ADC1, ADC_Channel_5 , ADC_SampleTime_7_5Cycles); 
  ADC_ChannelConfig(ADC1, ADC_Channel_6 , ADC_SampleTime_7_5Cycles);
  ADC_GetCalibrationFactor(ADC1);
  
  /* ADC DMA request in circular mode */
  ADC_DMARequestModeConfig(ADC1, ADC_DMAMode_Circular);
  
  /* Enable ADC_DMA */
  ADC_DMACmd(ADC1, ENABLE);  
  
  /* Enable the ADC peripheral */
  ADC_Cmd(ADC1, ENABLE);     
  
  /* Wait the ADRDY flag */
  while(!ADC_GetFlagStatus(ADC1, ADC_FLAG_ADRDY)); 
  
  /* ADC1 regular Software Start Conv */ 
  ADC_StartOfConversion(ADC1);
	
  /* DMA1 clock enable */
  RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1 , ENABLE);
  
  /* DMA1 Channel1 Config */
  DMA_DeInit(DMA1_Channel1);
  DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)(&ADC1->DR);
  DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t)adc_buffer;
  DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;
  DMA_InitStructure.DMA_BufferSize = 2;
  DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
  DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
  DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;
  DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;
  DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;
  DMA_InitStructure.DMA_Priority = DMA_Priority_High;
  DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
  DMA_Init(DMA1_Channel1, &DMA_InitStructure);
  /* DMA1 Channel1 enable */
  DMA_Cmd(DMA1_Channel1, ENABLE);
	
  /* Enable DMA1 Channel1 Transfer Complete interrupt */
  DMA_ITConfig(DMA1_Channel1, DMA_IT_TC, ENABLE);

  /* Enable DMA1 channel1 IRQ Channel */
  NVIC_InitStructure.NVIC_IRQChannel = DMA1_Channel1_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);
}


#define Qn 24
#define Q12toQ24(n) (((int32_t)n) << 12)
#define Q24(n) ((int32_t)(n*(1<<24)))

typedef int32_t q24_t;

__inline void pwm_set_duty(q24_t duty)
{
  TIM1->CCR3=((int64_t)duty*PWM_PERIOD_CODE)>>Qn;
  
  if(duty>Q24(0.5))
	{
		TIM1->CCR4 = ((int64_t)duty*(PWM_HALFPERIOD_CODE))>>Qn;
		TIM1->CCR2 = TIM1->CCR4;
	}
  else 
	{
		TIM1->CCR4 = ((int64_t)(Q24(1.0)+duty)*(PWM_HALFPERIOD_CODE))>>Qn;
		TIM1->CCR2 = TIM1->CCR4;
	}
}

#define KP_I Q24(0.8)//1.2----------1.38
#define KI_I Q24(0.05)//0.15---------0.092
#define KD_I Q24(0)//Q24(3.5)//3----------------5.08
#define MAX_I Q24(0.70)
#define MIN_I Q24(0.01)

#define KP_U Q24(0.3)//4.18
#define KI_U Q24(0.09)//0.182//
#define KD_U Q24(4.5)//18.5//6.6//
#define MAX_U Q24(0.97)
#define MIN_U Q24(0.05)

typedef struct
{
	q24_t A0;            /**< The derived gain, A0 = Kp + Ki + Kd . */
	q24_t A1;            /**< The derived gain, A1 = -Kp - 2Kd. */
	q24_t A2;            /**< The derived gain, A2 = Kd . */
	q24_t error[3];      /**< The state array of length 3. */
	q24_t out;
}q24_pid_def;

q24_t q24_volt,q24_curr;
q24_pid_def q24_pid_volt,q24_pid_curr;
q24_t q24_volt_target = Q24(0.1);

uint16_t adc_buffer[2];

void DMA1_Channel1_IRQHandler(void)
{
	DMA_ClearITPendingBit(DMA1_IT_TC1);
	if(flag_power_on == 0)
		return;
	LED_GREEN_ON();
	q24_volt = Q12toQ24(adc_buffer[1]);
	q24_curr = Q12toQ24(adc_buffer[0]);
	
  q24_pid_volt.error[0] = q24_volt_target- q24_volt;
  q24_pid_volt.out = q24_pid_volt.out+\
				(((int64_t)q24_pid_volt.A0*q24_pid_volt.error[0]+\
				(int64_t)q24_pid_volt.A1*q24_pid_volt.error[1]+\
				(int64_t)q24_pid_volt.A2*q24_pid_volt.error[2])>>Qn);
  q24_pid_volt.error[2]=q24_pid_volt.error[1];
  q24_pid_volt.error[1]=q24_pid_volt.error[0];
  if(q24_pid_volt.out>MAX_U) q24_pid_volt.out=MAX_U;
  if(q24_pid_volt.out<MIN_U) q24_pid_volt.out=MIN_U;
	
	q24_pid_curr.error[0] = Q24(0.05)- q24_curr;
  q24_pid_curr.out = q24_pid_curr.out+\
				(((int64_t)q24_pid_curr.A0*q24_pid_curr.error[0]+\
				(int64_t)q24_pid_curr.A1*q24_pid_curr.error[1]+\
				(int64_t)q24_pid_curr.A2*q24_pid_curr.error[2])>>Qn);
  q24_pid_curr.error[2]=q24_pid_curr.error[1];
  q24_pid_curr.error[1]=q24_pid_curr.error[0];
  if(q24_pid_curr.out>MAX_I) q24_pid_curr.out=MAX_I;
  if(q24_pid_curr.out<MIN_I) q24_pid_curr.out=MIN_I;
	
	if(q24_pid_curr.out>q24_pid_volt.out)	
		pwm_set_duty(q24_pid_volt.out);
	else
		pwm_set_duty(q24_pid_curr.out);
	LED_GREEN_OFF();
}

void power_init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	
  RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOA, ENABLE);
	//LED-->PA2
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_DOWN;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_Level_2;//10MHz
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1|GPIO_Pin_0;
	GPIO_Init(GPIOA,&GPIO_InitStructure);
	LED_GREEN_OFF();
	LED_RED_OFF();
	//pid parameters init
	q24_pid_volt.A0 = KP_I+KI_I+KD_I;
  q24_pid_volt.A1 = -KP_I-2*KD_I;
  q24_pid_volt.A2 = KD_I;
	q24_pid_volt.error[0] = 0;
	q24_pid_volt.error[1] = 0;
	q24_pid_volt.error[2] = 0;
	q24_pid_volt.out = 0;
	
	q24_pid_curr.A0 = KP_I+KI_I+KD_I;
  q24_pid_curr.A1 = -KP_I-2*KD_I;
  q24_pid_curr.A2 = KD_I;
	q24_pid_curr.error[0] = 0;
	q24_pid_curr.error[1] = 0;
	q24_pid_curr.error[2] = 0;
	q24_pid_curr.out = 0;

	timer_init();
	pwm_init();
	adc_init();
}

#include "xprintf.h"
void power_pid(int p,int i, int d)
{
	q24_t q24_p,q24_i,q24_d;
	q24_p = Q24((float)p/1000);
	q24_i = Q24((float)i/1000);
	q24_d = Q24((float)d/1000);
	//pif parameters init
	q24_pid_volt.A0 = q24_p+q24_i+q24_d;
  q24_pid_volt.A1 = -q24_p-2*q24_i;
  q24_pid_volt.A2 = q24_d;
	q24_pid_volt.error[0] = 0;
	q24_pid_volt.error[1] = 0;
	q24_pid_volt.error[2] = 0;
	q24_pid_volt.out = 0;
	//xPrintf("set pid:%d,%d,%d---0x%04x,0x%04x,0x%04x\n",p,i,d,q24_p,q24_i,q24_d);
}

void power_set(int volt)
{
	q24_volt_target = Q24((float)volt/1000);
}

void power_on(void)
{
//	pwm_set_duty(Q24(0.0f));
//	TIM_CCxCmd(TIM1,TIM_Channel_3,TIM_CCx_Enable);
//	TIM_CCxNCmd(TIM1,TIM_Channel_3,TIM_CCxN_Enable);
//	TIM_SelectOCxM(TIM1,TIM_Channel_3,TIM_OCMode_PWM1);
//  TIM_Cmd(TIM1, ENABLE);
	flag_power_on = 1;
//	q24_pid_volt.error[0] = 0;
//	q24_pid_volt.error[1] = 0;
//	q24_pid_volt.error[2] = 0;
//	q24_pid_volt.out = 0;
}

void power_off(void)
{
	flag_power_on = 0;
	pwm_set_duty(0);
//	TIM_ForcedOC3Config(TIM1,TIM_ForcedAction_InActive);
//  TIM_Cmd(TIM1, DISABLE);
//  TIM1->CCR3= 0;
//	TIM1->CCR4 = 0;
	q24_pid_volt.error[0] = 0;
	q24_pid_volt.error[1] = 0;
	q24_pid_volt.error[2] = 0;
	q24_pid_volt.out = 0;
	
}


