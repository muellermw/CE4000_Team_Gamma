/**
 * IR_Receiver.c
 *
 * Authors: Max Kallenberger, Marcus Mueller
 *
 * Receiver is PIN 50 for the capture timer, PIN 64 for the passthrough GPIO.
 * Need to change pins in CC3220SF_LaunchXL.c if you want to update.
 */

#include <ti/drivers/Capture.h>
#include <ti/drivers/GPIO.h>
#include <stdlib.h>

#include "Board.h"
#include "IR_Receiver.h"
#include "IR_Emitter.h"
#include "forwardLinkedList.h"
#include "Signal_Edge.h"

void IRinitSignalCapture();
void IRstartSignalCapture();
void IRstopSignalCapture();
void IRinitEdgeDetectGPIO();
void IRstartEdgeDetectGPIO();
void IRstopEdgeDetectGPIO();
void IRreceiverSwitchMode();
void IRedgeDetectionPassthrough(uint_least8_t index);
void IRedgeProgramButton(Capture_Handle handle, uint32_t interval);

Receiver_Mode receiverState;
static struct linkedList* test;
static Capture_Handle captureHandle;
static Capture_Params captureParams;
volatile static SignalEdge sequence[MAX_INDEX];
static uint32_t index = 0;
static uint32_t totTime = 0;

/**
 * Initial setup of the IR receiver and its peripherals
 */
void IR_Init_Receiver()
{
    test = malloc(sizeof(struct linkedList));
    fll_init(test);

    receiverState = program;
    IRinitSignalCapture();
    IRinitEdgeDetectGPIO();
    //IRstartEdgeDetectGPIO();

    IRstartSignalCapture();
}

/**
 *  ======== IRedgeDetectionPassthrough ========
 *  Callback function for the capture timer edge interrupt
 */
void IRedgeDetectionPassthrough(uint_least8_t index)
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
    SignalEdge temp;
    if(index == 0){
        temp.time_us = 0;
    }
    else{
        temp.time_us = interval;
    }

    if(!(index & 0b1)){
        temp.PWM = true;
    }
    else{
        temp.PWM = false;
    }

    if(index < MAX_INDEX){
        sequence[index] = temp;
        index++;
    }
    else{
        IRstopSignalCapture();
        index = 0;
        totTime = 0;
    }

    totTime += temp.time_us;
    if(totTime >= 125000){
        IRstopSignalCapture();
        index = 0;
        totTime = 0;
    }

}

/**
 * Initialize the input capture timer (should only be called once)
 */
void IRinitSignalCapture()
{
    // Set up input capture clock on pin 50 [subject to change]
    Capture_init();
    // Set up capture timer parameters
    Capture_Params_init(&captureParams);
    captureParams.mode  = Capture_ANY_EDGE;
    captureParams.callbackFxn = IRedgeProgramButton;
    captureParams.periodUnit = Capture_PERIOD_US;
    captureHandle = Capture_open(Board_CAPTURE0, &captureParams);
    // Make sure this capture timer is not running yet
    Capture_stop(captureHandle);
}

/**
 * Initialize the edge detect GPIO interrupt passthrough (should only be called once)
 */
void IRinitEdgeDetectGPIO()
{
    GPIO_setConfig(Board_IR_EDGE_DETECT_PIN, GPIO_CFG_IN_NOPULL | GPIO_CFG_IN_INT_BOTH_EDGES);
    GPIO_setCallback(Board_IR_EDGE_DETECT_PIN, IRedgeDetectionPassthrough);
    // Force the interrupt to be disabled until IRstartEdgeDetectGPIO is called
    GPIO_disableInt(Board_IR_EDGE_DETECT_PIN);
}

/**
 * Enable the passthrough IR GPIO interrupt to start directly recreating any signals detected
 */
void IRstartEdgeDetectGPIO()
{
    GPIO_enableInt(Board_IR_EDGE_DETECT_PIN);
}

/**
 * Disable the passthrough interrupt
 */
void IRstopEdgeDetectGPIO()
{
    GPIO_disableInt(Board_IR_EDGE_DETECT_PIN);
}

/**
 * Start the input capture timer interrupt to learn IR codes
 */
void IRstartSignalCapture()
{
    Capture_start(captureHandle);
}

/**
 * Stops the input capture timer and closes the capture driver instance so
 * the callback function for the timer interrupt can be reassigned
 */
void IRstopSignalCapture()
{
    Capture_stop(captureHandle);
}

/**
 * Switch the receiver mode based on what state we are in
 */
void IRreceiverSwitchMode()
{
    switch (receiverState)
    {
    case passthru:
        IRstopSignalCapture();
        IRstartEdgeDetectGPIO();
        break;
    case program:
        IRstopEdgeDetectGPIO();
        IRstartSignalCapture();
        break;
    }
}
