
#include "machine.h"
#include "hardware/adc.h"
#include "hardware/dma.h"

// ADC channels [0, 1, 2] are [GPIO26, GPIO27, GPIO28]
#define FIRST_ADC_GPIO 26
#define MIN_ADC_CHANNEL 0
#define MAX_ADC_CHANNEL 2

static uint prepare_dma_channel(uint8_t *capture_buffer, uint capture_depth);

static uint _dma_channel;
static volatile uint _measure_state = 0;
static volatile uint32_t _capture_end_addr;

//
// Perform initial setup for ADC functions.
//
void mach_adc_init()
{
    adc_init();
    adc_fifo_setup(
        true,    // Write each completed conversion to the sample FIFO
        true,    // Enable DMA data request (DREQ)
        1,       // DREQ (and IRQ) asserted when at least 1 sample present
        false,   // We won't see the ERR bit because of 8 bit reads; disable.
        true     // Shift each sample to 8 bits when pushing to FIFO
    );

    // Divisor of 0 -> full speed. Free-running capture with the divider is
    // equivalent to pressing the ADC_CS_START_ONCE button once per `div + 1`
    // cycles (div not necessarily an integer). Each conversion takes 96
    // cycles, so in general you want a divider of 0 (hold down the button
    // continuously) or > 95 (take samples less frequently than 96 cycle
    // intervals). This is all timed by the 48 MHz ADC clock.
    adc_set_clkdiv(0);
}

//
// This function is called by user from a PWM WRAP-event 
// IRQ handler. It needs to return control quickly.
//
// NOTE: By some unknown reason we need to pass through the
// several periods of PWM so the FIRST period will produce the
// most stable number of samples out of ADC. When we reduce
// the number of periods to two - the FIRST period becomes
// LESS stable in the number of ADC samples. The last period
// is very unstable in the number of ADC samples. Somehow
// the past (first ADC period) depends on the future (last
// ADC period). For example at 8300Hz PWM we get stable 60 
// samples during the FIRST period and average 45 samples 
// during the LAST (third) period.
void mach_adc_handle_period_end()
{
    switch(_measure_state) {

        // State 1: start the ADC and go to state 2
        case 1: 
            adc_run(true); 
            _measure_state = 2; 
            break;

        // State 2: save the current capture end and go to state 3    
        case 2: 
            _capture_end_addr = dma_channel_hw_addr(_dma_channel)->write_addr; 
            _measure_state = 3;
            break;

        // State 3: just pass on for the sake of stability and go to state 4    
        case 3:
            _measure_state = 4;
            break;

        // State 4: just pass on for the sake of stability and go to state 5    
        case 4:
            _measure_state = 5;
            break;

        // State 5: stop the ADC and return to the idle zero state     
        case 5:
            adc_run(false); 
            _measure_state = 0;
            break;
    }
}

//
// Returns the number of samples measured during the period and placed into the buffer.
//
uint mach_adc_measure_period(uint adc_channel, uint8_t *buffer, uint buffer_size)
{
    assert(adc_channel >= MIN_ADC_CHANNEL && adc_channel <= MAX_ADC_CHANNEL); 

    // Init GPIO for analogue use: hi-Z, no pulls, disable digital input buffer.
    adc_gpio_init(FIRST_ADC_GPIO + adc_channel);
    adc_select_input(adc_channel);

    // Allocate and setup DMA channel
    _dma_channel = prepare_dma_channel(buffer, buffer_size);

    // Perform the ADC capture. 
    // User calls the mach_adc_handle_period_end() function at period wraps.
    _measure_state = 1;
    while(_measure_state != 0) tight_loop_contents();
    adc_fifo_drain(); 

    // Release the alocated DMA channel     
    dma_channel_unclaim(_dma_channel);

    // Return the number of samples captured
    return (_capture_end_addr - (uint32_t)buffer);
}

static uint prepare_dma_channel(uint8_t *capture_buffer, uint capture_depth)
{
    // Set up the DMA to start transferring data as soon as it appears in FIFO
    uint dma_channel = dma_claim_unused_channel(true);
    dma_channel_config cfg = dma_channel_get_default_config(dma_channel);

    // Reading from constant address, writing to incrementing byte addresses
    channel_config_set_transfer_data_size(&cfg, DMA_SIZE_8);
    channel_config_set_read_increment(&cfg, false); 
    channel_config_set_write_increment(&cfg, true); 

    // Pace transfers based on availability of ADC samples
    channel_config_set_dreq(&cfg, DREQ_ADC);

    dma_channel_configure(dma_channel, &cfg,
        capture_buffer, // initial write address
        &adc_hw->fifo,  // initial read address
        capture_depth,  // transfer count
        true            // start immediately
    );

    return dma_channel;
}
