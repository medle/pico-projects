
#include "load.h"
#include "encoder.h"
#include <hardware/gpio.h>
#include "quadrature.pio.h"

#define COUNT_MODE_1X 2
#define COUNT_MODE_2X 1
#define COUNT_MODE_4X 0

static PIO s_encoder_pio;
static int s_encoder_sm;
static int s_encoder_count_mode;

void encoder_init()
{
    //gpio_init(ENCODER_KEY_PIN);
    //gpio_set_dir(ENCODER_KEY_PIN, GPIO_IN);

    //gpio_set_irq_enabled_with_callback(ENCODER_KEY_PIN, 
    //    GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL,
    //    true, &gpio_callback); 

    uint pio_pin = ENCODER_S1_PIN;
    uint max_step_rate = 0;
    gpio_init(pio_pin);
    gpio_set_dir(pio_pin, GPIO_IN);
    gpio_init(pio_pin + 1);
    gpio_set_dir(pio_pin + 1, GPIO_IN);

    PIO pio = pio0;
    uint offset = pio_add_program(pio, &quadrature_encoder_program); 
    uint sm = pio_claim_unused_sm(pio, true);
    quadrature_encoder_program_init(pio,sm, offset, pio_pin, max_step_rate);

    s_encoder_count_mode = COUNT_MODE_1X;
    s_encoder_pio = pio;
    s_encoder_sm = sm;
}

int encoder_get_count()
{
    return quadrature_encoder_get_count(s_encoder_pio, s_encoder_sm) >> s_encoder_count_mode;
}

