/*
 * IR_Receiver.h
 *
 *  Created on: Dec 8, 2018
 *      Author: Max Kallenberger
 */

#ifndef INC_IR_RECEIVER_H_
#define INC_IR_RECEIVER_H_

#define CAPTURE_MAX_US 16777215 //(2^24-1)
#define MAX_INDEX 128 //7250

typedef enum
{
    passthru,
    program
} Receiver_Mode;

void IR_Init_Receiver();

#endif /* INC_IR_RECEIVER_H_ */
