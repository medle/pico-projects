//
// SPI driver for the MCP4921 12-bit Digital-to-Analog converter.
// SL (18.02.2025)
//
#include "board_config.h"
#include <hardware/spi.h>
#include <hardware/gpio.h>
#include <pico/stdlib.h>

// LDAC pin is pulled low so conversion starts on CS up. 
#define MCP_SPI_INSTANCE BOARD_SPI_INSTANCE
#define MCP_SPI_BAUDRATE BOARD_SPI_BAUDRATE
#define MCP_SPI_SCK_PIN BOARD_SPI_SCK_PIN
#define MCP_SPI_TX_PIN BOARD_SPI_TX_PIN
#define MCP_SPI_CS_PIN BOARD_SPI_CS1_PIN

void mcp4921_init()
{
    spi_init(MCP_SPI_INSTANCE, MCP_SPI_BAUDRATE);

    // All writes to the MCP492X are 16-bit words. 
    spi_set_format(MCP_SPI_INSTANCE, 16, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);

    gpio_set_function(MCP_SPI_SCK_PIN, GPIO_FUNC_SPI);
    gpio_set_function(MCP_SPI_TX_PIN, GPIO_FUNC_SPI);
    gpio_set_function(MCP_SPI_CS_PIN, GPIO_FUNC_SPI);
}

void mcp4921_deinit()
{
    spi_deinit(MCP_SPI_INSTANCE);
}

// Enable chip communication.
static inline void cs_select() 
{
    asm volatile("nop \n nop \n nop");
    gpio_put(MCP_SPI_CS_PIN, 0);  // Active low
    asm volatile("nop \n nop \n nop");
}

// Disable chip communication.
static inline void cs_deselect() 
{
    asm volatile("nop \n nop \n nop");
    gpio_put(MCP_SPI_CS_PIN, 1);
    asm volatile("nop \n nop \n nop");
}

// Write the 12-bit value to the MCP.
void mcp4921_write_dac(uint16_t data)
{
    // Build the 16-bit register word, 4 MSB bits are flags, 12 LSB bits are data.
    uint16_t reg = 
      (0 << 15) | // channel A=0, channel B=1
      (0 << 14) | // buffered=1 (high impendance), unbuffered=0 (full range with 165kOhm impendance)
      (1 << 13) | // gain 1x=1, gain 2x=0
      (1 << 12) | // power up=1, power down=0
      data & 0x0FFF; 

      cs_select();
      spi_write16_blocking(MCP_SPI_INSTANCE, &reg, 1);
      cs_deselect();

      // settling time is 4.5uSec
      sleep_us(10);
}

