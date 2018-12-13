/**
 * IR_Emitter.c
 *
 * Emitter LED is on PIN 1 (which is where the PWM timer sends its signal)
 * Need to change pins in CC3220SF_LaunchXL.c if you want to update.
 */

// GPIO Driver files
#include <ti/drivers/GPIO.h>
// PWM Driver files
#include <ti/drivers/PWM.h>
// Timer Driver files
#include <ti/drivers/Timer.h>
// Board Header file
#include "Board.h"
#include "IR_Emitter.h"
#include <stdlib.h>

void IRinitPWMtimer();
void IRstartPWMtimer();
void IRstopPWMtimer();
void IRinitOneShotTimer();
void IRoneShotTimerHandler(Timer_Handle handle);

static PWM_Handle pwmHandle;
static PWM_Params pwmParams;
static Timer_Handle oneShotHandle;
static Timer_Params oneShotParams;
static SignalInterval* currentOutputSequence = 0;
static uint16_t currentOutputIndex = 0;
static bool sendingButton = false;


/**
 * Initialize PWM and one-shot timers to recreate stored IR signals
 */
void IR_Init_Emitter()
{
    IRinitPWMtimer();
    IRinitOneShotTimer();
    // Set the IR LED off just to be sure it wasn't left on
    IR_LED_OFF();
}

void IRinitPWMtimer()
{
    uint32_t   dutyValue;
    // Initialize the PWM driver.
    PWM_init();
    // Initialize the PWM parameters
    PWM_Params_init(&pwmParams);
    pwmParams.idleLevel = PWM_IDLE_LOW;      // Output low when PWM is not running
    pwmParams.periodUnits = PWM_PERIOD_HZ;   // Period is in Hz
    pwmParams.periodValue = 38000;           // Default to 38kHz (typical consumer IR frequency)
    pwmParams.dutyUnits = PWM_DUTY_FRACTION; // Duty is in fractional percentage
    pwmParams.dutyValue = 0;                 // 0% initial duty cycle
    // Open the PWM instance
    pwmHandle = PWM_open(Board_PWM0, &pwmParams);

    dutyValue = (uint32_t) (((uint64_t) PWM_DUTY_FRACTION_MAX * 50) / 100);
    PWM_setDuty(pwmHandle, dutyValue);  // set duty cycle to 50%
}

void IRinitOneShotTimer()
{
    Timer_init();

    Timer_Params_init(&oneShotParams);
    oneShotParams.periodUnits = Timer_PERIOD_US;
    oneShotParams.timerMode  = Timer_ONESHOT_CALLBACK;
    oneShotParams.timerCallback = IRoneShotTimerHandler;
}

/**
 *  ======== IRoneShotTimerHandler ========
 *  Callback function for the one-shot timer signal sending interrupt
 */
void IRoneShotTimerHandler(Timer_Handle handle)
{
    // need to close the timer to set the delay to a different value
    Timer_close(oneShotHandle);

    if (currentOutputSequence[currentOutputIndex].time_us != 0)
    {
        IRsetOneShotTimeout(currentOutputSequence[currentOutputIndex].time_us);

        if (currentOutputSequence[currentOutputIndex].PWM == true)
        {
            IRstartPWMtimer();
        }
        else
        {
            IRstopPWMtimer();
        }

        currentOutputIndex++;
        IRstartOneShotTimer();

    }
    else
    {
        IRstopPWMtimer();
        sendingButton = false;
    }
}

void IRstartPWMtimer()
{
    PWM_start(pwmHandle);
}

void IRstopPWMtimer()
{
    PWM_stop(pwmHandle);
}

void IRsetPWMperiod(uint32_t period)
{
    PWM_setPeriod(pwmHandle, period);
    uint32_t dutyValue = (uint32_t) (((uint64_t) PWM_DUTY_FRACTION_MAX * 50) / 100);
    PWM_setDuty(pwmHandle, dutyValue);  // set duty cycle to 50%
}

void IRsetOneShotTimeout(uint32_t time_in_us)
{
    oneShotParams.period = time_in_us;
    oneShotHandle = Timer_open(Board_TIMER0, &oneShotParams);
}

void IRstartOneShotTimer()
{
    Timer_start(oneShotHandle);
}

void IRemitterSendButton(SignalInterval* button, uint16_t frequency)
{
    sendingButton = true;
    currentOutputIndex = 0;
    currentOutputSequence = button;
    IRsetPWMperiod((uint32_t)frequency);
    // set a default timeout to start the IR sequence outside of any interrupt
    IRsetOneShotTimeout(50);
    IRstartOneShotTimer();
}

bool IRbuttonSending()
{
    return sendingButton;
}
