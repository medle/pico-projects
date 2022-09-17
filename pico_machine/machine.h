
#include <stdio.h>
#include <assert.h>
#include "pico/stdlib.h"

void mach_start_pwm(uint hz, float duty, void (*wrap_handler)());
void mach_stop_pwm();

/* Board LED functions. */
void led_set(bool on);
void led_run_startup_welcome();
void led_blink(int num_blinks, int ms_delay_each);

/* Panic management functions. */
void __expect0(int code, const char *file, int line, const char *expr);
void __assert_failure(int code, const char *file, int line, const char *expr);

#ifdef NDEBUG
 #define expect0(__e) (__e)
 #define assert(__e) ((void)0)
#else
 #define expect0(__e) __expect0((__e), __FILE__, __LINE__, #__e) 
 #define assert(__e) ((__e) ? (void)0 : __assert_failure(0, __FILE__, __LINE__, #__e))
#endif
