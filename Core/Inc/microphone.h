#ifndef MICROPHONE_H
#define MICROPHONE_H

#include "stm32l496xx.h"
#include "stm32l4xx_hal.h"

void MX_ADC1_Init(void);
int getMicrophoneData();
float getMicrophoneDataCentered();


#endif // MICROPHONE_H