/*
 * Control_States.h
 *
 *  Created on: Jan 17, 2019
 *      Author: Max Kallenberger, Marcus Mueller
 */

#ifndef INC_CONTROL_STATES_H_
#define INC_CONTROL_STATES_H_

#define IDLE_STR          "idle"
#define APP_INIT_STR      "app_init"
#define ADD_BUTTON_STR    "add_button"
#define DELETE_BUTTON_STR "delete_button"
#define SEND_BUTTON_STR   "send_button"
#define CLEAR_BUTTONS_STR "clear_all"

typedef enum
{
    idle,
    app_init,
    add_button,
    delete_button,
    send_button
} ControlState;

#endif /* INC_CONTROL_STATES_H_ */
