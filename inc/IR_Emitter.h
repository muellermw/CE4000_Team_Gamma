/**
 * IR_Emitter.c
 *
 * Authors: Max Kallenberger, Marcus Mueller
 *
 * Emitter is PIN 53. Need to change in CC3220SF_LaunchXL.c if you want to update.
 */

#ifndef INC_IR_EMITTER_H_
#define INC_IR_EMITTER_H_

// Infrared LED control function shortcuts
#define IR_LED_OFF() GPIO_write(Board_IR_OUTPUT_PIN, Board_GPIO_LED_OFF)
#define IR_LED_ON() GPIO_write(Board_IR_OUTPUT_PIN, Board_GPIO_LED_ON)

void IR_Init_Emitter();
void IRsetPWMperiod(uint32_t period);
void IRstartOneShotTimer();
void IRsetOneShotTimeout(uint32_t time_in_us);

#endif /* INC_IR_EMITTER_H_ */
