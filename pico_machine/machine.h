
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
void mach_adc_init();
uint mach_adc_measure_period(uint channel_num, uint8_t *buffer, uint buffer_size);
void mach_adc_handle_period_end(); 

// User command format functions.
void command_parse_input_char(char ch);
bool command_respond_success(char *message);
bool command_respond_syntax_error(char *message);
bool command_respond_user_error(char *message, char *param);

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
