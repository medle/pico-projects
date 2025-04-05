
#ifndef DAC_MCP4921_H
#define DAC_MCP4921_H

#include <pico/stdlib.h>

void mcp4921_init();
void mcp4921_deinit();

/// @brief Writes the 12-bit value to the MCP4921 DAC.
/// @param data The 12-bit value to write into DAC.
void mcp4921_write_dac(uint16_t data);

#define MCP4921_MAX_VALUE 0x0fff

#endif