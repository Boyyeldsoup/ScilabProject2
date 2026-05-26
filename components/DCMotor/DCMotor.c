#include <stdio.h>
#include "DCMotor.h"
#include "mySerial.h"
#include "driver/ledc.h"
#include "driver/gpio.h"

// DC motor aansturing met PWM via LEDC
//
// Commando's vanuit Scilab:
//   Cijkl   init motor i (1..4), pin1=ascii(j), pin2=ascii(k), mode=ascii(l)
//           mode 0 = L293 (twee PWM pinnen, een per richting)
//           mode 1 = L298 (1 PWM pin + 1 richtings-bit)
//   Mijk    motor i, richting j (0/1), snelheid k (raw byte 0..255)
//   Mir     motor i loslaten (release)
//
// LEDC kanalen die we hier gebruiken: 3 t.e.m. 7
//   (0 = AnalogWrite, 1 = Servo1, 2 = Servo2)
// We gebruiken LEDC_TIMER_2 op 1kHz, 8 bit (0..255 past mooi)

#define MOTOR_TIMER         LEDC_TIMER_2
#define MOTOR_FIRST_KANAAL  3
#define MOTOR_LAAT_KANAAL   7

typedef struct {
    int  pin1;
    int  pin2;
    int  mode;        // 0 = L293, 1 = L298
    int  kanaal1;     // LEDC kanaal voor pin1 (altijd PWM)
    int  kanaal2;     // LEDC kanaal voor pin2 (alleen bij mode 0)
    int  inited;
} dcmotor_t;

// motors[1..4] worden gebruikt, [0] niet
static dcmotor_t motors[5] = {
    {0,0,0,-1,-1,0},
    {0,0,0,-1,-1,0},
    {0,0,0,-1,-1,0},
    {0,0,0,-1,-1,0},
    {0,0,0,-1,-1,0}
};

static int motor_timer_inited = 0;
static int kanaal_in_gebruik[8] = {0,0,0,0,0,0,0,0};


void DCMotor_setup_timer(void)
{
    if (motor_timer_inited) return;

    ledc_timer_config_t motor_timer = {
        .speed_mode      = LEDC_LOW_SPEED_MODE,
        .timer_num       = MOTOR_TIMER,
        .duty_resolution = LEDC_TIMER_8_BIT,
        .freq_hz         = 1000,
        .clk_cfg         = LEDC_AUTO_CLK,
    };
    ledc_timer_config(&motor_timer);
    motor_timer_inited = 1;
}

// Geef een vrij LEDC kanaal terug uit de range die voor motoren is gereserveerd
static int DCMotor_pak_kanaal(void)
{
    for (int k = MOTOR_FIRST_KANAAL; k <= MOTOR_LAAT_KANAAL; k++) {
        if (!kanaal_in_gebruik[k]) {
            kanaal_in_gebruik[k] = 1;
            return k;
        }
    }
    return -1;
}

static void DCMotor_geef_kanaal_terug(int k)
{
    if (k >= 0 && k < 8) kanaal_in_gebruik[k] = 0;
}


// Configureer een GPIO als LEDC PWM pin op het opgegeven kanaal
void DCMotor_attach_pwm(int gpio, int kanaal, int duty)
{
    ledc_channel_config_t channel_cfg = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel    = kanaal,
        .timer_sel  = MOTOR_TIMER,
        .intr_type  = LEDC_INTR_DISABLE,
        .gpio_num   = gpio,
        .duty       = duty,
        .hpoint     = 0,
    };
    ledc_channel_config(&channel_cfg);
}

void DCMotor_set_pwm(int kanaal, int duty)
{
    ledc_set_duty(LEDC_LOW_SPEED_MODE, kanaal, duty);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, kanaal);
}


void handle_dcmotor_init(void)
{
    char num_c, pin1_c, pin2_c, mode_c;
    mySerial_Readn(&num_c, 1);
    if (num_c < '1' || num_c > '4') return;
    int n = num_c - '0';

    mySerial_Readn(&pin1_c, 1);
    mySerial_Readn(&pin2_c, 1);
    mySerial_Readn(&mode_c, 1);

    int pin1 = pin1_c - 48;
    int pin2 = pin2_c - 48;
    int mode = mode_c - '0';

    if (mode != 0 && mode != 1) {
        printf("ongeldige motor mode\n");
        return;
    }

    DCMotor_setup_timer();

    dcmotor_t *m = &motors[n];

    // als de motor al inited was, geef oude kanalen terug
    if (m->kanaal1 >= 0) DCMotor_geef_kanaal_terug(m->kanaal1);
    if (m->kanaal2 >= 0) DCMotor_geef_kanaal_terug(m->kanaal2);

    m->pin1 = pin1;
    m->pin2 = pin2;
    m->mode = mode;
    m->kanaal1 = DCMotor_pak_kanaal();
    m->kanaal2 = (mode == 0) ? DCMotor_pak_kanaal() : -1;

    if (m->kanaal1 < 0) {
        printf("motor %d: geen vrij PWM kanaal beschikbaar\n", n);
        return;
    }

    // pin1 is altijd PWM
    DCMotor_attach_pwm(pin1, m->kanaal1, 0);

    if (mode == 0) {
        // pin2 ook PWM
        if (m->kanaal2 >= 0) DCMotor_attach_pwm(pin2, m->kanaal2, 0);
    } else {
        // pin2 is gewone GPIO output (richting), start LOW
        gpio_reset_pin(pin2);
        gpio_set_direction(pin2, GPIO_MODE_OUTPUT);
        gpio_set_level(pin2, 0);
    }

    m->inited = 1;
    printf("motor %d init: pin1=%d pin2=%d mode=%d\n", n, pin1, pin2, mode);
    mySerial_WriteString("OK");
}


void DCMotor_release(int n)
{
    dcmotor_t *m = &motors[n];
    if (!m->inited) return;
    DCMotor_set_pwm(m->kanaal1, 0);
    if (m->mode == 0 && m->kanaal2 >= 0) DCMotor_set_pwm(m->kanaal2, 0);
    printf("motor %d losgelaten\n", n);
}


void handle_dcmotor_speed(void)
{
    char num_c, dir_c;
    mySerial_Readn(&num_c, 1);
    if (num_c < '1' || num_c > '4') return;
    int n = num_c - '0';

    mySerial_Readn(&dir_c, 1);

    if (dir_c == 'r') {
        DCMotor_release(n);
        return;
    }

    if (dir_c != '0' && dir_c != '1') {
        printf("ongeldige motor richting\n");
        return;
    }
    int richting = dir_c - '0';

    char snelheid_byte;
    mySerial_Readn(&snelheid_byte, 1);
    int snelheid = (uint8_t)snelheid_byte;

    dcmotor_t *m = &motors[n];
    if (!m->inited) {
        printf("motor %d nog niet geinitialiseerd\n", n);
        return;
    }

    if (m->mode == 0) {
        // L293: twee PWM pinnen, een ervan op snelheid, ander op 0
        if (richting == 1) {
            DCMotor_set_pwm(m->kanaal1, snelheid);
            if (m->kanaal2 >= 0) DCMotor_set_pwm(m->kanaal2, 0);
        } else {
            if (m->kanaal2 >= 0) DCMotor_set_pwm(m->kanaal2, snelheid);
            DCMotor_set_pwm(m->kanaal1, 0);
        }
    } else {
        // L298: PWM op pin1, richting via GPIO op pin2
        gpio_set_level(m->pin2, richting);
        DCMotor_set_pwm(m->kanaal1, snelheid);
    }

    printf("motor %d richting=%d snelheid=%d\n", n, richting, snelheid);
}
