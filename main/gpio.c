#include "gpio.h"
#include "driver/gpio.h"

// I/O setup
void gpio_setup()
{
    // GPIO34 is input-only and has no internal pull-down on ESP32.
    gpio_set_direction(SYNC_PIN, GPIO_MODE_INPUT);
    gpio_set_direction(IN_S_PIN, GPIO_MODE_INPUT);
    // IN_MODE: external DC signal (HIGH = tasks C/D enabled, LOW = disabled)
    gpio_set_direction(IN_MODE_PIN, GPIO_MODE_INPUT);

    gpio_set_direction(ACK_A,   GPIO_MODE_OUTPUT);
    gpio_set_direction(ACK_B,   GPIO_MODE_OUTPUT);
    gpio_set_direction(ACK_AGG, GPIO_MODE_OUTPUT);
    gpio_set_direction(ACK_C,   GPIO_MODE_OUTPUT);
    gpio_set_direction(ACK_D,   GPIO_MODE_OUTPUT);
    gpio_set_direction(ACK_S,   GPIO_MODE_OUTPUT);
}

int read_SYNC() { return gpio_get_level(SYNC_PIN);    }
int read_IN_S() { return gpio_get_level(IN_S_PIN);    }
int read_IN_MODE() { return gpio_get_level(IN_MODE_PIN); }

void set_ack_A(int v) { gpio_set_level(ACK_A,   v); }
void set_ack_B(int v) { gpio_set_level(ACK_B,   v); }
void set_ack_AGG(int v) { gpio_set_level(ACK_AGG, v); }
void set_ack_C(int v) { gpio_set_level(ACK_C,   v); }
void set_ack_D(int v) { gpio_set_level(ACK_D,   v); }
void set_ack_S(int v)   { gpio_set_level(ACK_S,   v); }