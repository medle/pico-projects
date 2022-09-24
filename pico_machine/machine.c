
#include "machine.h"

#define PROGRAM_NAME "InductorMachine"

// maximum number of samples we can deliver in one ADC batch
#define MAX_SAMPLES 200

// each sample produces 3 digits and one space
#define MAX_RESPONSE (MAX_SAMPLES * 4) 

static uint8_t _capture_buffer[MAX_SAMPLES];
static char _response_buffer[MAX_RESPONSE];

//
// PWM IRQ end-of-period (wrap) handler.
//
static void __on_wrap_irq_handler() 
{
    mach_adc_handle_period_end();
}

//
// Executes the PWM user command, starts PWM with the given parameters.
//
static bool execute_pwm_and_respond(uint hz, uint duty1024, uint dead_clocks)
{
    if (hz < 10)
        return command_respond_user_error("hz < 10", NULL);

    if (duty1024 >= 1024)
        return command_respond_user_error("duty cycle out of range [0,1023]", NULL);

    if (dead_clocks < 0 || dead_clocks > 200)
        return command_respond_user_error("dead clocks value out of range [0, 200]", NULL);

    if(mach_pwm_is_running())     
        return command_respond_user_error("PWM is already running", NULL);

    float duty = (float)(duty1024 * 100) / 1023;

    mach_pwm_start(hz, duty, dead_clocks, __on_wrap_irq_handler);
    return command_respond_success("PWM is enabled.");
}

//
// Executes the ADC user command, records ADC sample batch and sends it back to user.
//
static bool execute_adc_batch_and_respond(uint adc_channel)
{
    if (adc_channel < 0 || adc_channel > 2)
        return command_respond_user_error("ADC channel out of range [0,2]->[GPIO26,GPIO28]", NULL);

    if(!mach_pwm_is_running())     
        return command_respond_user_error("Can't record ADC when PWM isn't running", NULL);
     
    int num_samples = mach_adc_measure_period(adc_channel, _capture_buffer, sizeof(_capture_buffer));

    // produce a string of space separated sample values
    _response_buffer[0] = '\0';
    for(int i = 0; i < num_samples; i++) {
        char temp[10];
        uint8_t sample = _capture_buffer[i];
        itoa(sample, temp, 10);  
        if(i > 0) strcat(_response_buffer, " ");
        strcat(_response_buffer, temp);
    }     
  
  return command_respond_success(_response_buffer);
}

//
// Executes the STOP user command, stops everything running now.
//
static bool execute_stop_and_respond()
{
    if(mach_pwm_is_running()) mach_pwm_stop();
    return command_respond_success("PWM is disabled.");
}

bool mach_execute_command_and_respond(user_command_t *command_ptr)
{
    switch(command_ptr->command_id) {

        case PWM_COMMAND_ID: 
            return execute_pwm_and_respond(command_ptr->parameter_1, 
                command_ptr->parameter_2, command_ptr->parameter_3); 

        case ADC_COMMAND_ID:
            return execute_adc_batch_and_respond(command_ptr->parameter_1);

        case STOP_COMMAND_ID:
            return execute_stop_and_respond();

        case HELLO_COMMAND_ID:
            return command_respond_success
                (PROGRAM_NAME " ready (version " __DATE__ " " __TIME__ ").");
    } 

    return command_respond_user_error("unknown command", NULL);
}

