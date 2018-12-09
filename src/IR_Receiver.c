/*
 * IR_Receiver.c
 *
 *  Created on: Dec 8, 2018
 *      Author: Max Kallenberger
 *
 *      Receiver is PIN 50. Need to change in CC3220SF_LaunchXL.c if you want to update.
 */

#include <ti/drivers/Capture.h>
#include <ti/drivers/GPIO.h>
#include <stdlib.h>

#include "Board.h"
#include "IR_Receiver.h"
#include "IR_Emitter.h"
#include "forwardLinkedList.h"

void IRedgeDetectionPassthrough(uint_least8_t index);
void IRedgeProgramButton(Capture_Handle handle, uint32_t interval);

Receiver_Mode mode = program;
static struct linkedList* test;
static Capture_Handle handle;
static Capture_Params params;

void IR_Init_Receiver()
{
    test = malloc(sizeof(struct linkedList));
    fll_init(test);

    GPIO_setConfig(Board_EDGE_DETECT_P58, GPIO_CFG_IN_NOPULL | GPIO_CFG_IN_INT_BOTH_EDGES);
    if(mode == program){
        // Set up input capture clock on pin 50
        Capture_init();
        Capture_Params_init(&params);
        params.mode  = Capture_ANY_EDGE;
        params.callbackFxn = IRedgeProgramButton;
        params.periodUnit = Capture_PERIOD_COUNTS;
        handle = Capture_open(Board_CAPTURE0, &params);

        // For error checking the input capture timer IF NEEDED
        /* if (handle == NULL) {
            //Capture_open() failed
            //while(1);
        }*/

        int status = Capture_start(handle);

        // For error checking the input capture timer IF NEEDED
        /*if (status == Capture_STATUS_ERROR) {
            //Capture_start() failed
            //while(1);
        }*/
    }
    else if(mode == passthru){
        GPIO_setCallback(Board_EDGE_DETECT_P58, IRedgeDetectionPassthrough);
        GPIO_enableInt(Board_EDGE_DETECT_P58);
    }
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

void IRedgeProgramButton(Capture_Handle handle, uint32_t interval)
{
    IR_LED_OFF();
}

