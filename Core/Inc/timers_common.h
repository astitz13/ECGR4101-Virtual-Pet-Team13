#ifndef TIMERS_COMMON_H
#define TIMERS_COMMON_H

// Import required HAL functions and typedefs
#include "stm32l4xx_hal.h"

// Global callback for timers used by microphone and MPU6050
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim);

#endif // TIMERS_COMMON_H