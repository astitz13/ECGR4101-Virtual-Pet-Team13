#ifndef AUDIO_H
#define AUDIO_H

// Clock for TIM16 driven at 80 MHz
#define AUDIO_TIMER_PSC 79
#define AUDIO_TIMER_PERIOD (3816/2)

#include "stm32l4xx_hal.h"

void HAL_TIM_PWM_MspInit(TIM_HandleTypeDef* htim_pwm);

void MX_TIM16_Init(void);

void startAudio(void);

void stopAudio(void);

void adjustDutyCycle(float dutyCycle);


#endif // AUDIO_H