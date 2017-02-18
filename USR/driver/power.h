#ifndef _POWER_H_
#define _POWER_H_


#define LED_RED_ON()	GPIOA->BRR = GPIO_Pin_1
#define LED_RED_OFF()	GPIOA->BSRR = GPIO_Pin_1
#define LED_RED_SWITCH()	GPIOA->ODR ^= GPIO_Pin_1

#define LED_GREEN_ON()	GPIOA->BRR = GPIO_Pin_0
#define LED_GREEN_OFF()	GPIOA->BSRR = GPIO_Pin_0
#define LED_GREEN_SWITCH()	GPIOA->ODR ^= GPIO_Pin_0

#define LED_SWITCH()
#define LED_ON()
#define LED_OFF()
void power_init(void);

#endif
