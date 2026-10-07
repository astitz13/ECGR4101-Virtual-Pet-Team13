#include "mpu6050.h"
// Include necessary HAL functions and typedefs
#include "stm32l4xx_hal.h"
#include "stm32l4xx_hal_gpio.h"
#include "stm32l4xx_hal_i2c.h"
// Required for error handler
#include "main.h"

// Handles I2C communication
static I2C_HandleTypeDef hi2c1;

// Write bytes to MPU6050 register over I2C
void MPU6050_writeBytes(MPU6050_register_t reg, uint16_t len, uint8_t *data) {
    HAL_I2C_Mem_Write(&hi2c1, MPU6050_I2C_ADDR, reg, I2C_MEMADD_SIZE_8BIT, data, len, 1);
}

// Read bytes from MPU6050 register
void MPU6050_readBytes(MPU6050_register_t reg, uint16_t len, uint8_t *data) {
    HAL_I2C_Mem_Read(&hi2c1, MPU6050_I2C_ADDR, reg, I2C_MEMADD_SIZE_8BIT, data, len, 1);
}

// Initialize MPU6050
void MPU6050_init(void) {
    HAL_Delay(50);

    uint8_t writeByte = 1 << 7; // Reset
    MPU6050_writeBytes(MPU6050_PWR_MGMT_1, 1, &writeByte);
    HAL_Delay(100);

    writeByte = 0; // Wake up MPU6050
    MPU6050_writeBytes(MPU6050_PWR_MGMT_1, 1, &writeByte);
    HAL_Delay(50);

    writeByte = 31; // 250 Hz assuming 8kHz starting
    MPU6050_writeBytes(MPU6050_SMPLRT_DIV, 1, &writeByte);
    HAL_Delay(50);

    writeByte = 0; // Disable DLPF
    MPU6050_writeBytes(MPU6050_CONFIG, 1, &writeByte);
    HAL_Delay(50);

    writeByte = 0; // +/- 250 degrees per second
    MPU6050_writeBytes(MPU6050_GYRO_CONFIG, 1, &writeByte);
    HAL_Delay(50);

    writeByte = 0; // +/- 2g
    MPU6050_writeBytes(MPU6050_ACCEL_CONFIG, 1, &writeByte);
    HAL_Delay(50);

    writeByte = 0b0011000; // LATCH_INT_EN and INT_RD_CLEAR set
    MPU6050_writeBytes(MPU6050_INT_PIN_CFG, 1, &writeByte);
    HAL_Delay(50);

    writeByte = 1;
    MPU6050_writeBytes(MPU6050_INT_ENABLE, 1, &writeByte);
    HAL_Delay(50);
}

// Get data of MPU6050 into raw data struct
void MPU6050_getData(MPU6050_data_raw_t *data) {
    uint8_t buf[14];
    // 14 bytes after XOUT_H include all acceleration, temperature, and gyro registers
    MPU6050_readBytes(MPU6050_ACCEL_XOUT_H, 14, (uint8_t*)buf);

    // Adjust endianness of registers and load into struct
    data->acc_x = (buf[0] << 8) | buf[1];
    data->acc_y = (buf[2] << 8) | buf[3];
    data->acc_z = (buf[4] << 8) | buf[5];
    data->temperature = (buf[6] << 8) | buf[7];
    data->gyro_x = (buf[8] << 8) | buf[9];
    data->gyro_y = (buf[10] << 8) | buf[11];
    data->gyro_z = (buf[12] << 8) | buf[13];
}

// Get normalized MPU6050 data
void MPU6050_getParsedData(MPU6050_data_t *data) {
    MPU6050_data_raw_t temp;
    MPU6050_getData(&temp);

    // Convert acceleration into gs (1g = 9.81 m/s^2)
    data->acc_x = (int16_t)temp.acc_x / 16384.0f; // LSB sensitivity from datasheet
    data->acc_y = (int16_t)temp.acc_y / 16384.0f;
    data->acc_z = (int16_t)temp.acc_z / 16384.0f;
    // Convert temperature to Celsius
    data->temperature = (int16_t)temp.temperature / 340.0f + 36.53f;
    // Convert gyro to rad/s
    data->gyro_x = (int16_t)temp.gyro_x / 131.0f;
    data->gyro_y = (int16_t)temp.gyro_y / 131.0f;
    data->gyro_z = (int16_t)temp.gyro_z / 131.0f;
}

// Get if data is ready based on interrupt pin
bool MPU6050_getDataReady(void) {
    return HAL_GPIO_ReadPin(MPU6050_INT_PORT, MPU6050_INT_PIN) != 0;
}

// Initialize I2C
void I2C_Init(void) {
    hi2c1.Instance = I2C1;
    hi2c1.Init.Timing = 0x2000090E;
    hi2c1.Init.OwnAddress1 = 0;
    hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    hi2c1.Init.OwnAddress2 = 0;
    hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;

    if (HAL_I2C_Init(&hi2c1) != HAL_OK) {
        Error_Handler();
    }
}

// Initialize GPIO for I2C (including SDA and SCL pins)
void HAL_I2C_MspInit(I2C_HandleTypeDef* i2cHandle) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    if(i2cHandle->Instance == I2C1) {
        __HAL_RCC_GPIOG_CLK_ENABLE();
        GPIO_InitStruct.Pin = GPIO_PIN_13|GPIO_PIN_14;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
        GPIO_InitStruct.Pull = GPIO_PULLUP;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        GPIO_InitStruct.Alternate = GPIO_AF4_I2C1;
        HAL_GPIO_Init(GPIOG, &GPIO_InitStruct);

        __HAL_RCC_I2C1_CLK_ENABLE();
    }
}

// Deinitialization of GPIO for I2C
void HAL_I2C_MspDeInit(I2C_HandleTypeDef* i2cHandle) {
    if(i2cHandle->Instance==I2C1) {
        __HAL_RCC_I2C1_CLK_DISABLE();
        HAL_GPIO_DeInit(GPIOG, GPIO_PIN_13);
        HAL_GPIO_DeInit(GPIOG, GPIO_PIN_14);
    }
}