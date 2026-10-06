#include "microphone.h"
#include "main.h"
#include "stm32l4xx_hal_def.h"
#include "stm32l4xx_hal_gpio.h"
#include "stm32l4xx_hal_rcc.h"

ADC_HandleTypeDef hadc3;

void MX_ADC1_Init(void) {


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

    if (HAL_ADC_Init(&hadc3) != HAL_OK)
    {
        Error_Handler();
    }

    sConfig.Channel = ADC_CHANNEL_6;
    sConfig.Rank = ADC_REGULAR_RANK_1;
    sConfig.SamplingTime = ADC_SAMPLETIME_47CYCLES_5;
    sConfig.SingleDiff = ADC_SINGLE_ENDED;
    sConfig.OffsetNumber = ADC_OFFSET_NONE;
    sConfig.Offset = 0;

    if (HAL_ADC_ConfigChannel(&hadc3, &sConfig) != HAL_OK)
    {
        Error_Handler();
    }

    if (HAL_ADCEx_Calibration_Start(&hadc3, ADC_SINGLE_ENDED) != HAL_OK)
    {
        Error_Handler();
    }
}

int getMicrophoneData() {
    int result = -1;

    HAL_ADC_Start(&hadc3);

    HAL_StatusTypeDef status = HAL_ADC_PollForConversion(&hadc3, 10U);
    if (status == HAL_OK) {
        result = HAL_ADC_GetValue(&hadc3);
    }

    HAL_ADC_Stop(&hadc3);

    return result;
}

float getMicrophoneDataCentered() {
    int data_int = getMicrophoneData();

    return (data_int - 2048.0f) / 2048.0f; // Converts 0 to 4095 to -1 to 1
}

void HAL_ADC_MspInit(ADC_HandleTypeDef* adcHandle) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    if(adcHandle->Instance == ADC3) {
        __HAL_RCC_ADC_CLK_ENABLE();

        __HAL_RCC_GPIOF_CLK_ENABLE();
        GPIO_InitStruct.Pin = GPIO_PIN_3;
        GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);
    }
}

void HAL_ADC_MspDeInit(ADC_HandleTypeDef* adcHandle) {
    if(adcHandle->Instance==ADC3) {
        __HAL_RCC_ADC_CLK_DISABLE();
        HAL_GPIO_DeInit(GPIOF, GPIO_PIN_3);
    }
}