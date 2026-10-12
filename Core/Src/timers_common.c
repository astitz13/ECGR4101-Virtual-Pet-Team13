#include "timers_common.h"

#include "microphone.h"
#include "audio.h"

// Use TIM15 or TIM17 callback based on what timer elapsed
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM15) {
      TIM15_PeriodElapsedCallback(htim);
    }

    if (htim->Instance == TIM17) {
      TIM17_PeriodElapsedCallback(htim);
    }
}