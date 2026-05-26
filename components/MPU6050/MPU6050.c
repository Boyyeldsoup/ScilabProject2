#include <stdio.h>
#include <string.h>
#include "MPU6050.h"
#include "mySerial.h"
#include "driver/i2c_master.h"

// Lichte MPU6050 driver - leest accel + gyro waarden via I2C.
//
// Commando's vanuit Scilab:
//   Gt   test of de I2C verbinding werkt -> antwoord "OK" of "KO"
//   Ga   start acquisitie (zet vlag aan)
//   Gs   stop acquisitie
//   Gr   stuur 9 int16 waarden: yaw, pitch, roll, ax, ay, az, wx, wy, wz
//
// LET OP: yaw/pitch/roll worden hier op 0 gezet. De originele Arduino code
// gebruikt de DMP firmware van InvenSense voor de sensor fusie. Dat porten
// naar ESP-IDF is een serieus karwei - voor nu krijg je alleen de ruwe data.
// Indien je de hoeken wel wilt: een Madgwick of complementair filter
// in software is meestal voldoende.
//
// Bedrading volgens stijl van de prof (zie I2C_MCP230008 component):
//   SDA -> GPIO 8
//   SCL -> GPIO 9
//   AD0 -> GND  (-> adres 0x68)


static i2c_master_bus_config_t i2c_mst_config = {
    .clk_source = I2C_CLK_SRC_DEFAULT,
    .i2c_port = MPU_I2C_PORT,
    .scl_io_num = MPU_SCL_pin,
    .sda_io_num = MPU_SDA_pin,
    .glitch_ignore_cnt = 7,
    .flags.enable_internal_pullup = true,
};

static i2c_master_bus_handle_t bus_handle;

static i2c_device_config_t dev_cfg = {
    .dev_addr_length = I2C_ADDR_BIT_LEN_7,
    .device_address = MPU_ADR,
    .scl_speed_hz = 400000,
};

static i2c_master_dev_handle_t dev_handle;

static int init_ok = 0;
static int acq_loopt = 0;


void MPU_write(mpu_reg_t reg, uint8_t value)
{
    uint8_t data_wr[2];
    data_wr[0] = reg;
    data_wr[1] = value;
    i2c_master_transmit(dev_handle, data_wr, 2, -1);
}

uint8_t MPU_read(mpu_reg_t reg)
{
    uint8_t buf[1];
    buf[0] = (uint8_t)reg;
    i2c_master_transmit_receive(dev_handle, buf, 1, buf, 1, -1);
    return buf[0];
}

void MPU_read_burst(mpu_reg_t reg, uint8_t *out, int len)
{
    uint8_t r = (uint8_t)reg;
    i2c_master_transmit_receive(dev_handle, &r, 1, out, len, -1);
}


int MPU_setup(int i2c_start)
{
    if (init_ok) return 1;

    if (i2c_start) {
        if (i2c_new_master_bus(&i2c_mst_config, &bus_handle) != ESP_OK) {
            printf("MPU6050: kon I2C bus niet opzetten\n");
            return 0;
        }
    }
    if (i2c_master_bus_add_device(bus_handle, &dev_cfg, &dev_handle) != ESP_OK) {
        printf("MPU6050: kon device niet toevoegen\n");
        return 0;
    }

    // WHO_AM_I check (zou 0x68 moeten zijn, soms 0x70 op clones)
    uint8_t who = MPU_read(MPU_WHO_AM_I);
    printf("MPU6050: WHO_AM_I = 0x%02X\n", who);

    // Wakker maken (bit 6 = sleep, default 1 -> moet 0 zijn)
    MPU_write(MPU_PWR_MGMT_1, 0x00);

    init_ok = 1;
    return 1;
}


void MPU_lees_data(int16_t out[9])
{
    memset(out, 0, sizeof(int16_t) * 9);
    if (!init_ok) return;

    uint8_t buf[14];
    MPU_read_burst(MPU_ACCEL_XOUT_H, buf, 14);

    // MPU6050 stuurt big-endian
    int16_t ax = (int16_t)((buf[0]  << 8) | buf[1]);
    int16_t ay = (int16_t)((buf[2]  << 8) | buf[3]);
    int16_t az = (int16_t)((buf[4]  << 8) | buf[5]);
    // buf[6..7] = temperatuur, slaan we over
    int16_t gx = (int16_t)((buf[8]  << 8) | buf[9]);
    int16_t gy = (int16_t)((buf[10] << 8) | buf[11]);
    int16_t gz = (int16_t)((buf[12] << 8) | buf[13]);

    // yaw, pitch, roll laten we op 0 (geen sensor fusie)
    out[0] = 0;
    out[1] = 0;
    out[2] = 0;
    out[3] = ax;
    out[4] = ay;
    out[5] = az;
    out[6] = gx;
    out[7] = gy;
    out[8] = gz;
}


void handle_mpu6050(void)
{
    char cmd;
    mySerial_Readn(&cmd, 1);

    if (cmd == 't') {
        int ok = MPU_setup(1);
        mySerial_WriteString(ok ? "OK" : "KO");
    }
    else if (cmd == 'a') {
        MPU_setup(1);
        acq_loopt = 1;
        printf("MPU6050: acquisitie gestart\n");
    }
    else if (cmd == 's') {
        acq_loopt = 0;
        printf("MPU6050: acquisitie gestopt\n");
    }
    else if (cmd == 'r') {
        int16_t data[9];
        MPU_lees_data(data);
        // 9 * 2 = 18 bytes, little-endian (ESP32 is LE)
        mySerial_Writen((const char*)data, sizeof(data));
    }
    else {
        printf("onbekend MPU commando\n");
    }

    (void)acq_loopt;  // momenteel alleen informatief, 'r' leest direct
}
