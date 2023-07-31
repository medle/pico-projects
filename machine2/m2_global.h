
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pico/stdlib.h>

// Display painting functions.
void drawLogo();
void drawStartStopButton(bool running, bool tick);
void generateSineWaveValues(uint8_t *values, int numValues);
void drawGraph(uint8_t *values, int numValues, int color);
void drawGraphGrid();

// PWM functions.
#define MACH_PWM_GPIO_A 0
#define MACH_PWM_GPIO_B 1
typedef struct PwmTopDivider { uint16_t top; uint16_t divider; } PwmTopDivider;
PwmTopDivider pwmChooseTopDivider(uint periodsPerSecond, bool dualSlope);
void machPwmInit();
bool machPwmStart(uint hz, float duty, void (*wrapHandler)());
void machPwmResetCounter();
bool machPwmStop();

// Zero-cross sensing functions.
#define MACH_SENSE_GPIO 4
void machSenseEnable(bool on);

// ADC functions.
typedef enum AdcChannel { ADC_CH0, ADC_CH1, ADC_CH2 } AdcChannel;
void machAdcInit();
uint machAdcMeasurePeriod(AdcChannel adcChannel, uint8_t *buffer, uint bufferSize);
void machAdcHandlePeriodEnd(); 

// Board LED functions. 
void ledSet(bool on);
void ledRunStartupWelcome();
void ledBlink(int numBlinks, int msDelayEach);

// Asyncronous waiting.
typedef struct WaitToken { uint64_t stopTime; uint periodMs; } WaitToken;
void beginWait(WaitToken *pToken, uint periodMs);
bool waitCompleted(WaitToken *pToken, bool restart);

// Panic management functions.
void __expect(int value, int expected, const char *file, int line, const char *expr);
void __assert_failure(int code, const char *file, int line, const char *expr);

#ifdef NDEBUG
 #define expect0(__e) (__e)
 #define expect1(__e) (__e)
 //#define assert(__e) ((void)0)
#else
 #define expect0(__e) __expect0((__e), 0, __FILE__, __LINE__, #__e) 
 #define expect1(__e) __expect1((__e), 1, __FILE__, __LINE__, #__e) 
 //#define assert(__e) ((__e) ? (void)0 : __assert_failure(0, __FILE__, __LINE__, #__e))
#endif
