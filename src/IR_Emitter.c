/**
 * IR_Emitter.c
 *
 * This is the control mechanism for repeating IR commands
 *
 * Emitter LED is on GPIO 9 (PIN 64) (which is where the PWM timer sends its signal)
 */

#include <stdlib.h>
// GPIO Driver files
#include <ti/drivers/GPIO.h>
// PWM Driver files
#include <ti/drivers/PWM.h>
// Timer Driver files
#include <ti/drivers/Timer.h>
// Board Header file
#include "Board.h"
#include "IR_Emitter.h"

static PWM_Handle pwmHandle;
static PWM_Params pwmParams;
static Timer_Handle oneShotHandle;
static Timer_Params oneShotParams;
static SignalInterval* currentOutputSequence = 0;
static uint16_t currentOutputIndex = 0;

static void IRsetPWMperiod(uint32_t period);
static void IRinitOneShotTimer();
static void IRstartOneShotTimer();
static void IRinitPWMtimer();
static void IRstartPWMtimer();
static void IRstopPWMtimer();
static void IRsetOneShotTimeout(uint32_t time_in_us);

void IRoneShotTimerHandler(Timer_Handle handle);

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

/**
 * Start the send sequence needed to output a valid IR
 * command using a PWM output and a one-shot timer
 * @param button The SignalInterval that represents the IR signal to send
 * @param frequency The carrier frequency of the IR signal to send
 */
void IRemitterSendButton(SignalInterval* button, uint16_t frequency)
{
    currentOutputIndex = 0;
    currentOutputSequence = button;
    IRsetPWMperiod((uint32_t)frequency);
    // set a default timeout to start the IR sequence outside of any interrupt
    IRsetOneShotTimeout(50);
    IRstartOneShotTimer();
}

/**
 *  ======== IRoneShotTimerHandler ========
 *  Callback function for the one-shot timer signal sending interrupt
 *  Loops through and IR timing sequence to output an IR signal through the IR emitter
 */
void IRoneShotTimerHandler(Timer_Handle handle)
{
    // need to close the timer to set the delay to a different value
    Timer_close(oneShotHandle);

    // Check to make sure we have not reached the end of the output sequence buffer
    if ((currentOutputSequence[currentOutputIndex].time_us != 0) && (currentOutputIndex < MAX_SEQUENCE_INDEX))
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

        // Need to free the output sequence
        free(currentOutputSequence);
    }
}

/**
 * Sets up the PWM output for IR output - 50% duty cycle
 */
static void IRinitPWMtimer()
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
    pwmHandle = PWM_open(Board_PWM_IR_OUTPUT, &pwmParams);

    dutyValue = (uint32_t) (((uint64_t) PWM_DUTY_FRACTION_MAX * 50) / 100);
    PWM_setDuty(pwmHandle, dutyValue);  // set duty cycle to 50%
}

/**
 * Sets up the one-shot timer to loop through the timings of the IR signal
 */
static void IRinitOneShotTimer()
{
    Timer_init();

    Timer_Params_init(&oneShotParams);
    oneShotParams.periodUnits = Timer_PERIOD_US;
    oneShotParams.timerMode  = Timer_ONESHOT_CALLBACK;
    oneShotParams.timerCallback = IRoneShotTimerHandler;
}

/**
 * Starts the PWM timer output
 */
static void IRstartPWMtimer()
{
    PWM_start(pwmHandle);
}

/**
 * Stops the PWM timer output
 */
static void IRstopPWMtimer()
{
    PWM_stop(pwmHandle);
}

/**
 * Sets the period (IR carrier frequency) of the PWM output
 * @param period The period to set the PWM output to
 */
static void IRsetPWMperiod(uint32_t period)
{
    PWM_setPeriod(pwmHandle, period);
    uint32_t dutyValue = (uint32_t) (((uint64_t) PWM_DUTY_FRACTION_MAX * 50) / 100);
    PWM_setDuty(pwmHandle, dutyValue);  // set duty cycle to 50%
}

/**
 * Starts the one-shot timer
 */
static void IRstartOneShotTimer()
{
    Timer_start(oneShotHandle);
}

/**
 * Sets the one-shot timer timeout
 * @param time_in_us The timeout in microseconds
 */
static void IRsetOneShotTimeout(uint32_t time_in_us)
{
    oneShotParams.period = time_in_us;
    oneShotHandle = Timer_open(Board_EMITTER_TIMER, &oneShotParams);
}
