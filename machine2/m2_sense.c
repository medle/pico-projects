
#include "m2_globals.h"

static bool _isRunning = false;
static uint _maxWaves = 2;
static uint _waveCount = 0;

static volatile bool _isMeasuring = false;
static uint _measureCount = 0;
#define MAX_MEASURES 10
static uint64_t _measures[MAX_MEASURES];

static uint computeMeasuredWaveHz();

static void senseCallbackRoutine(uint gpio, uint32_t events)
{
    if (++_waveCount >= _maxWaves) {
        machPwmResetCounter();
        machAdcHandlePeriodEnd();
        _waveCount = 0;
    }

    if (_isMeasuring) {
        _measures[_measureCount] = time_us_64();
        if (++_measureCount == MAX_MEASURES) _isMeasuring = false;
    }
} 

void machSenseEnable(bool enable)
{
    if (_isRunning == enable) return;

    _isMeasuring = true;
    _waveCount = 0;

    // Schedule a sense routine call on each falling sense signal edge - the
    // moment when LC-tank current goes back to a full stop during oscillating.
    uint flags = /*GPIO_IRQ_EDGE_RISE |*/ GPIO_IRQ_EDGE_FALL;
    gpio_set_irq_enabled_with_callback(
        MACH_SENSE_GPIO, flags, enable, senseCallbackRoutine);    
}

static uint computeMeasuredWaveHz()
{
    if (_measureCount <= 1) return 0;
    
    uint maxUs = 0;
    for (int i = 1; i < _measureCount; i++) {
        uint us = (uint)(_measures[i] - _measures[i - 1]);
        if (us > maxUs) maxUs = us;
    }

    return maxUs > 0 ? 1000000 / maxUs : 0;
}
