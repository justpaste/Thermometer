#ifndef __TIM3_H
#define __TIM3_H 			   
#include "stm32f0xx.h"
#include "stdbool.h" 

void TIM_Config(vu16 arr,vu16 psc);
extern uint16_t PoweroffCount;
extern uint16_t BuzzerCount;
extern uint16_t KeyModeCount;
extern uint16_t KeyupCount;
extern bool BuzzerFalg;


#endif
