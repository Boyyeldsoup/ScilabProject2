#include <stdio.h>
#include "Digital.h"
#include "mySerial.h"
#include "driver/gpio.h"

char pin;
char Input_output;
char pinASCII;
char status;
char dgv;
char input_value;

void handle_digital(void)
{

    mySerial_Readn(&input_value,1);
    switch(input_value) 
    {
        case 'a':
            printf("assign\n");

            mySerial_Readn(&pinASCII,1);
            if (pinASCII>49 && pinASCII<102) { 
                pin=pinASCII-48; //number of the digital pin

                mySerial_Readn(&Input_output,1);
                if (Input_output == 48) {
                    gpio_reset_pin(pin);
                    gpio_set_direction(pin, GPIO_MODE_INPUT);
                }
                if (Input_output == 49) {
                    gpio_reset_pin(pin);
                    gpio_set_direction(pin, GPIO_MODE_OUTPUT);
                } 
            }


        break;
        case 'r':
            printf("read\n");
            mySerial_Readn(&pinASCII,1);
            if (pinASCII>49 && pinASCII<102) { 
                pin=pinASCII-48; //number of the digital pin
                dgv=gpio_get_level(pin);
                dgv=dgv+48; //convert to ASCII
                mySerial_Writen(&dgv,1);
                
            }
        break;
        case 'w':
            printf("writeD\n");
            mySerial_Readn(&pinASCII,1);
            if (pinASCII>49 && pinASCII<102) 
            { 
                pin=pinASCII-48; //number of the digital pin

                mySerial_Readn(&status,1);
                if(status)
                if (status == 48 || status == 49) { // 0 or 1
                    dgv=status-48;
                    if (dgv == 0)
                        printf("status is 0\n");
                    else
                        printf("status is 1\n");
                    

                    gpio_set_level(pin, dgv);
                }
            }
        break;
    }      
}
