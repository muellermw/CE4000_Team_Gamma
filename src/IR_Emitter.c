/**
 * IR_Emitter.c
 *
 *
 */

// Driver Header files
#include <ti/drivers/GPIO.h>
// Board Header file
#include "Board.h"
#include "IR_Emitter.h"
#include <stdlib.h>

/**
 * Initialize IR GPIO and set callback functions to initially passthrough all detected IR signals
 */
void IR_Init_Emitter()
{
    IR_LED_OFF();
}


