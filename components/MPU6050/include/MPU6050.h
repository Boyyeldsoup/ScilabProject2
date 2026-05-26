#pragma once

#define MPU_I2C_PORT    0
#define MPU_ADR         0x68
#define MPU_SDA_pin     8
#define MPU_SCL_pin     9

// Register adressen van MPU6050
typedef enum {
    MPU_PWR_MGMT_1   = 0x6B,
    MPU_WHO_AM_I     = 0x75,
    MPU_ACCEL_XOUT_H = 0x3B,
} mpu_reg_t;


void handle_mpu6050(void);
