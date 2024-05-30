
#include "smps.h"

#include "hardware/pio.h"

#include "smps_repeater.pio.h"

static void my_pio_irq_handler()
{
    // check if PIO0_IRQ_0=7 is set
    if (pio_interrupt_get(pio0, 0)) {
        _smps_limit_occured = true;
        pio_interrupt_clear(pio0, 0);
    }
}

void smps_pio_start_repeater()
{
    // find and init an available state machine
    PIO pio = pio0;
    int sm = pio_claim_unused_sm(pio0, true);
    uint offset = pio_add_program(pio, &smps_repeater_program);

    gpio_init(LIMITER_PWM_INPUT_PIN);
    gpio_set_dir(LIMITER_PWM_INPUT_PIN, GPIO_IN);
    gpio_pull_down(LIMITER_PWM_INPUT_PIN);

    gpio_init(LIMITER_INPUT_PIN);
    gpio_set_dir(LIMITER_INPUT_PIN, GPIO_IN);
    gpio_pull_down(LIMITER_INPUT_PIN);
    
    gpio_init(LIMITER_OUTPUT_PIN);
    gpio_set_dir(LIMITER_OUTPUT_PIN, GPIO_OUT);

    smps_repeater_program_init(pio, sm, offset, LIMITER_FIRST_PIN);

    // enable IRQ for the state machine
    uint irq_num = PIO0_IRQ_0;
    pio_set_irq0_source_enabled(pio, pis_interrupt0, true); 
    irq_set_exclusive_handler(irq_num, my_pio_irq_handler);
    irq_set_enabled(irq_num, true);

    // start the state machine 
    pio_sm_set_enabled(pio, sm, true);
}

