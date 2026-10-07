#include "sensors.h"

void initSensors() {
    I2C_Init();
    initMicrophone();

    MPU6050_init();
}