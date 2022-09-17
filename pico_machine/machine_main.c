
#include "machine.h"

// For ADC input
#include "hardware/adc.h"
#include "hardware/dma.h"

// ADC channel 0 is GPIO26
#define CAPTURE_CHANNEL 0
#define CAPTURE_DEPTH 200
uint8_t capture_buf[CAPTURE_DEPTH];

static void setup_adc()
{
    // Init GPIO for analogue use: hi-Z, no pulls, disable digital input buffer.
    adc_gpio_init(26 + CAPTURE_CHANNEL);

    adc_init();
    adc_select_input(CAPTURE_CHANNEL);
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
    adc_set_clkdiv(8);
}

static uint dma_chan;

static void prepare_dma()
{
    // Set up the DMA to start transferring data as soon as it appears in FIFO
    dma_chan = dma_claim_unused_channel(true);
    dma_channel_config cfg = dma_channel_get_default_config(dma_chan);

    // Reading from constant address, writing to incrementing byte addresses
    channel_config_set_transfer_data_size(&cfg, DMA_SIZE_8);
    channel_config_set_read_increment(&cfg, false); 
    channel_config_set_write_increment(&cfg, true); 

    // Pace transfers based on availability of ADC samples
    channel_config_set_dreq(&cfg, DREQ_ADC);

    dma_channel_configure(dma_chan, &cfg,
        capture_buf,    // initial write address
        &adc_hw->fifo,  // initial read address
        CAPTURE_DEPTH,  // transfer count
        true            // start immediately
    );
}

static void release_dma()
{
    dma_channel_unclaim(dma_chan);
}

static void run_adc_capture()
{
    //printf("Starting capture\n");
    adc_run(true);

    // Once DMA finishes, stop any new conversions from starting, and clean up
    // the FIFO in case the ADC was still mid-conversion.
    dma_channel_wait_for_finish_blocking(dma_chan);
    //printf("Capture finished\n");
    adc_run(false);
    adc_fifo_drain();
}

static void print_capture(uint len)
{
    for (int i = 0; i < len; ++i) {
        printf("%d ", capture_buf[i]);
    }
    printf("\n");
}

static volatile uint mode = 0;

static uint wrap_count = 0;
static void on_wrap() 
{
    wrap_count += 1;

    if (mode == 1) {
        adc_run(true);
        mode = 2;  
    } else if (mode == 2) {
        adc_run(false);
        mode = 0;
    }
}

int main() 
{
    stdio_init_all(); 
    led_run_startup_welcome();
    printf("Starting.\n"); 

    mach_start_pwm(8300, 25, on_wrap);

    setup_adc();
    printf("Setup ADC done.\n"); 

    int i = 0;
    while (true) {
        sleep_ms(1000);
        led_set(1);
        sleep_ms(5);
        led_set(0);
        printf("%d: wrap_count=%d\n", ++i, wrap_count);

        prepare_dma();

        //run_adc_capture();
        mode = 1;
        while(mode != 0) tight_loop_contents();

        adc_fifo_drain(); 
        uint32_t addr = dma_channel_hw_addr(dma_chan)->write_addr;
        release_dma();

        uint32_t start = (uint32_t)capture_buf;  
        uint len = addr - start;
        printf("buf=%x addr=%x len=%d\n", start, addr, len);

        print_capture(len);
       
    }
}

