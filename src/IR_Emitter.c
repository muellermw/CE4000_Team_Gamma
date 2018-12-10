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
// Board Header file
#include "Board.h"
#include "IR_Emitter.h"
#include <stdlib.h>

void IRinitPWMtimer();
void IRstartPWMtimer();
void IRstopPWMtimer();

static PWM_Handle pwmHandle;
static PWM_Params pwmParams;

/**
 * Initialize PWM and one-shot timers to recreate stored IR signals
 */
void IR_Init_Emitter()
{
    IRinitPWMtimer();
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
