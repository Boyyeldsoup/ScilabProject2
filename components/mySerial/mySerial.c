#include <stdio.h>
#include <string.h>
#include "mySerial.h"
#include "driver/usb_serial_jtag.h"



void mySerial_Setup(void)
{
     usb_serial_jtag_driver_config_t usb_serial_jtag_config = {
        .rx_buffer_size = BUF_SIZE,
        .tx_buffer_size = BUF_SIZE,
    };

    usb_serial_jtag_driver_install(&usb_serial_jtag_config);
}
int mySerial_Readn(char *data, int len)
{
    int lengte = usb_serial_jtag_read_bytes(data, len, portMAX_DELAY);
    return lengte;
}
void mySerial_Writen(const char *data, int len)
{
    usb_serial_jtag_write_bytes((const char *) data, len, 20 / portTICK_PERIOD_MS);
            

}
void mySerial_WriteString(const char *str)
{
    mySerial_Writen(str, strlen(str));
}