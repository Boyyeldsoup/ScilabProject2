#include <stdio.h>
#include "AnalogWrite.h"
#include <stdint.h>
#include "driver/ledc.h"
#include "mySerial.h"

void handle_analog_write(void) {
    char pinASCII;
    mySerial_Readn(&pinASCII, 1);

    if (pinASCII > 47 && pinASCII < 93)
    {
        int pin = pinASCII - 48;

        uint8_t duty_value;
        mySerial_Readn((char*)&duty_value, 1);

        ledc_timer_config_t timer_cfg = {
            .speed_mode      = LEDC_LOW_SPEED_MODE,
            .timer_num       = LEDC_TIMER_0,
            .duty_resolution = LEDC_TIMER_8_BIT,
            .freq_hz         = 1000,
            .clk_cfg         = LEDC_AUTO_CLK,
        };
        ledc_timer_config(&timer_cfg);

        ledc_channel_config_t channel_cfg = {
            .speed_mode = LEDC_LOW_SPEED_MODE,
            .channel    = LEDC_CHANNEL_0,
            .timer_sel  = LEDC_TIMER_0,
            .intr_type  = LEDC_INTR_DISABLE,
            .gpio_num   = pin,
            .duty       = duty_value,
            .hpoint     = 0,
        };
        ledc_channel_config(&channel_cfg);

        printf("analog write pin %d duty %d\n", pin, duty_value);
    }
    else
    {
        printf("invalid pin\n");
    }

}
