/**
 * IR_Control.h
 */

#ifndef INC_IR_CONTROL_H_
#define INC_IR_CONTROL_H_

#define IR_OUTPUT_PIN Board_IR_OUTPUT_P53

// Infrared LED control function shortcuts
#define IR_LED_OFF() GPIO_write(IR_OUTPUT_PIN, Board_GPIO_LED_OFF)
#define IR_LED_ON() GPIO_write(IR_OUTPUT_PIN, Board_GPIO_LED_ON)

void IR_init();

#endif /* INC_IR_CONTROL_H_ */
