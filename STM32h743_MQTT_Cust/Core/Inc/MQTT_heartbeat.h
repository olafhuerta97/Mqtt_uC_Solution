/*
 * HeartBeat.h
 *
 *  Created on: Oct 31, 2021
 *      Author: Olaf
 */

#ifndef INC_HEARTBEAT_H_
#define INC_HEARTBEAT_H_

void Hb_Timer_Handler(TIM_HandleTypeDef *htim);
void* Hb_Subtopics_Handler(const char *subtopic);
void Hb_Data_Handler(const char * data, u16_t len , void* subtopics_void);


#endif /* INC_HEARTBEAT_H_ */
