
#include "smps.h"
#include "easy_eeprom.h"

#define MEMORY_MAGIC 0xB000

static memory_t _default_memory = { 
    .magic = MEMORY_MAGIC, 
    .pwm_hz = 5000, 
    .pwm_duty = 0.5,
    .amp_limit = 5
};

memory_t _smps_memory;

void smps_memory_init()
{
    easy_eeprom_init();
}

void smps_memory_restore()
{
    int ret = easy_eeprom_read_bytes(0, (uint8_t *)&_smps_memory, sizeof(_smps_memory));
    if (ret != sizeof(_smps_memory) || _smps_memory.magic != MEMORY_MAGIC) {
        _smps_memory = _default_memory;
    }
}

void smps_memory_save()
{
    easy_eeprom_write_bytes(0, (uint8_t *)&_smps_memory, sizeof(_smps_memory));
}

