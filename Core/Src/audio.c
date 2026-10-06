#include "audio.h"
#include "stm32l4xx_hal.h"
#include "main.h"
#include "stm32l4xx_hal_gpio_ex.h"

TIM_HandleTypeDef htim16;

static inline int calculateDutyCycle(float dutyCycle) {
    return (int)((AUDIO_TIMER_PERIOD + 1) * dutyCycle) - 1;
}

void MX_TIM16_Init(void)
{
    TIM_OC_InitTypeDef sConfigOC = {0};
    TIM_BreakDeadTimeConfigTypeDef sBreakDeadTimeConfig = {0};

    // 1. Base Timer Configuration
    htim16.Instance = TIM16;
    htim16.Init.Prescaler = AUDIO_TIMER_PSC;
    htim16.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim16.Init.Period = AUDIO_TIMER_PERIOD;
    htim16.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim16.Init.RepetitionCounter = 0;
    htim16.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    
    //if (HAL_TIM_Base_Init(&htim16) != HAL_OK) {
    //    Error_Handler();
    //}
    
    if (HAL_TIM_PWM_Init(&htim16) != HAL_OK) {
        Error_Handler();
    }

    // 2. PWM Channel 1 Configuration
    sConfigOC.OCMode = TIM_OCMODE_PWM1;
    sConfigOC.Pulse = calculateDutyCycle(0.5);
    sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
    sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
    sConfigOC.OCNPolarity = TIM_OCNPOLARITY_HIGH;
    sConfigOC.OCNIdleState = TIM_OCNIDLESTATE_RESET;
    sConfigOC.OCIdleState = TIM_OCIDLESTATE_RESET;

    if (HAL_TIM_PWM_ConfigChannel(&htim16, &sConfigOC, TIM_CHANNEL_1) != HAL_OK) {
        Error_Handler();
    }

    sBreakDeadTimeConfig.OffStateRunMode = TIM_OSSR_DISABLE;
    sBreakDeadTimeConfig.OffStateIDLEMode = TIM_OSSI_DISABLE;
    sBreakDeadTimeConfig.LockLevel = TIM_LOCKLEVEL_OFF;
    sBreakDeadTimeConfig.DeadTime = 0;
    sBreakDeadTimeConfig.BreakState = TIM_BREAK_DISABLE;
    sBreakDeadTimeConfig.BreakPolarity = TIM_BREAKPOLARITY_HIGH;
    sBreakDeadTimeConfig.BreakFilter = 0;
    sBreakDeadTimeConfig.AutomaticOutput = TIM_AUTOMATICOUTPUT_ENABLE;
    
    if (HAL_TIMEx_ConfigBreakDeadTime(&htim16, &sBreakDeadTimeConfig) != HAL_OK) {
        Error_Handler();
    }
}

void startAudio(void)
{
    HAL_TIM_PWM_Start(&htim16, TIM_CHANNEL_1);
}

void stopAudio(void) {
    HAL_TIM_PWM_Stop(&htim16, TIM_CHANNEL_1);
}

void adjustDutyCycle(float dutyCycle) {
    __HAL_TIM_SET_COMPARE(&htim16, TIM_CHANNEL_1, calculateDutyCycle(dutyCycle));
}

void HAL_TIM_PWM_MspInit(TIM_HandleTypeDef* htim_pwm)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    
    if(htim_pwm->Instance == TIM16)
    {
        /* 1. Enable peripheral clocks */
        __HAL_RCC_GPIOE_CLK_ENABLE();  // Clock for Port E
        __HAL_RCC_TIM16_CLK_ENABLE();  // Clock for Timer 16

        /* 2. Configure PE0 Pin Properties */
        GPIO_InitStruct.Pin = GPIO_PIN_0;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;         // Alternate Function Push-Pull
        GPIO_InitStruct.Pull = GPIO_NOPULL;             // Keep neutral (or pull up/down depending on hardware)
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;    // Can be adjusted to Medium/High based on your frequency
        
        /* 3. Assign Alternate Function (AF4 for TIM16_CH1 on PE0) */
        GPIO_InitStruct.Alternate = GPIO_AF14_TIM16; 
        
        // Initialize GPIOE Pin 0
        HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);
    }
}
