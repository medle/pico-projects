
#ifndef _EASY_EEPROM_H
#define _EASY_EEPROM_H

#include "pico/stdlib.h"

#ifdef __cplusplus
extern "C" {
#endif

void easy_eeprom_init();
void easy_eeprom_scan_bus();
int easy_eeprom_read_bytes(uint16_t address, uint8_t *target, uint len);
int easy_eeprom_write_bytes(uint16_t address, uint8_t *source, uint len);

#ifdef __cplusplus
}
#endif

// _EASY_EEPROM_H
#endif
