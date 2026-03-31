#include <stdio.h>
#include "mySerial.h"
#include "driver/gpio.h"

//my functions___________________________________
void setup(void);
void loop(void);
void handle_reference(void);
void handle_digital(void);
//void handle_analog_write(void);
//_______________________________________________


void app_main(void)
{
    setup();

    while(1) {
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
    mySerial_Readn(&input_value,1);
    switch(input_value) 
    {
        case 'A':
            printf("analoog\n");
            break;
        case 'W':
            printf("analoog schrijven\n");
            //handle_analog_write();
            break;
        case 'S':
            printf("servo\n");
            break;
        case 'G':
            printf("MPU6050\n");
            break;
        case 'I':
            printf("interrupt\n");
            break;
        case 'E':
            printf("encoder\n");
            break;
        case 'C':
            printf("counter\n");
            break;
        case 'M':
            printf("dc motor\n");
            break;
        case 'R':
            printf("analog reference\n");
            handle_reference();
            break;

        case 'D':
            printf("Digitaal\n");
            handle_digital();
            break;
        default:
            printf("unknown character\n");
    }




}


void handle_reference(void)
{
    char tmp;
    mySerial_Readn(&tmp,1);
    switch(tmp) 
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

char pin;
char Input_output;
char pinASCII;
char status;
char dgv;

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

/*void handle_analog_write(void)
{
        printf("analoog schrijven\n");
        switch(pin)
        {
            case '0':
                printf("not implemented\n");
                break;
            case '1':
                printf("not implemented\n");
                break;
            default:
                printf("unknown character\n");
        }

        
}*/

