#include "microphone.h"
// Import required HAL functions and typedefs
#include "stm32l496xx.h"
#include "stm32l4xx_hal_def.h"
#include "stm32l4xx_hal_gpio.h"
#include "stm32l4xx_hal_rcc.h"
// Importing main required for error handler
#include "main.h"
// Utils includes absolute value function
#include "utils.h"

// Keep track of samples
float microphone_samples[MICROPHONE_SAMPLES];
size_t curr_sample_idx = 0;

// Handlers for ADC3 and TIM15
ADC_HandleTypeDef hadc3;
TIM_HandleTypeDef htim15;

// Initializes ADC and TIM15 for microphone
void initMicrophone(void) {
    MX_ADC3_Init();
    MX_TIM15_Init();
}

// Initialize ADC3 on channel 6
void MX_ADC3_Init(void) 
{
    ADC_ChannelConfTypeDef sConfig = {0};

    hadc3.Instance = ADC3;

    hadc3.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV2;
    hadc3.Init.Resolution = ADC_RESOLUTION_12B;
    hadc3.Init.DataAlign = ADC_DATAALIGN_RIGHT;
    hadc3.Init.ScanConvMode = ADC_SCAN_DISABLE;
    hadc3.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
    hadc3.Init.LowPowerAutoWait = DISABLE;
    hadc3.Init.ContinuousConvMode = DISABLE;
    hadc3.Init.NbrOfConversion = 1;
    hadc3.Init.DiscontinuousConvMode = DISABLE;
    hadc3.Init.ExternalTrigConv = ADC_SOFTWARE_START;
    hadc3.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
    hadc3.Init.Overrun = ADC_OVR_DATA_OVERWRITTEN;
    hadc3.Init.OversamplingMode = DISABLE;

    if (HAL_ADC_Init(&hadc3) != HAL_OK) {
        Error_Handler();
    }

    sConfig.Channel = ADC_CHANNEL_6;
    sConfig.Rank = ADC_REGULAR_RANK_1;
    sConfig.SamplingTime = ADC_SAMPLETIME_47CYCLES_5;
    sConfig.SingleDiff = ADC_SINGLE_ENDED;
    sConfig.OffsetNumber = ADC_OFFSET_NONE;
    sConfig.Offset = 0;

    if (HAL_ADC_ConfigChannel(&hadc3, &sConfig) != HAL_OK) {
        Error_Handler();
    }

    if (HAL_ADCEx_Calibration_Start(&hadc3, ADC_SINGLE_ENDED) != HAL_OK) {
        Error_Handler();
    }
}

// Init for HAL GPIO pins
void HAL_ADC_MspInit(ADC_HandleTypeDef* adcHandle) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    if(adcHandle->Instance == ADC3) {
        __HAL_RCC_ADC_CLK_ENABLE();

        // Set PF3 to use ADC3 Channel 6
        __HAL_RCC_GPIOF_CLK_ENABLE();
        GPIO_InitStruct.Pin = GPIO_PIN_3;
        GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);
    }
}

// Deinit for ADC3
void HAL_ADC_MspDeInit(ADC_HandleTypeDef* adcHandle) {
    if(adcHandle->Instance == ADC3) {
        __HAL_RCC_ADC_CLK_DISABLE();
        HAL_GPIO_DeInit(GPIOF, GPIO_PIN_3);
    }
}

// Initialize TIM15 for microphone sampling
void MX_TIM15_Init(void)
{
    __HAL_RCC_TIM15_CLK_ENABLE();
    HAL_NVIC_SetPriority(TIM1_BRK_TIM15_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(TIM1_BRK_TIM15_IRQn);

    htim15.Instance = TIM15;
    htim15.Init.Prescaler = MICROPHONE_TIMER_PSC;
    htim15.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim15.Init.Period = MICROPHONE_TIMER_PERIOD;
    htim15.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim15.Init.RepetitionCounter = 0;
    htim15.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    
    if (HAL_TIM_Base_Init(&htim15) != HAL_OK) {
        Error_Handler();
    }
}

// Callback for TIM15 to add sampled data
void TIM15_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM15){
        // Collect microphone data and add to circular array
        float current_data = getMicrophoneDataCentered();
        microphone_samples[curr_sample_idx] = current_data;
        curr_sample_idx = (curr_sample_idx + 1) % MICROPHONE_SAMPLES;
    }
}

// Use HAL IRQHandler for timers
void TIM1_BRK_TIM15_IRQHandler(void)
{
  HAL_TIM_IRQHandler(&htim15);
}

// Get raw microphone data from ADC
int getMicrophoneData() {
    // Default to -1 if error
    int result = -1;

    HAL_ADC_Start(&hadc3);

    HAL_StatusTypeDef status = HAL_ADC_PollForConversion(&hadc3, 10U);
    if (status == HAL_OK) {
        // If value available, read ADC value
        result = HAL_ADC_GetValue(&hadc3);
    }

    HAL_ADC_Stop(&hadc3);

    return result;
}

// Get microphone data adjusted to range of -1 to 1
float getMicrophoneDataCentered() {
    int data_int = getMicrophoneData();

    return (data_int - 2048.0f) / 2048.0f; // Converts 0 to 4095 to -1 to 1
}

// Begin TIM15 to start recording
void startRecording()
{
    HAL_TIM_Base_Start_IT(&htim15);
}

// Stop TIM15 to stop recording
void stopRecording() {
    HAL_TIM_Base_Stop_IT(&htim15);
}

// Get latest sampled value of microphone
float getLatestMicrophoneData() {
    return microphone_samples[curr_sample_idx];
}

// Get maximum sampled value of microphone
float getMaxMicrophoneData() {
    float maxVal = -1;
    for (size_t i = 0; i < MICROPHONE_SAMPLES; i++) {
        if (microphone_samples[i] > maxVal) {
            maxVal = microphone_samples[i];
        }
    }
    return maxVal;
}

// Get minimum sampled value of microphone
float getMinMicrophoneData() {
    float minVal = 1;
    for (size_t i = 0; i < MICROPHONE_SAMPLES; i++) {
        if (microphone_samples[i] < minVal) {
            minVal = microphone_samples[i];
        }
    }
    return minVal;
}

// Get value with largest absolute value
float getExtremeMicrophoneData() {
    float maxVal = 1;
    for (size_t i = 0; i < MICROPHONE_SAMPLES; i++) {
        if (abs(microphone_samples[i]) > abs(maxVal)) {
            maxVal = microphone_samples[i];
        }
    }
    return maxVal;
}

// Copy certain number of samples to output array
// This does flatten the circular array into a linear array ending at the current sample
void getRecordedMicrophoneData(float *out, size_t length) {
    // Store current sample index in case it changes due to timer interrupts while processing
    size_t idx = curr_sample_idx;
    // Iterate through minimum of length and MICROPHONE_SAMPLES
    for (size_t i = 0; i < length && i < MICROPHONE_SAMPLES; i++) {
        // Start at curr_sample_idx and go back once each time, wrapping around circular array
        size_t tmp_idx = (idx - i + MICROPHONE_SAMPLES) % MICROPHONE_SAMPLES;
        // Add latest samples at end of output buffer and older samples at beginning
        out[length - i - 1] = microphone_samples[tmp_idx];
    }
}

// Clear microphone samples
void clearRecordedMicrophoneData(void) {
    for (size_t i = 0; i < MICROPHONE_SAMPLES; i++) {
        microphone_samples[i] = 0;
    }
}
