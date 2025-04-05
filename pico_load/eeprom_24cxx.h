
#ifndef _EEPROM_24CXX_H
#define _EEPROM_24CXX_H

#include "pico/stdlib.h"

void eeprom_init();
void eeprom_scan_bus();
int eeprom_read_bytes(uint16_t address, uint8_t *target, uint len);
int eeprom_write_bytes(uint16_t address, uint8_t *source, uint len);

// _EEPROM_24CXX_H
#endif
