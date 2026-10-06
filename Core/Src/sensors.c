#include "sensors.h"

void initSensors() {
    I2C_Init();
    MX_ADC1_Init();

    MPU6050_init();
}