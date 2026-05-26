#include <stdio.h>
#include "Servo.h"
#include "mySerial.h"
#include "driver/ledc.h"
#include "driver/gpio.h"

// Servo aansturing via LEDC PWM op 50 Hz (puls van 1ms tot 2ms)
// ESP32-S3 ondersteunt max 14 bit resolutie -> 16384 stappen voor 20ms periode
//   1 ms = 16384/20    = 819   (=  0 graden)
//   2 ms = 16384/20*2  = 1638  (= 180 graden)
//
// Servo 1 -> kanaal 1, GPIO 4    (kanaal 0 wordt al door AnalogWrite gebruikt)
//                                 (GPIO 9 niet gebruiken, dat is I2C SCL voor MPU6050)
// Servo 2 -> kanaal 2, GPIO 5
// Beide kanalen delen LEDC_TIMER_1 op 50Hz

#define SERVO1_GPIO     4
#define SERVO2_GPIO     5
#define SERVO_TIMER     LEDC_TIMER_1
#define SERVO1_CHANNEL  LEDC_CHANNEL_1
#define SERVO2_CHANNEL  LEDC_CHANNEL_2

#define SERVO_MIN_DUTY  819     // 1 ms
#define SERVO_MAX_DUTY  1638    // 2 ms

void Servo_setup_timer(void)
{
    ledc_timer_config_t servo_timer = {
        .speed_mode      = LEDC_LOW_SPEED_MODE,
        .timer_num       = SERVO_TIMER,
        .duty_resolution = LEDC_TIMER_14_BIT,
        .freq_hz         = 50,
        .clk_cfg         = LEDC_AUTO_CLK,
    };
    ledc_timer_config(&servo_timer);
}

void Servo_attach(int servo_num)
{
    int gpio = (servo_num == 1) ? SERVO1_GPIO : SERVO2_GPIO;
    int chan = (servo_num == 1) ? SERVO1_CHANNEL : SERVO2_CHANNEL;

    Servo_setup_timer();   // mag meerdere keren, doet gewoon overschrijven

    ledc_channel_config_t servo_channel = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel    = chan,
        .timer_sel  = SERVO_TIMER,
        .intr_type  = LEDC_INTR_DISABLE,
        .gpio_num   = gpio,
        .duty       = SERVO_MIN_DUTY,   // start op 0 graden
        .hpoint     = 0,
    };
    ledc_channel_config(&servo_channel);

    printf("servo %d attached op GPIO %d\n", servo_num, gpio);
}

void Servo_detach(int servo_num)
{
    int chan = (servo_num == 1) ? SERVO1_CHANNEL : SERVO2_CHANNEL;
    int gpio = (servo_num == 1) ? SERVO1_GPIO : SERVO2_GPIO;

    ledc_stop(LEDC_LOW_SPEED_MODE, chan, 0);
    gpio_reset_pin(gpio);

    printf("servo %d detached\n", servo_num);
}

void Servo_write(int servo_num, int hoek)
{
    if (hoek < 0)   hoek = 0;
    if (hoek > 180) hoek = 180;

    int chan = (servo_num == 1) ? SERVO1_CHANNEL : SERVO2_CHANNEL;
    uint32_t duty = SERVO_MIN_DUTY + (hoek * (SERVO_MAX_DUTY - SERVO_MIN_DUTY)) / 180;

    ledc_set_duty(LEDC_LOW_SPEED_MODE, chan, duty);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, chan);

    printf("servo %d -> %d graden\n", servo_num, hoek);
}


// Hier komt het commando van Scilab binnen:
//   Sa1 / Sa2     attach servo 1 of 2
//   Sd1 / Sd2     detach servo
//   Sw1n / Sw2n   schrijf hoek n (raw byte 0..180) naar servo
void handle_servo(void)
{
    char cmd;
    mySerial_Readn(&cmd, 1);

    char num;
    mySerial_Readn(&num, 1);
    if (num != '1' && num != '2') {
        printf("ongeldig servo nummer\n");
        return;
    }
    int servo_num = num - '0';

    if (cmd == 'a') {
        Servo_attach(servo_num);
    }
    else if (cmd == 'd') {
        Servo_detach(servo_num);
    }
    else if (cmd == 'w') {
        char hoek_byte;
        mySerial_Readn(&hoek_byte, 1);
        Servo_write(servo_num, (uint8_t)hoek_byte);
    }
    else {
        printf("onbekend servo commando\n");
    }
}
