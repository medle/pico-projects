
#include "machine.h"

#define HELLO_COMMAND_NAME "HELLO"
#define PWM_COMMAND_NAME "PWM"
#define ADC_COMMAND_NAME "ADC"
#define STOP_COMMAND_NAME "STOP"
#define SET_COMMAND_NAME "SET"

static user_command_t _user_command;

#define MAX_BUFFER 32
static char _buffer[MAX_BUFFER];
static uint8_t _bufferSize = 0;

static int CountSpaces(char *p)
{
    int n = 0;
    while (p[n] == ' ')
        n++;
    return n;
}

static int ParseNameAfterSpaces(char *p, char *target, int target_size)
{
    int numSpaces = CountSpaces(p);
    if (numSpaces < 1)
        return 0;

    int n = 0;

    char *start = p + numSpaces;
    for (;;)
    {
        char ch = start[n];
        if (ch == ' ') break;
        if (n >= target_size) return -1;
        target[n] = ch;
        n += 1;
    }

    if (n == 0) return -1;
    target[n] = '\0';
    return n + numSpaces; 
}

static int ParseNumberAfterSpaces(char *p, int *pNumber)
{
    int numSpaces = CountSpaces(p);
    if (numSpaces < 1)
        return 0;

    int numDigits = 0;
    int n = 0;

    char *digits = p + numSpaces;
    for (;;)
    {
        char ch = digits[numDigits];
        if (ch < '0' || ch > '9')
            break;
        n = (n * 10) + (ch - '0');
        numDigits += 1;
    }

    if (numDigits == 0)
        return 0;

    *pNumber = n;
    return (numSpaces + numDigits);
}

static bool ParseSetCommand()
{
    int commandLength = strlen(SET_COMMAND_NAME);
    if(strncasecmp(SET_COMMAND_NAME, _buffer, commandLength) == 0) {
        
        char *inside = _buffer + commandLength;
        int length = ParseNameAfterSpaces(
            inside, _user_command.set_name, sizeof(_user_command.set_name));
        if (length < 1) return false;
         
        inside += length;
        length = ParseNumberAfterSpaces(inside, &_user_command.parameter_1);
        if (length < 1) return false;

        _user_command.command_id = SET_COMMAND_ID;
        return true;
    }

    return false;
}

static bool ParseCommand(char *commandName, int commandId, int numParams)
{
    int commandLength = strlen(commandName);
    if (strncasecmp(commandName, _buffer, commandLength) == 0)
    {
        _user_command.command_id = commandId;
        if (numParams == 0)
            return true;

        char *inside = _buffer + commandLength;
        int length = ParseNumberAfterSpaces(inside, &_user_command.parameter_1);
        if (length < 1)
            return false;
        if (numParams == 1)
            return true;

        inside += length;
        length = ParseNumberAfterSpaces(inside, &_user_command.parameter_2);
        if (length < 1)
            return false;
        if (numParams == 2)
            return true;

        inside += length;
        length = ParseNumberAfterSpaces(inside, &_user_command.parameter_3);
        if (length < 1)
            return false;
        if (numParams == 3)
            return true;
    }

    return false;
}

static bool ParseCommandInBuffer()
{
    // parse ADC command with one parameter
    if (ParseCommand(ADC_COMMAND_NAME, ADC_COMMAND_ID, 1))
        return true;

    // parse PWM command with two parameters
    if (ParseCommand(PWM_COMMAND_NAME, PWM_COMMAND_ID, 2))
        return true;

    // commands without parameters
    if (ParseCommand(STOP_COMMAND_NAME, STOP_COMMAND_ID, 0))
        return true;

    if (ParseCommand(HELLO_COMMAND_NAME, HELLO_COMMAND_ID, 0))
        return true;

    if (ParseSetCommand()) return true; 

    return false;
}

void command_parse_input_char(char ch)
{
    // ignore caret return
    if (ch == '\r' || ch == 0)
    {
        return;
    }

    // at the end of line
    if (ch == '\n')
    {

        // ugly hack, ignore empty lines, when new client connects we sometimes
        // get empty line which client didn't send and probing fails.
        if (_bufferSize == 0)
            return;

        _buffer[_bufferSize] = 0;

        // analyze buffer
        if (ParseCommandInBuffer())
        {
            mach_execute_command_and_respond(&_user_command);
        }
        else
        {
            command_respond_syntax_error(_buffer);
        }

        _bufferSize = 0;
        return;
    }

    if (_bufferSize == (MAX_BUFFER - 1))
    {
        // ignore data from too long an input
        return;
    }

    // ignore all 8-bit characters, strangely when new client connects
    // we sometimes get four 0xF0 chars and device probing fails.
    if (ch & (1 << 7))
    {
        return;
    }

    // store next char for later processing
    _buffer[_bufferSize] = ch;
    _bufferSize += 1;
}

bool command_respond_success(char *message)
{
    printf("OK: ");

    if (message != NULL)
        printf("%s", message);

    puts("");
    return true;
}

bool command_respond_syntax_error(char *text)
{
    return command_respond_user_error("Syntax error", text);
}

bool command_respond_user_error(char *message, char *param)
{
    printf("ERR: ");

    if (message != NULL)
        printf("%s", message);

    if (param != NULL)
    {
        printf(" [%s]", param);
    }

    puts("");
    return false;
}
