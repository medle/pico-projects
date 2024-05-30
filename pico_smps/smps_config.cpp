
#include "smps_globals.h"
#include "easy_eeprom.h"

#define CONFIG_MAGIC 0xB000

static config_t _default_config = { 
    .magic = CONFIG_MAGIC, 
    .pwm_hz = 5000, 
    .pwm_duty = 0.5,
    .amp_limit = 5
};

config_t _smps_config;

void smps_config_init()
{
    easy_eeprom_init();
}

void smps_config_restore()
{
    int ret = easy_eeprom_read_bytes(0, (uint8_t *)&_smps_config, sizeof(_smps_config));
    if (ret != sizeof(_smps_config) || _smps_config.magic != CONFIG_MAGIC) {
        _smps_config = _default_config;
    }
}

void smps_config_save()
{
    easy_eeprom_write_bytes(0, (uint8_t *)&_smps_config, sizeof(_smps_config));
}

