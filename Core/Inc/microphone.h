#ifndef MICROPHONE_H
#define MICROPHONE_H

#include "stm32l496xx.h"
#include "stm32l4xx_hal.h"

// Sample 0.1 s = 100 ms at 16 kHz
#define MICROPHONE_SAMPLES 1600

#define MICROPHONE_TIMER_PSC 49
#define MICROPHONE_TIMER_PERIOD 99

void initMicrophone();
void MX_ADC1_Init(void);
void MX_TIM15_Init(void);
void TIM15_PeriodElapsedCallback(TIM_HandleTypeDef *htim);
void TIM1_BRK_TIM15_IRQHandler(void);

int getMicrophoneData();
float getMicrophoneDataCentered();

void startRecording();
void stopRecording();

float getLatestMicrophoneData();
float getMaxMicrophoneData();
float getMinMicrophoneData();
float getExtremeMicrophoneData();

void getRecordedMicrophoneData(float *out, size_t length);
void clearRecordedMicrophoneData(void);

#endif // MICROPHONE_H