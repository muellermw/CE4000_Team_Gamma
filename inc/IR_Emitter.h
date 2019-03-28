/**
 * IR_Emitter.h
 *
 * Authors: Max Kallenberger, Marcus Mueller
 *
 * Emitter LED is on GPIO 9 (PIN 64) (which is where the PWM timer sends its signal)
 */

#ifndef INC_IR_EMITTER_H_
#define INC_IR_EMITTER_H_

#include "Signal_Interval.h"

// Infrared LED control function shortcuts
#define IR_LED_OFF() GPIO_write(Board_IR_OUTPUT_PIN, Board_GPIO_LED_OFF)
#define IR_LED_ON() GPIO_write(Board_IR_OUTPUT_PIN, Board_GPIO_LED_ON)

#define MAX_SEQUENCE_INDEX 128 // 7250

void IR_Init_Emitter();
void IRemitterSendButton(SignalInterval* button, uint16_t frequency);

#endif /* INC_IR_EMITTER_H_ */
