/*
 * my_timer.h
 *
 *  Created on: Oct 25, 2021
 *      Author: Olaf
 */

#ifndef MY_TIMER_H_
#define MY_TIMER_H_

#include <stdlib.h>

typedef enum
{
TIMER_SINGLE_SHOT = 0, /*Periodic Timer*/
TIMER_PERIODIC         /*Single Shot Timer*/
} t_timer;

typedef void (*time_handler)(size_t timer_id, void * user_data);

size_t start_timer(unsigned int interval, time_handler handler, void * user_data);
//void stop_timer(size_t timer_id);
void finalize();


#endif /* MY_TIMER_H_ */
