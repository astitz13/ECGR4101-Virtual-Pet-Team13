#ifndef AUDIO_H
#define AUDIO_H

// Clock for TIM16 driven at 80 MHz
// Sampling rate at 16 kHz
// PWM driven at 160 kHz - 80,000,000 / (49+1) / (9 + 1) = 160,000
#define AUDIO_TIMER_PSC 0
#define AUDIO_TIMER_PERIOD 499
// Sampling rate at 16 kHz - 80,000,000 / (49 + 1) / (99 + 1) = 16,000
#define AUDIO_TIMER_SAMPLE_PSC 49
#define AUDIO_TIMER_SAMPLE_PERIOD 99

#include "stm32l4xx_hal.h"

void HAL_TIM_PWM_MspInit(TIM_HandleTypeDef* htim_pwm);

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim);

void initAudioTimers();

void MX_TIM16_Init(void);

void MX_TIM17_Init(void);

void startAudio(int8_t *audio, size_t num_samples);

void stopAudio(void);

void adjustDutyCycle(float dutyCycle);

void TIM1_TRG_COM_TIM17_IRQHandler(void);

#endif // AUDIO_H