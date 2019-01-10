/**
 * IR_Receiver.h
 *
 * Author: Max Kallenberger
 *
 * Receiver is GPIO 15 (PIN 6) for the capture timer, GPIO 14 (PIN 5) for the passthrough interrupt.
 */

#ifndef INC_IR_RECEIVER_H_
#define INC_IR_RECEIVER_H_

#include "IR_Emitter.h"

#define CAPTURE_MAX_US 16777215 //(2^24-1)
#define MAX_SEQUENCE_INDEX 128 //7250

typedef enum
{
    passthru,
    program
} Receiver_Mode;

void IR_Init_Receiver();
SignalInterval* getIRsequence();
uint16_t getIRcarrierFrequency();

#endif /* INC_IR_RECEIVER_H_ */
