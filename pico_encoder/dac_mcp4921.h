
#ifndef DAC_MCP4921_H
#define DAC_MCP4921_H

#include <pico/stdlib.h>

void mcp4921_init();
void mcp4921_deinit();

// Write the 12-bit value to the MCP4921 DAC.
void mcp4921_write_dac(uint16_t data);

#endif