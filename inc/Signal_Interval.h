/**
 * Signal_Interval.h
 *
 * Represents an IR signal pulse in microseconds
 *
 * Created on: Dec 11, 2018
 *     Author: Max Kallenberger
 */

#ifndef INC_SIGNAL_INTERVAL_H_
#define INC_SIGNAL_INTERVAL_H_

#include <stdbool.h>

typedef struct {
    uint32_t time_us;
    bool PWM;
} SignalInterval;

#endif /* INC_SIGNAL_INTERVAL_H_ */
