/*
 * IR_Receiver.h
 *
 *  Created on: Dec 8, 2018
 *      Author: Max Kallenberger
 */

#ifndef INC_IR_RECEIVER_H_
#define INC_IR_RECEIVER_H_

typedef enum
{
    passthru,
    program
} Receiver_Mode;

void IR_Init_Receiver();

#endif /* INC_IR_RECEIVER_H_ */
