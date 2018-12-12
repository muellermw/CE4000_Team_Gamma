/*
 * Signal_Edge.h
 *
 *  Created on: Dec 11, 2018
 *      Author: Max Kallenberger
 */

#ifndef INC_SIGNAL_EDGE_H_
#define INC_SIGNAL_EDGE_H_

#include <stdbool.h>

typedef struct {
    uint32_t time_us;
    bool PWM;
}SignalEdge;

#endif /* INC_SIGNAL_EDGE_H_ */
