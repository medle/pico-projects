
#include "encoder.h"
#include <hardware/gpio.h>
#include "dac_mcp4921.h"
#include "quadrature.pio.h"
#include "eeprom_24cxx.h"

int g_count;
char g_buffer[100];

// see button debounce library https://github.com/TuriSc/RP2040-Button
void gpio_callback(uint gpio, uint32_t events);

static void encoder_init();
static int encoder_read();

int falls = 0;
int rises = 0;

int main() 
{
    stdio_init_all();
    sleep_ms(500);

    eeprom_init();
    encoder_init();

    //eeprom_scan_bus();

    //uint32_t hey;
    //int ret = eeprom_read_bytes(0, (uint8_t *)&hey, sizeof(hey));
    //printf("eeprom read=%d 0x%x\n", ret, hey);

    encoder_led_init(); 
    encoder_display_init(); 

    mcp4921_init();

    sprintf(g_buffer, "ready");
    encoder_display_repaint();

    const int cycle_ms = 100; 

    uint16_t v = 0; 
    uint16_t max_v = 0x0fff;
    
    while(true) {
        
        mcp4921_write_dac(v); 

        int n = encoder_read();

        float f_v = (v * 3.3) / max_v;
        sprintf(g_buffer, "v=%.2f %d", f_v, n);

        v += max_v / (5 * 20);
        if (v > max_v) v = 0;
        
        //sprintf(g_buffer, "e%d r%d f%d", n, rises, falls); 

        // display update lasts 15ms
        encoder_display_repaint();

        encoder_led_set(true); 
        sleep_ms(cycle_ms);
        encoder_led_set(false); 
        sleep_ms(cycle_ms);
    }

    return 0;
}

void gpio_callback(uint gpio, uint32_t events) {
    if (gpio == ENCODER_KEY_PIN) {
        if (events & GPIO_IRQ_EDGE_RISE) rises += 1; 
        if (events & GPIO_IRQ_EDGE_FALL) falls += 1; 
    }
    //sprintf(g_buffer, "%d r%d f%d", last, rises, falls);
}

#define COUNT_MODE_1X 2
#define COUNT_MODE_2X 1
#define COUNT_MODE_4X 0

static PIO s_encoder_pio;
static int s_encoder_sm;
static int s_encoder_count_mode;

static void encoder_init()
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

static int encoder_read()
{
    return quadrature_encoder_get_count(s_encoder_pio, s_encoder_sm) >> s_encoder_count_mode;
}

