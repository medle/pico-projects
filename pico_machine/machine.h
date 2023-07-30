
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "pico/stdlib.h"

enum {
  HELLO_COMMAND_ID,
  PWM_COMMAND_ID,
  ADC_COMMAND_ID,
  STOP_COMMAND_ID,
  SET_COMMAND_ID,
  RUN_COMMAND_ID
};

typedef struct  
{
  int command_id;
  int parameter_1;
  int parameter_2;
  int parameter_3;
  char set_name[20];
} user_command_t;

typedef struct 
{
  uint divider;
  uint wrap;
  uint level;
} pwm_config_t;

// Machine core functions.
void mach_init();
bool mach_execute_command_and_respond(user_command_t *command);

// PWM functions.
void mach_pwm_init();
void mach_pwm_start(uint hz, float duty, void (*wrap_handler)());
void mach_pwm_change_waveform(uint hz, float duty);
bool mach_pwm_is_running();
pwm_config_t mach_pwm_get_config();
void mach_pwm_stop();
void mach_pwm_set_dead_clocks(uint dead_clocks);
void mach_pwm_set_one_sided(bool one_sided);

// ADC functions.
void machAdcInit();
uint machAdcMeasurePeriod(uint channel_num, uint8_t *buffer, uint buffer_size);
void machAdcHandlePeriodEnd(); 

// User command format functions.
void command_parse_input_char(char ch);
void command_respond_success_begin();
void command_respond_data(char *s);
bool command_respond_end(bool result);
bool command_respond_success(char *message);
bool command_respond_syntax_error(char *message);
bool command_respond_user_error(char *message, char *param);

// PWM helpers.
uint16_t compute_pwm_match_level(uint16_t top, float duty, bool inverted);
uint16_t compute_pwm_top_and_divider(uint hz, bool dual_slope, uint *divider_ptr);

// Direct drive functions.
void DirectInit();
void DirectSetMaxWaves(uint waves);
bool DirectRunAndRespond(int hertz, float duty);
void DirectStop();
bool DirectStopAndRespond();
bool DirectIsRunning();

// Board LED functions. 
void ledSet(bool on);
void ledRunStartupWelcome();
void ledBlink(int num_blinks, int ms_delay_each);

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
