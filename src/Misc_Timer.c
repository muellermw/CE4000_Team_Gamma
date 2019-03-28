/**
 * This file represents the control of 32-bit Timer A0 for
 * miscellaneous one-shot timer usage
 * @file Misc_Timer.c
 * @date 3/10/2019
 * @author: Marcus Mueller
 */

#include <stddef.h>
// Board Header file
#include "Board.h"
#include "Misc_Timer.h"

static Timer_Handle oneShotHandle;
static Timer_Params oneShotParams;
static int hasBeenInitialized = 0;


/**
 * Initialize the timer using TI's timer API
 */
void initMiscOneShotTimer()
{
    Timer_init();

    Timer_Params_init(&oneShotParams);
    oneShotParams.periodUnits = Timer_PERIOD_US;
    oneShotParams.timerMode  = Timer_ONESHOT_CALLBACK;
    oneShotParams.timerCallback = NULL;

    // Set init flag for method safety (start and stop methods)
    hasBeenInitialized = 1;
}

/**
 * Set the timeout time for the miscellaneous timer
 *
 * Note: due to the nature of the 32-bit wide timer, with no
 *       prescaler, we are limited to a one-shot delay of 53.5 seconds.
 *       The max value that can be used is 53,687,090 (2^32/80(MHz))
 * @param time_in_us The timeout time for the timer
 */
void setMiscOneShotTimeout(uint32_t time_in_us)
{
    if (oneShotParams.timerCallback != NULL)
    {
        oneShotParams.period = time_in_us;
        oneShotHandle = Timer_open(Board_MISC_TIMER, &oneShotParams);
    }
}

/**
 * Start the one-shot timer if it has been initialized and the handle is valid
 */
void startMiscOneShotTimer()
{
    if (hasBeenInitialized == 1)
    {
        Timer_start(oneShotHandle);
    }
}

/**
 * Stop the one-shot timer
 */
void stopMiscOneShotTimer()
{
    if (hasBeenInitialized == 1)
    {
        Timer_stop(oneShotHandle);
    }
}

/**
 * Set the callback function for the timer timeout
 * @param func The Timer_CallBackFxn pointer
 */
void setMiscOneShotTimerCallback(Timer_CallBackFxn func)
{
    oneShotParams.timerCallback = func;
}

