/**
 * This header file represents the control of 32-bit Timer A0
 * @file Misc_Timer.h
 * @date 3/10/2019
 * @author: Marcus Mueller
 */

#ifndef INC_MISC_TIMER_H_
#define INC_MISC_TIMER_H_

// Timer Driver files
#include <ti/drivers/Timer.h>

void initMiscOneShotTimer();
void setMiscOneShotTimeout(uint32_t time_in_us);
void startMiscOneShotTimer();
void stopMiscOneShotTimer();
void setMiscOneShotTimerCallback(Timer_CallBackFxn func);


#endif /* INC_MISC_TIMER_H_ */
