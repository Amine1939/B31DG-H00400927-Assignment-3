#include "edge_counter.h"
#include "driver/pulse_cnt.h"
#include "esp_err.h"

#define IN_A_PIN 18
#define IN_B_PIN 19

static pcnt_unit_handle_t unitA = NULL;
static pcnt_unit_handle_t unitB = NULL;

static int prevA = 0;
static int prevB = 0;

static void init_unit(pcnt_unit_handle_t *unit, int pin)
{
    pcnt_unit_config_t config = {
        .low_limit  = -32768,
        .high_limit =  32767,
    };
    ESP_ERROR_CHECK(pcnt_new_unit(&config, unit));

    pcnt_chan_config_t chan_config = {
        .edge_gpio_num  = pin,
        .level_gpio_num = -1
    };
    pcnt_channel_handle_t chan = NULL;
    ESP_ERROR_CHECK(pcnt_new_channel(*unit, &chan_config, &chan));

    ESP_ERROR_CHECK(pcnt_channel_set_edge_action(
        chan,
        PCNT_CHANNEL_EDGE_ACTION_INCREASE,
        PCNT_CHANNEL_EDGE_ACTION_HOLD));

    ESP_ERROR_CHECK(pcnt_unit_enable(*unit));
    ESP_ERROR_CHECK(pcnt_unit_clear_count(*unit));
    ESP_ERROR_CHECK(pcnt_unit_start(*unit));
}

void edge_counter_init(void)
{
    init_unit(&unitA, IN_A_PIN);
    init_unit(&unitB, IN_B_PIN);
}

int edges_A(void)
{
    int now;
    ESP_ERROR_CHECK(pcnt_unit_get_count(unitA, &now));
    int delta = now - prevA;
    if (delta < 0) delta = 0;
    prevA = now;
    return delta;
}

int edges_B(void)
{
    int now;
    ESP_ERROR_CHECK(pcnt_unit_get_count(unitB, &now));
    int delta = now - prevB;
    if (delta < 0) delta = 0;
    prevB = now;
    return delta;
}