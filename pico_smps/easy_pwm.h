
#ifndef _EASY_PWM_H
#define _EASY_PWM_H

#include "pico/stdlib.h"

#ifdef __cplusplus
extern "C" {
#endif

void easy_pwm_enable(uint gpio, uint hz, float duty);
void easy_pwm_disable(uint gpio);

#ifdef __cplusplus
}
#endif

// _EASY_PWM_H
#endif
