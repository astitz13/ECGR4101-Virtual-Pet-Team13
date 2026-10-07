#include "timers_common.h"

#include "microphone.h"
#include "audio.h"

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  if (htim->Instance == TIM15)
  {
    TIM15_PeriodElapsedCallback(htim);
  }
  
  if (htim->Instance == TIM17)
  {
    TIM17_PeriodElapsedCallback(htim);
  }
}