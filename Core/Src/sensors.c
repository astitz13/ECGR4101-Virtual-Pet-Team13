#include "sensors.h"

// Initialize microphone and MPU6050
void initSensors() {
    I2C_Init();
    initMicrophone();

    MPU6050_init();
}