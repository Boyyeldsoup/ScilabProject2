#include <stdio.h>
#include "Encoder.h"
#include "mySerial.h"
#include "driver/gpio.h"
#include "esp_attr.h"

// Quadrature encoder met GPIO interrupts
//
// Commando's vanuit Scilab:
//   Eajklm  activate encoder j op chA=GPIO k, chB=GPIO l, mode=ascii(m)
//           mode 1: tellen op opgaande flank van chA
//           mode 2: tellen op elke flank van chA
//           mode 4: tellen op elke flank van chA EN chB (volle quadrature)
//   Epi     read positie van encoder i (4 bytes signed little-endian)
//   Eri     release encoder i (test.c protocol leest dan ook nog 2 dummy bytes)
//   Ezi     zet positie op nul

#define MAX_ENCODERS 6

typedef struct {
    int  pinA;
    int  pinB;
    int  mode;          // 0 = niet actief, 1/2/4 anders
    volatile int32_t pos;
    volatile uint8_t laatste_A;
    volatile uint8_t laatste_B;
} encoder_t;

static encoder_t encoders[MAX_ENCODERS] = {
    {-1,-1,0,0,0,0},{-1,-1,0,0,0,0},{-1,-1,0,0,0,0},
    {-1,-1,0,0,0,0},{-1,-1,0,0,0,0},{-1,-1,0,0,0,0}
};

static int isr_geinstalleerd = 0;


// Mode 1: tellen op opgaande flank van A, richting kijken via B
static void IRAM_ATTR Encoder_ISR_mode1(void* arg)
{
    encoder_t *e = (encoder_t*)arg;
    if (gpio_get_level(e->pinB)) e->pos++;
    else                          e->pos--;
}

// Mode 2: tellen op elke flank van A
static void IRAM_ATTR Encoder_ISR_mode2(void* arg)
{
    encoder_t *e = (encoder_t*)arg;
    int a = gpio_get_level(e->pinA);
    int b = gpio_get_level(e->pinB);
    if (a == b) e->pos++;
    else        e->pos--;
}

// Mode 4: tellen op elke flank van A, met volledige quadrature decode
static void IRAM_ATTR Encoder_ISR_mode4_A(void* arg)
{
    encoder_t *e = (encoder_t*)arg;
    int a = gpio_get_level(e->pinA);
    int b = gpio_get_level(e->pinB);
    if (a != e->laatste_A) {
        if (a == b) e->pos--;
        else        e->pos++;
        e->laatste_A = a;
    }
}

// Mode 4: tellen op elke flank van B
static void IRAM_ATTR Encoder_ISR_mode4_B(void* arg)
{
    encoder_t *e = (encoder_t*)arg;
    int a = gpio_get_level(e->pinA);
    int b = gpio_get_level(e->pinB);
    if (b != e->laatste_B) {
        if (a == b) e->pos++;
        else        e->pos--;
        e->laatste_B = b;
    }
}


void Encoder_release(int idx)
{
    if (idx < 0 || idx >= MAX_ENCODERS) return;
    encoder_t *e = &encoders[idx];
    if (e->mode == 0) return;

    gpio_isr_handler_remove(e->pinA);
    gpio_set_intr_type(e->pinA, GPIO_INTR_DISABLE);
    if (e->mode == 4) {
        gpio_isr_handler_remove(e->pinB);
        gpio_set_intr_type(e->pinB, GPIO_INTR_DISABLE);
    }

    e->mode = 0;
    e->pos = 0;
    e->pinA = -1;
    e->pinB = -1;
    printf("encoder %d losgelaten\n", idx);
}


void Encoder_activate(int idx, int pinA, int pinB, int mode)
{
    if (idx < 0 || idx >= MAX_ENCODERS) {
        printf("ongeldig encoder nummer %d\n", idx);
        return;
    }
    if (mode != 1 && mode != 2 && mode != 4) {
        printf("ongeldige encoder mode %d\n", mode);
        return;
    }

    // Indien al actief, eerst loslaten
    if (encoders[idx].mode != 0) Encoder_release(idx);

    encoder_t *e = &encoders[idx];
    e->pinA = pinA;
    e->pinB = pinB;
    e->mode = mode;
    e->pos = 0;

    // Beide pinnen als input
    gpio_config_t io_cfg = {
        .pin_bit_mask = (1ULL << pinA) | (1ULL << pinB),
        .mode         = GPIO_MODE_INPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_cfg);

    if (!isr_geinstalleerd) {
        gpio_install_isr_service(0);
        isr_geinstalleerd = 1;
    }

    if (mode == 1) {
        gpio_set_intr_type(pinA, GPIO_INTR_POSEDGE);
        gpio_isr_handler_add(pinA, Encoder_ISR_mode1, e);
    }
    else if (mode == 2) {
        gpio_set_intr_type(pinA, GPIO_INTR_ANYEDGE);
        gpio_isr_handler_add(pinA, Encoder_ISR_mode2, e);
    }
    else { // mode 4
        e->laatste_A = gpio_get_level(pinA);
        e->laatste_B = gpio_get_level(pinB);
        gpio_set_intr_type(pinA, GPIO_INTR_ANYEDGE);
        gpio_set_intr_type(pinB, GPIO_INTR_ANYEDGE);
        gpio_isr_handler_add(pinA, Encoder_ISR_mode4_A, e);
        gpio_isr_handler_add(pinB, Encoder_ISR_mode4_B, e);
    }

    printf("encoder %d actief: A=GPIO%d B=GPIO%d mode=%d\n",
            idx, pinA, pinB, mode);
}


void handle_encoder(void)
{
    char cmd;
    mySerial_Readn(&cmd, 1);

    if (cmd == 'a') {
        char num_b, pinA_b, pinB_b, mode_c;
        mySerial_Readn(&num_b,  1);
        mySerial_Readn(&pinA_b, 1);
        mySerial_Readn(&pinB_b, 1);
        mySerial_Readn(&mode_c, 1);
        Encoder_activate((uint8_t)num_b, (uint8_t)pinA_b, (uint8_t)pinB_b, mode_c - '0');
    }
    else if (cmd == 'p') {
        char num_b;
        mySerial_Readn(&num_b, 1);
        int idx = (uint8_t)num_b;
        int32_t v = (idx >= 0 && idx < MAX_ENCODERS) ? encoders[idx].pos : 0;
        mySerial_Writen((const char*)&v, 4);
    }
    else if (cmd == 'r') {
        char num_b, dummyA, dummyB;
        mySerial_Readn(&num_b, 1);
        Encoder_release((uint8_t)num_b);
        // test.c protocol verwacht nog 2 extra bytes -> opvragen en weggooien
        mySerial_Readn(&dummyA, 1);
        mySerial_Readn(&dummyB, 1);
        (void)dummyA; (void)dummyB;
    }
    else if (cmd == 'z') {
        char num_b;
        mySerial_Readn(&num_b, 1);
        int idx = (uint8_t)num_b;
        if (idx >= 0 && idx < MAX_ENCODERS) encoders[idx].pos = 0;
    }
    else {
        printf("onbekend encoder commando\n");
    }
}
