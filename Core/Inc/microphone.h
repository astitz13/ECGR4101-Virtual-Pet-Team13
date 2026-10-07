#ifndef MICROPHONE_H
#define MICROPHONE_H

// Import necessary STM32 dependencies and HAL functions/typedefs
#include "stm32l496xx.h"
#include "stm32l4xx_hal.h"

// Sample 0.1 s = 100 ms at 16 kHz
#define MICROPHONE_SAMPLES 1600
// Since TIM15 runs at 80 MHz, this enables sampling at 16 kHz
#define MICROPHONE_TIMER_PSC 49
#define MICROPHONE_TIMER_PERIOD 99

// Initializes ADC and TIM15 for microphone
void initMicrophone(void);
// Individual initializations of ADC3 and TIM15
void MX_ADC3_Init(void);
void MX_TIM15_Init(void);
// Init for ADC GPIO pins
void HAL_ADC_MspInit(ADC_HandleTypeDef* adcHandle);
void HAL_ADC_MspDeInit(ADC_HandleTypeDef* adcHandle);
// Callback and request handler for TIM15
void TIM15_PeriodElapsedCallback(TIM_HandleTypeDef *htim);
void TIM1_BRK_TIM15_IRQHandler(void);

// Get raw microphone data from ADC
int getMicrophoneData();
// Get microphone data adjusted to range of -1 to 1
float getMicrophoneDataCentered();

// Start recording samples from microphone on timer
void startRecording();
// Stop recording samples
void stopRecording();

// Get latest added sample from samples array
float getLatestMicrophoneData();
// Get maximum value across last 100 ms of samples
float getMaxMicrophoneData();
// Get minimum value across last 100 ms
float getMinMicrophoneData();
// Get value of maximum absolute value across last 100 ms
float getExtremeMicrophoneData();

// Copy certain number of samples to output array
// This does flatten the circular array into a linear array ending at the current sample
void getRecordedMicrophoneData(float *out, size_t length);
// Clear microphone samples
void clearRecordedMicrophoneData(void);

#endif // MICROPHONE_H