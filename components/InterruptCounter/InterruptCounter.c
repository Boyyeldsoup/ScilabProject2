#include <stdio.h>
#include "InterruptCounter.h"
#include "mySerial.h"
#include "driver/gpio.h"
#include "esp_attr.h"

// Telt opgaande flanken (rising edge) op een GPIO pin via een interrupt
//
// Commando's vanuit Scilab:
//   Iag   activate counter op GPIO g (g = raw byte = pin nummer)
//   Irg   release counter op GPIO g
//   Ipg   read counter (4 bytes little-endian) van GPIO g
//   Izg   zet counter op nul
//
// Op de originele Arduino was g een "INT nummer" (0..5). Op de ESP32
// kan elke GPIO een interrupt zijn, dus we sturen het pin nummer
// rechtstreeks door.

#define MAX_COUNTERS 8

typedef struct {
    int gpio;                       // -1 als deze plek vrij is
    volatile uint32_t waarde;
} counter_t;

static counter_t counters[MAX_COUNTERS] = {
    {-1,0},{-1,0},{-1,0},{-1,0},
    {-1,0},{-1,0},{-1,0},{-1,0}
};

static int isr_geinstalleerd = 0;


// De interrupt routine zelf - houden we kort
static void IRAM_ATTR Counter_ISR(void* arg)
{
    counter_t *c = (counter_t*)arg;
    c->waarde++;
}


// Zoek het slot dat bij een GPIO hoort, -1 als niet gevonden
static int Counter_zoek_slot(int gpio)
{
    for (int i = 0; i < MAX_COUNTERS; i++) {
        if (counters[i].gpio == gpio) return i;
    }
    return -1;
}

static int Counter_zoek_vrij_slot(void)
{
    for (int i = 0; i < MAX_COUNTERS; i++) {
        if (counters[i].gpio == -1) return i;
    }
    return -1;
}


void Counter_activate(int gpio)
{
    if (Counter_zoek_slot(gpio) >= 0) {
        printf("counter op GPIO %d is al actief\n", gpio);
        return;
    }
    int slot = Counter_zoek_vrij_slot();
    if (slot < 0) {
        printf("geen vrij counter slot meer\n");
        return;
    }

    counters[slot].gpio = gpio;
    counters[slot].waarde = 0;

    gpio_config_t io_cfg = {
        .pin_bit_mask   = (1ULL << gpio),
        .mode           = GPIO_MODE_INPUT,
        .pull_up_en     = GPIO_PULLUP_DISABLE,
        .pull_down_en   = GPIO_PULLDOWN_DISABLE,
        .intr_type      = GPIO_INTR_POSEDGE,    // tellen op opgaande flank
    };
    gpio_config(&io_cfg);

    if (!isr_geinstalleerd) {
        gpio_install_isr_service(0);
        isr_geinstalleerd = 1;
    }
    gpio_isr_handler_add(gpio, Counter_ISR, &counters[slot]);

    printf("counter actief op GPIO %d (slot %d)\n", gpio, slot);
}

void Counter_release(int gpio)
{
    int slot = Counter_zoek_slot(gpio);
    if (slot < 0) {
        printf("geen counter op GPIO %d\n", gpio);
        return;
    }
    gpio_isr_handler_remove(gpio);
    gpio_set_intr_type(gpio, GPIO_INTR_DISABLE);

    counters[slot].gpio = -1;
    counters[slot].waarde = 0;
    printf("counter op GPIO %d is vrijgegeven\n", gpio);
}

void Counter_send(int gpio)
{
    int slot = Counter_zoek_slot(gpio);
    uint32_t v = (slot >= 0) ? counters[slot].waarde : 0;
    // 4 bytes little-endian sturen, net zoals test.c doet
    mySerial_Writen((const char *)&v, 4);
}

void Counter_reset(int gpio)
{
    int slot = Counter_zoek_slot(gpio);
    if (slot >= 0) counters[slot].waarde = 0;
}


void handle_interrupt(void)
{
    char cmd;
    mySerial_Readn(&cmd, 1);

    char gpio_byte;
    mySerial_Readn(&gpio_byte, 1);
    int gpio = (uint8_t)gpio_byte;

    if      (cmd == 'a') Counter_activate(gpio);
    else if (cmd == 'r') Counter_release(gpio);
    else if (cmd == 'p') Counter_send(gpio);
    else if (cmd == 'z') Counter_reset(gpio);
    else printf("onbekend interrupt commando\n");
}
