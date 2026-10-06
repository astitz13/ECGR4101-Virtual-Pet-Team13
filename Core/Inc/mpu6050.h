#ifndef MPU6050_H
#define MPU6050_H


#include "stm32l4xx_hal.h"
#include <stdbool.h>

#define MPU6050_I2C_ADDR (0x68 << 1)
#define MPU6050_INT_PORT GPIOF
#define MPU6050_INT_PIN 11

typedef enum {
    MPU6050_SELF_TEST_X = 0x0D,
    MPU6050_SELF_TEST_Y = 0x0E,
    MPU6050_SELF_TEST_Z = 0x0F,
    MPU6050_SELF_TEST_A = 0x10,
    MPU6050_SMPLRT_DIV = 0x19,
    MPU6050_CONFIG = 0x1A,
    MPU6050_GYRO_CONFIG = 0x1B,
    MPU6050_ACCEL_CONFIG = 0x1C,
    MPU6050_MOT_THR = 0x1F,
    MPU6050_FIFO_EN = 0x23,
    MPU6050_INT_PIN_CFG = 0x37,
    MPU6050_INT_ENABLE = 0x38,
    MPU6050_INT_STATUS = 0x3A,
    MPU6050_ACCEL_XOUT_H = 0x3B,
    MPU6050_ACCEL_XOUT_L = 0x3C,
    MPU6050_ACCEL_YOUT_H = 0x3D,
    MPU6050_ACCEL_YOUT_L = 0x3E,
    MPU6050_ACCEL_ZOUT_H = 0x3F,
    MPU6050_ACCEL_ZOUT_L = 0x40,
    MPU6050_TEMP_OUT_H = 0x41,
    MPU6050_TEMP_OUT_L = 0x42,
    MPU6050_GYRO_XOUT_H = 0x43,
    MPU6050_GYRO_XOUT_L = 0x44,
    MPU6050_GYRO_YOUT_H = 0x45,
    MPU6050_GYRO_YOUT_L = 0x46,
    MPU6050_GYRO_ZOUT_H = 0x47,
    MPU6050_GYRO_ZOUT_L = 0x48,
    MPU6050_SIGNAL_PATH_RESET = 0x68,
    MPU6050_MOT_DETECT_CTRL = 0x69,
    MPU6050_USER_CTRL = 0x6A,
    MPU6050_PWR_MGMT_1 = 0x6B,
    MPU6050_PWR_MGMT_2 = 0x6C,
    MPU6050_FIFO_COUNTH = 0x72,
    MPU6050_FIFO_COUNTL = 0x73,
    MPU6050_FIFO_R_W = 0x74,
    MPU6050_WHO_AM_I = 0x75
} MPU6050_register_t;

typedef struct {
    float acc_x;
    float acc_y;
    float acc_z;
    float temperature;
    float gyro_x;
    float gyro_y;
    float gyro_z;
} MPU6050_data_t;

typedef struct {
    uint16_t acc_x;
    uint16_t acc_y;
    uint16_t acc_z;
    uint16_t temperature;
    uint16_t gyro_x;
    uint16_t gyro_y;
    uint16_t gyro_z;
} MPU6050_data_raw_t;

void MPU6050_writeBytes(MPU6050_register_t reg, uint16_t len, uint8_t *data);
void MPU6050_readBytes(MPU6050_register_t reg, uint16_t len, uint8_t *data);

void MPU6050_init();

void MPU6050_getData(MPU6050_data_raw_t *data);
void MPU6050_getParsedData(MPU6050_data_t *data);

bool MPU6050_getDataReady();


void I2C_Init();
void HAL_I2C_MspInit(I2C_HandleTypeDef* i2cHandle);
void HAL_I2C_MspDeInit(I2C_HandleTypeDef* i2cHandle);

#endif // MPU6050_H