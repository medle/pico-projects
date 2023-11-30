
#include "m2_globals.h"
#include "hardware/adc.h"
#include "hardware/dma.h"

// ADC channels [0, 1, 2] are [GPIO26, GPIO27, GPIO28]
#define FIRST_ADC_GPIO 26

static void setupDmaFifo();
static uint prepareDmaChannel(uint8_t *captureBuffer, uint captureDepth);

static uint _dmaChannel;
static volatile uint _measureState = 0;

//
// Perform initial setup for ADC functions.
//
void machAdcInit()
{
    adc_init();

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
void machAdcHandlePeriodEnd()
{
    switch(_measureState) {
        case 1: _measureState = 2; break; // period starts
        case 2: _measureState = 0; break; // period starts
    }
}

// This bit-banger version produces 25% less values than DMA version but is stable
static uint measurePeriodByLoop(AdcChannel adcChannel, uint8_t *buffer, uint bufferSize)
{
    adc_gpio_init(FIRST_ADC_GPIO + adcChannel);
    adc_select_input(adcChannel);
    
    int n = 0;

    _measureState = 1;
    while(_measureState != 2) tight_loop_contents();
    while(_measureState != 0) {
        uint16_t value12bit = adc_read();
        buffer[n] = (uint8_t)(value12bit >> 4); // 12bit to 8bit scaling
        if(++n == bufferSize) break;
    }  

    return n;
}

// This DMA version converts values at full speed but produces junk each odd call
static uint measurePeriodByDma(AdcChannel adcChannel, uint8_t *buffer, uint bufferSize)
{
    adc_gpio_init(FIRST_ADC_GPIO + adcChannel);
    adc_select_input(adcChannel);

    _dmaChannel = prepareDmaChannel(buffer, bufferSize);

    _measureState = 1;
    while(_measureState != 2) tight_loop_contents();
    adc_run(true); 
    while(_measureState != 0) tight_loop_contents();
    adc_run(false);  
    adc_fifo_drain(); 

    uint8_t *end = (uint8_t *)dma_channel_hw_addr(_dmaChannel)->write_addr; 

    dma_channel_unclaim(_dmaChannel);

    return (end - buffer);
}

uint machAdcMeasurePeriod(AdcChannel adcChannel, uint8_t *buffer, uint bufferSize)
{
    return measurePeriodByLoop(adcChannel, buffer, bufferSize);
}

static uint prepareDmaChannel(uint8_t *captureBuffer, uint captureDepth)
{
    setupDmaFifo();

    // Set up the DMA to start transferring data as soon as it appears in FIFO
    uint dmaChannel = dma_claim_unused_channel(true);
    dma_channel_config cfg = dma_channel_get_default_config(dmaChannel);

    // Reading from constant address, writing to incrementing byte addresses
    channel_config_set_transfer_data_size(&cfg, DMA_SIZE_8);
    channel_config_set_read_increment(&cfg, false); 
    channel_config_set_write_increment(&cfg, true); 

    // Pace transfers based on availability of ADC samples
    channel_config_set_dreq(&cfg, DREQ_ADC);

    dma_channel_configure(dmaChannel, &cfg,
        captureBuffer,  // initial write address
        &adc_hw->fifo,  // initial read address
        captureDepth,   // transfer count
        true            // start immediately
    );

    return dmaChannel;
}

static void setupDmaFifo()
{
    adc_fifo_setup(
            true,    // Write each completed conversion to the sample FIFO
            true,    // Enable DMA data request (DREQ)
            1,       // DREQ (and IRQ) asserted when at least 1 sample present
            false,   // We won't see the ERR bit because of 8 bit reads; disable.
            true     // Shift each sample to 8 bits when pushing to FIFO
        );
}
