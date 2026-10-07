#ifndef TIMERS_COMMON_H
#define TIMERS_COMMON_H

#include "stm32l4xx_hal.h"

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim);

#endif // TIMERS_COMMON_H