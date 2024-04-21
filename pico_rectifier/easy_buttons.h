
#ifndef _EASY_BUTTONS_H
#define _EASY_BUTTONS_H

#include "pico/stdlib.h"

#ifdef __cplusplus
extern "C" {
#endif

bool easy_buttons_register(uint gpio, void (*callback)(uint, bool), bool released_value);
bool easy_buttons_unregister(uint gpio);
void easy_buttons_sleep_ms(uint ms);

#ifdef __cplusplus
}
#endif

// _EASY_BUTTONS_H
#endif
