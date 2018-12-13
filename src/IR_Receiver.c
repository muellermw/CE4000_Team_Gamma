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
#include <Signal_Interval.h>
#include <stdbool.h>

#include "Board.h"
#include "IR_Receiver.h"
#include "IR_Emitter.h"
#include "forwardLinkedList.h"

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
static SignalInterval sequence[MAX_INDEX];
static SignalInterval currentInt;
static uint16_t edgeCnt = 0;
static uint16_t frequency = 0;
static uint32_t seqIndex = 0;
static uint32_t totalCaptureTime = 0;
static bool buttonCaptured = false;

/**
 * Initial setup of the IR receiver and its peripherals
 */
void IR_Init_Receiver()
{
    test = malloc(sizeof(struct linkedList));
    fll_init(test);

    receiverState = program;
    // make sure the sequence array is initialized to zero
    memset(&sequence[0], 0, sizeof(sequence));
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
    if(seqIndex == 0){
        seqIndex++;
        currentInt.time_us = 0;
        currentInt.PWM = true;
    }
    else if((totalCaptureTime >= 125000) || (seqIndex >= MAX_INDEX)){
        IRstopSignalCapture();
        seqIndex = 0;
        totalCaptureTime = 0;
        edgeCnt = 0;
        buttonCaptured = true;
    }
    else{
        if(frequency == 0){
            edgeCnt++;
        }
        totalCaptureTime += interval;
        if(interval <= 25){
            currentInt.time_us += interval;
        }
        else{
            if(frequency == 0){
                edgeCnt--;
                uint32_t period_us = (currentInt.time_us*2);
                frequency = (1000000*edgeCnt)/period_us;
            }
            sequence[seqIndex-1] = currentInt;
            seqIndex++;
            currentInt.time_us = interval;
            currentInt.PWM = false;
            sequence[seqIndex-1] = currentInt;
            seqIndex++;
            currentInt.time_us = 0;
            currentInt.PWM = true;
        }
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

SignalInterval* getIRsequence()
{
    return &sequence[0];
}

uint16_t getIRcarrierFrequency()
{
    return frequency;
}

bool IRbuttonReady()
{
    return buttonCaptured;
}
