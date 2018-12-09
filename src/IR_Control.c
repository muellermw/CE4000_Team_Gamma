/**
 * IR_Control.c
 */

// Driver Header files
#include <ti/drivers/GPIO.h>
// Board Header file
#include "Board.h"
#include "IR_Control.h"
#include <stdlib.h>
void IRedgeDetectionPassthrough(uint_least8_t index);


/**
 * Initialize IR GPIO and set callback functions to initially passthrough all detected IR signals
 */
void IR_init()
{
    GPIO_setConfig(Board_EDGE_DETECT_P58, GPIO_CFG_IN_NOPULL | GPIO_CFG_IN_INT_BOTH_EDGES);
    IR_LED_OFF();
    GPIO_setCallback(Board_EDGE_DETECT_P58, IRedgeDetectionPassthrough);
    GPIO_enableInt(Board_EDGE_DETECT_P58);
}

/**
 *  ======== IRedgeDetector ========
 *  Callback function for the GPIO edge interrupt on GPIOCC32XX_GPIO_30.
 */
void IRedgeDetectionPassthrough(uint_least8_t index)
{
    // Clear the GPIO interrupt and set the IR out to match the interrupt edge
    if (GPIO_read(Board_EDGE_DETECT_P58))
    {
        IR_LED_OFF();
    }
    else
    {
        IR_LED_ON();
    }
}

