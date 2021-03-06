/**
 * IR_Receiver.c
 *
 * This is the control mechanism for repeating and learning IR commands
 *
 * Receiver is GPIO 15 (PIN 6) for the capture timer, GPIO 14 (PIN 5) for the passthrough interrupt.
 */

#include <ti/drivers/Capture.h>
#include <ti/drivers/GPIO.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <stdbool.h>

#include "Board.h"
#include "Signal_Interval.h"
#include "IR_Emitter.h"
#include "IR_Receiver.h"

Receiver_Mode receiverState;
static Capture_Handle captureHandle;
static Capture_Params captureParams;
static SignalInterval irSequence[MAX_SEQUENCE_INDEX];
static SignalInterval currentInt;
static uint16_t edgeCnt = 0;
static uint16_t frequency = 0;
static uint16_t irSequenceSize = 0;
static int32_t seqIndex = -1;
static uint32_t totalCaptureTime = 0;
static bool irGapDetected = false;
static bool buttonCaptured = false;

static void IRinitSignalCapture();
static void IRstartSignalCapture();
static void IRstopSignalCapture();
static void IRinitEdgeDetectGPIO();
static void ConvertToUs(SignalInterval *seq, uint32_t length);

void IRedgeDetectionPassthrough(uint_least8_t index);
void IRedgeProgramButton(Capture_Handle handle, uint32_t interval);

/**
 * Initial setup of the IR receiver and its peripherals
 */
void IR_Init_Receiver()
{
    receiverState = passthru;

    // make sure the sequence array is initialized to zero
    memset(&irSequence[0], 0, sizeof(irSequence));
    IRinitSignalCapture();
    IRinitEdgeDetectGPIO();
    IRstartEdgeDetectGPIO();
}

/**
 *  ======== IRedgeDetectionPassthrough ========
 *  Callback function for the GPIO edge interrupt
 *
 *  This function reads the edge input edge from the IR receiver and duplicates the response
 *  on the IR Emitter GPIO pin.
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
 *
 *  This function is used the record an IR signal sent from a remote and
 *  store it into the pre-allocated array, sequence. It detects the carry frequency
 *  and records times corresponding to pulses of PWM input and silence. This implementation
 *  assumes a maximum signal length of 125ms. It also assumes no signal will have a
 *  pulse of silence greater than 20ms. Times are recorded in 1E-10 seconds and
 *  later converted to microseconds. The maximum period between edges during
 *  PWM pulses is assumed to be less than 25us.
 *
 *  @param interval     This is the period between edges in clock ticks
 */
void IRedgeProgramButton(Capture_Handle handle, uint32_t interval)
{
    // Interval is in clock ticks, and each tick is 125E-10s
    interval = interval * TIME_PER_TICK;

    // Need to ignore the first edge detected because the interval
    // does not hold relevant information.
    if(seqIndex == RESET_INDEX){
        // Update index and ensure variables are prepared to
        // record data.
        seqIndex++;
        currentInt.time_us = 0;
        currentInt.PWM = true;
    }

    // If the signal has exceeded the time limit, the end of the array has been reached, or
    // a sufficiently long silent pulse has been found, stop recording the signal.
    else if((totalCaptureTime >= MAXIMUM_SEQUENCE_TIME) || (seqIndex >= MAX_SEQUENCE_INDEX) || (irGapDetected == true)){
        IRstopSignalCapture();

        // Update the sequence size so we know how large the IR sequence buffer is when storing
        irSequenceSize = seqIndex;

        // Since the final index recorded is guaranteed to be a silence,
        // update the time to 0us to prevent the IR Emitter from outputting it
        // unnecessarily.
        seqIndex--;
        (irSequence[seqIndex]).time_us = 0;

        // Convert the 1E-10s that were recorded to microseconds
        ConvertToUs(irSequence, seqIndex);

        // Reset variables for next capture
        seqIndex = RESET_INDEX;
        totalCaptureTime = 0;
        edgeCnt = 0;
        irGapDetected = false;
        buttonCaptured = true;
    }

    // Either need add accumulated time or record the data
    else{
        // If a frequency has not been calculated, count the number of edges detected
        if(frequency == 0){
            edgeCnt++;
        }

        // Record the total capture time to ensure maximum signal length is not exceeded
        totalCaptureTime += interval;

        // If the time between edges is less than the maximum PWM half period
        // add the time to calculate the PWM pulse length
        if(interval <= PWM_GAP){
            currentInt.time_us += interval;
        }

        // Otherwise, a silent pulse has been detected, and both pulses
        // must be recorded
        else{
            // Calculate the average frequency from the first PWM pulse
            if(frequency == 0){
                edgeCnt--;

                // Rather than divide edges by 2 to get # of periods
                // multiply time by 2 for efficiency
                uint32_t period_us = (currentInt.time_us*2);
                frequency = (E_10S_TO_SEC_SCALAR*edgeCnt)/period_us;
            }
            // Record the PWM pulse
            irSequence[seqIndex] = currentInt;
            // Record the silent pulse
            seqIndex++;
            currentInt.time_us = interval;
            currentInt.PWM = false;
            irSequence[seqIndex] = currentInt;

            // Reset variables to receiver more pulses
            seqIndex++;
            currentInt.time_us = 0;
            currentInt.PWM = true;

            // If the silent pulse was longer than the maximum allowed gap,
            // assume the sequence ended, and a duplicate signal is next
            if(interval >= END_SEQUENCE_TIME){
                irGapDetected = true;
            }
        }
    }
}

