/*
 * IR_Receiver.c
 *
 * Authors: Max Kallenberger, Marcus Mueller
 *
 * Receiver is PIN 50. Need to change in CC3220SF_LaunchXL.c if you want to update.
 */

#include <ti/drivers/Capture.h>
#include <ti/drivers/GPIO.h>
#include <stdlib.h>

#include "Board.h"
#include "IR_Receiver.h"
#include "IR_Emitter.h"
#include "forwardLinkedList.h"

void IRstartSignalCapture();
void IRstopSignalCapture();
void IRresetSignalCapture();
void IRedgeDetectionPassthrough(Capture_Handle handle, uint32_t interval);
void IRedgeProgramButton(Capture_Handle handle, uint32_t interval);

Receiver_Mode receiverState;
static struct linkedList* test;
static Capture_Handle captureHandle;
static Capture_Params captureParams;

/**
 * Initial setup of the IR receiver and its peripherals
 */
void IR_Init_Receiver()
{
    test = malloc(sizeof(struct linkedList));
    fll_init(test);

    // Set up input capture clock on pin 50 [subject to change]
    Capture_init();
    // Set up capture timer parameters
    Capture_Params_init(&captureParams);
    captureParams.mode  = Capture_ANY_EDGE;
    // The callback function will be set by the IR start mode functions
    captureParams.callbackFxn = NULL;
    captureParams.periodUnit = Capture_PERIOD_COUNTS;

    receiverState = passthru;
    IRstartSignalCapture();
}

/**
 *  ======== IRedgeDetectionPassthrough ========
 *  Callback function for the capture timer edge interrupt
 */
void IRedgeDetectionPassthrough(Capture_Handle handle, uint32_t interval)
{
    // Clear the GPIO interrupt and set the IR out to match the interrupt edge
    if (GPIO_read(Board_IR_EDGE_DETECT_PIN))
    {
        IR_LED_OFF();
    }
    else
    {
        IR_LED_ON();
    }
}

/**
 *  ======== IRedgeProgramButton ========
 *  Callback function for the capture timer learning interrupt
 */
void IRedgeProgramButton(Capture_Handle handle, uint32_t interval)
{
    IR_LED_OFF();
}

/**
 * Start the input capture timer based on the current state the IR receiver is in
 */
void IRstartSignalCapture()
{
    // Set the receiver mode based on the current state
    switch (receiverState)
    {
    case program:
        captureParams.callbackFxn = IRedgeProgramButton;
        break;

    default:
        captureParams.callbackFxn = IRedgeDetectionPassthrough;
    }

    captureHandle = Capture_open(Board_CAPTURE0, &captureParams);

    // For error checking the input capture timer IF NEEDED
    /* if (handle == NULL) {
        //Capture_open() failed
        //while(1);
    }*/

    int status = Capture_start(captureHandle);

    // For error checking the input capture timer IF NEEDED
    /*if (status == Capture_STATUS_ERROR) {
        //Capture_start() failed
        //while(1);
    }*/
}

/**
 * Stops the input capture timer and closes the capture driver instance so
 * the callback function for the timer interrupt can be reassigned
 */
void IRstopSignalCapture()
{
    // Capture_stop is not necessary, but eliminates any chance of errors from firing interrupts
    Capture_stop(captureHandle);
    Capture_close(captureHandle);
}

/**
 * This is the procedure to reset the input capture timer when the receiver state is changed
 */
void IRresetSignalCapture()
{
    IRstopSignalCapture();
    IRstartSignalCapture();
}
