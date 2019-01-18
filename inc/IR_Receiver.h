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

#define CAPTURE_MAX_US 16777215 // (2^24-1)
#define TIME_PER_TICK 125 // 125E-10s
#define RESET_INDEX -1
#define MAXIMUM_SEQUENCE_TIME 1250000000 // 125ms
#define END_SEQUENCE_INDEX -2
#define END_SEQUENCE_TIME 200000000 // 20ms between edges
#define PWM_GAP 250000 // 25us between PWM edges
#define E_10S_TO_SEC_SCALAR 10000000000 // E-10 to seconds
#define E_10S_TO_US_SCALAR 10000 // E-10 to microseconds


typedef enum
{
    passthru,
    program
} Receiver_Mode;

void IR_Init_Receiver();
SignalInterval* getIRsequence(uint16_t* sequenceSize);
uint16_t getIRcarrierFrequency();

#endif /* INC_IR_RECEIVER_H_ */