/**
 * Switch the receiver mode based on what state we are in
 */
void IRreceiverSetMode(Receiver_Mode mode)
{
    switch (mode)
    {
    case passthru:
        IRstopSignalCapture();
        IRstartEdgeDetectGPIO();
        receiverState = passthru;
        break;
    case program:
        IRstopEdgeDetectGPIO();
        IRstartSignalCapture();
        receiverState = program;
        break;
    }
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
 * Get the IR sequence in addition to its size
 * @param  sequenceSize the pointer to an integer where the sequence size will be stored
 *         NOTE: sequence size is in BYTES
 * @return the pointer to the IR sequence buffer
 */
SignalInterval* getIRsequence(uint16_t* sequenceSize)
{
    *sequenceSize = (uint16_t)(irSequenceSize*sizeof(SignalInterval));
    return &irSequence[0];
}

/**
 * Gets the carrier frequency of the last captured IR signal
 * @return The IR carrier frequency in Hz
 */
uint16_t getIRcarrierFrequency()
{
    uint16_t RetVal = frequency;
    frequency = 0;
    return RetVal;
}

/**
 * Report if a button has been captures and is ready to be stored
 * @return True it a button is ready, false if not
 */
bool IRbuttonReady()
{
    bool RetVal = buttonCaptured;
    if (RetVal == true)
    {
        // Reset the button captured flag
        buttonCaptured = false;
    }

    return RetVal;
}

/**
 * Start the input capture timer interrupt to learn IR codes
 */
static void IRstartSignalCapture()
{
    Capture_start(captureHandle);
}

/**
 * Stops the input capture timer and closes the capture driver instance so
 * the callback function for the timer interrupt can be reassigned
 */
static void IRstopSignalCapture()
{
    Capture_stop(captureHandle);
}

/**
 * Initialize the input capture timer (should only be called once)
 */
static void IRinitSignalCapture()
{
    // Set up input capture clock on pin 50 [subject to change]
    Capture_init();
    // Set up capture timer parameters
    Capture_Params_init(&captureParams);
    captureParams.mode  = Capture_ANY_EDGE;
    captureParams.callbackFxn = IRedgeProgramButton;
    captureParams.periodUnit = Capture_PERIOD_COUNTS;
    captureHandle = Capture_open(Board_CAPTURE0, &captureParams);
    // Make sure this capture timer is not running yet
    Capture_stop(captureHandle);
}

/**
 * Initialize the edge detect GPIO interrupt passthrough (should only be called once)
 */
static void IRinitEdgeDetectGPIO()
{
    GPIO_setConfig(Board_IR_EDGE_DETECT_PIN, GPIO_CFG_IN_NOPULL | GPIO_CFG_IN_INT_BOTH_EDGES);
    GPIO_setCallback(Board_IR_EDGE_DETECT_PIN, IRedgeDetectionPassthrough);
    // Force the interrupt to be disabled until IRstartEdgeDetectGPIO is called
    GPIO_disableInt(Board_IR_EDGE_DETECT_PIN);
}

/**
 * This function converts an IR sequence recorded in 1E-10s to micr5osecond times
 *
 * @param seq This is the array of pulse times.
 * @param length This is the amount of pulses in the array.
 */
static void ConvertToUs(SignalInterval *seq, uint32_t length){
    // Check to length to prevent index out of bounds
    if(length >= MAX_SEQUENCE_INDEX){
        length = MAX_SEQUENCE_INDEX;
    }

    // Convert 1E-10s to microseconds
    for(int i = 0; i < length; i++){
        (seq[i]).time_us = ((seq[i]).time_us / E_10S_TO_US_SCALAR);
    }
}
