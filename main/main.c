#include <stdio.h>
#include "mySerial.h"
#include "driver/gpio.h"
#include "Digital.h"
#include "AnalogRead.h"
#include "AnalogWrite.h"
#include "Servo.h"
#include "InterruptCounter.h"
#include "DCMotor.h"
#include "Encoder.h"
#include "MPU6050.h"

//mijn functies__________________________________
void setup(void);
void loop(void);
void handle_reference(void);
//_______________________________________________


void app_main(void)
{
    setup();

    while (1) {
        loop();
    }
}


void setup(void)
{
    mySerial_Setup();
}


char input_value;
void loop(void)
{
    //lees 1 character in input_value
    mySerial_Readn(&input_value, 1);
    switch (input_value)
    {
        case 'A':
            printf("analoog\n");
            handle_analog_read();
            break;
        case 'W':
            printf("analoog schrijven\n");
            handle_analog_write();
            break;
        case 'S':
            printf("servo\n");
            handle_servo();
            break;
        case 'G':
            printf("MPU6050\n");
            handle_mpu6050();
            break;
        case 'I':
            printf("interrupt\n");
            handle_interrupt();
            break;
        case 'E':
            printf("encoder\n");
            handle_encoder();
            break;
        case 'C':
            printf("DC motor init\n");
            handle_dcmotor_init();
            break;
        case 'M':
            printf("DC motor speed\n");
            handle_dcmotor_speed();
            break;
        case 'R':
            printf("analog reference\n");
            handle_reference();
            break;
        case 'D':
            printf("Digitaal\n");
            handle_digital();
            break;
        case 'L':
            printf("liksensor\n");
            handle_analog_read();
            break;
        case 'P':
            printf("potentiometer\n");
            handle_analog_read();
            break;
        default:
            printf("unknown character\n");
    }
}


void handle_reference(void)
{
    char tmp;
    mySerial_Readn(&tmp, 1);
    switch (tmp)
    {
        case '0':
        case '1':
        case '2':
            printf("not implemented\n");
            break;

        case '3':
            mySerial_WriteString("v5");
            break;
        default:
            printf("unknown character\n");
    }
}
