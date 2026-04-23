#include <stdio.h>
#include "AnalogRead.h"
#include "mySerial.h"
#include "esp_adc/adc_oneshot.h"
#include "stdint.h"

static int raw_value;
static adc_oneshot_unit_handle_t adc_handle = NULL;

void handle_analog_read(void)
{
    char pinASCII;
    mySerial_Readn(&pinASCII, 1);

    if (pinASCII > 47 && pinASCII < 57)
    {
        int channel = pinASCII - 48;

        // initialize only once
        if (adc_handle == NULL)
        {
            adc_oneshot_unit_init_cfg_t init_cfg = {
                .unit_id = ADC_UNIT_1,
            };
            adc_oneshot_new_unit(&init_cfg, &adc_handle);
        }

        adc_oneshot_chan_cfg_t chan_cfg = {
            .atten = ADC_ATTEN_DB_12,
            .bitwidth = ADC_BITWIDTH_12,
        };
        adc_oneshot_config_channel(adc_handle, channel, &chan_cfg);

        // average 64 samples
        int sum = 0;
        int sample;
        for (int i = 0; i < 64; i++) {
            adc_oneshot_read(adc_handle, channel, &sample);
            sum += sample;
        }
        raw_value = sum / 64;
        printf("raw_value: %d\n", raw_value);

        uint16_t result = (uint16_t)(raw_value / 4);  // scale 12-bit to 10-bit
        mySerial_Writen((char*)&result, 2);
    }
    else
    {
        printf("invalid pin\n");
    }
}