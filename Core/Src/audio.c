#include "audio.h"
// Import necessary HAL functions and typedefs
#include "stm32l4xx_hal.h"
#include "stm32l4xx_hal_gpio_ex.h"
#include "stm32l4xx_hal_tim.h"
// Import main required for error handler
#include "main.h"

// Define handlers for TIM16 and TIM 17
TIM_HandleTypeDef htim16;
TIM_HandleTypeDef htim17;

// Maintains state of audio playing
int8_t *current_sample = NULL;
size_t samples_remaining = 0;

// Internal inline function for calculating duty cycle
static inline int calculateDutyCycle(float dutyCycle)
{
    return (int)((AUDIO_TIMER_PERIOD + 1) * dutyCycle) - 1;
}

// Function to initialize both timers (TIM16 and TIM17) for audio
void initAudioTimers(void)
{
    MX_TIM16_Init();
    MX_TIM17_Init();
}

// Initialize TIM16 for PWM
void MX_TIM16_Init(void)
{
    TIM_OC_InitTypeDef sConfigOC = {0};
    TIM_BreakDeadTimeConfigTypeDef sBreakDeadTimeConfig = {0};

    htim16.Instance = TIM16;
    htim16.Init.Prescaler = AUDIO_TIMER_PSC;
    htim16.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim16.Init.Period = AUDIO_TIMER_PERIOD;
    htim16.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim16.Init.RepetitionCounter = 0;
    htim16.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    
    if (HAL_TIM_PWM_Init(&htim16) != HAL_OK) {
        Error_Handler();
    }

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

// Initialize TIM17 for audio playback
void MX_TIM17_Init(void) {
    // Interrupt Service Routine needs to be registered
    __HAL_RCC_TIM17_CLK_ENABLE();
    HAL_NVIC_SetPriority(TIM1_TRG_COM_TIM17_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(TIM1_TRG_COM_TIM17_IRQn);

    htim17.Instance = TIM17;
    htim17.Init.Prescaler = AUDIO_TIMER_SAMPLE_PSC;
    htim17.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim17.Init.Period = AUDIO_TIMER_SAMPLE_PERIOD;
    htim17.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim17.Init.RepetitionCounter = 0;
    htim17.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    
    if (HAL_TIM_Base_Init(&htim17) != HAL_OK) {
        Error_Handler();
    }
}

// Initialize GPIO for PWM signal
void HAL_TIM_PWM_MspInit(TIM_HandleTypeDef* htim_pwm)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    
    if(htim_pwm->Instance == TIM16) {
        // Enable clocks for GPIOE and TIM16
        __HAL_RCC_GPIOE_CLK_ENABLE();
        __HAL_RCC_TIM16_CLK_ENABLE();

        // Configure PE0 with alternate function
        GPIO_InitStruct.Pin = GPIO_PIN_0;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
        
        // Use TIM16 PWM as alternate function
        GPIO_InitStruct.Alternate = GPIO_AF14_TIM16; 
        
        // Initialize PE0 with previous struct
        HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);
    }
}

// Callback for TIM17 to play samples
void TIM17_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM17) {
        // Check if still playing audio and valid audio loaded
        if (current_sample != NULL && samples_remaining > 0) {
            // Adjust duty cycle based on current amplitude
            adjustDutyCycle(((int)*current_sample + 127) / 255.0f);
            // Go to next sample and keep track of number left
            current_sample++;
            samples_remaining--;
        } else {
            // Stop audio if TIM17 still active and audio done
            stopAudio();
        }
  }
}

// Use HAL IRQHandler for timers
void TIM1_TRG_COM_TIM17_IRQHandler(void)
{
  HAL_TIM_IRQHandler(&htim17);
}

// Start audio with pointer to audio buffer and number of samples in audio
void startAudio(int8_t *audio, size_t num_samples)
{
    // Set sample pointer and number of samples
    current_sample = audio;
    samples_remaining = num_samples;
    // Begin TIM16 and TIM17 for PWM and playback, respectively
    HAL_TIM_PWM_Start(&htim16, TIM_CHANNEL_1);
    HAL_TIM_Base_Start_IT(&htim17);
}

// Stops all audio
void stopAudio(void) {
    // Set no samples left and current sample to null pointer
    current_sample = NULL;
    samples_remaining = 0;
    // Stop TIM16 and TIM17
    HAL_TIM_PWM_Stop(&htim16, TIM_CHANNEL_1);
    HAL_TIM_Base_Stop_IT(&htim17);
}

// Adjust duty cycle of TIM16 dynamically
void adjustDutyCycle(float dutyCycle) {
    __HAL_TIM_SET_COMPARE(&htim16, TIM_CHANNEL_1, calculateDutyCycle(dutyCycle));
}
