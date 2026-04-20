#include <stdio.h>
#include "AnalogRead.h"
#include "mySerial.h"

void handle_analog_read(void)
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
            printf("not implemented\n");
            break;
        default:
            printf("unknown character\n");
    }
}

