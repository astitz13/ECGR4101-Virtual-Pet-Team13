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

// Include necessary HAL functions and typedefs
#include "stm32l4xx_hal.h"

// Include timer PeriodElapsedCallback handler shared with microphone
#include "timers_common.h"


// Function to initialize both timers (TIM16 and TIM17) for audio
void initAudioTimers(void);
// Individual functions to initialize TIM16 and TIM17
void MX_TIM16_Init(void);
void MX_TIM17_Init(void);
// Initialization for GPIO pins necessary for PWM on TIM16
void HAL_TIM_PWM_MspInit(TIM_HandleTypeDef* htim_pwm);
// Callback and request handler for TIM17
void TIM17_PeriodElapsedCallback(TIM_HandleTypeDef *htim);
void TIM1_TRG_COM_TIM17_IRQHandler(void);

// Start audio with pointer to audio buffer and number of samples in audio
void startAudio(int8_t *audio, size_t num_samples);
// Stops all audio
void stopAudio(void);

// Internal function to adjust PWM duty cycle
void adjustDutyCycle(float dutyCycle);


#endif // AUDIO_H