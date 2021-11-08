/*
 * HeartBeat.h
 *
 *  Created on: Oct 31, 2021
 *      Author: Olaf
 */

#ifndef INC_HEARTBEAT_H_
#define INC_HEARTBEAT_H_

void HeartBeatTimerHandler(TIM_HandleTypeDef *htim);
void HeartBeat_TopicHandler(const char * data, u16_t len , void* subtopics_void);
void* HeartBeat_SubTopicHandler(const char *subtopic);

#endif /* INC_HEARTBEAT_H_ */
