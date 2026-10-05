#include "sensors.h"

void initSensors() {
    I2C_Init();

    MPU6050_init();
}