
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "pico/stdlib.h"

// Painting functions.
void showLogo();
void drawStartStopButton(bool on);
void generateSineWaveValues(uint8_t *values, int numValues);
void drawGraph(uint8_t *values, int numValues, int color);
void drawGraphGrid();

// PWM functions.
#define PWM_GPIO 0
typedef struct PwmTopDivider { uint16_t top; uint16_t divider; } PwmTopDivider;
PwmTopDivider pwmChooseTopDivider(uint periodsPerSecond, bool dualSlope);
void pwmInit(uint gpio);
bool pwmStart(uint hz, float duty);
bool pwmStop();

// Board LED functions. 
void led_set(bool on);
void led_run_startup_welcome();
void led_blink(int num_blinks, int ms_delay_each);

// Panic management functions.
void __expect0(int code, const char *file, int line, const char *expr);
void __assert_failure(int code, const char *file, int line, const char *expr);

#ifdef NDEBUG
 #define expect0(__e) (__e)
 //#define assert(__e) ((void)0)
#else
 #define expect0(__e) __expect0((__e), __FILE__, __LINE__, #__e) 
 //#define assert(__e) ((__e) ? (void)0 : __assert_failure(0, __FILE__, __LINE__, #__e))
#endif
