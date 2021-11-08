/*
 * mqtt_leds.h
 *
 *  Created on: Oct 26, 2021
 *      Author: Olaf
 */

#ifndef INC_MQTT_LEDS_H_
#define INC_MQTT_LEDS_H_

#include "MQTT_main.h"

void* Leds_Subtopics_Handler(const char *subtopic);
void  Leds_Data_Handler(const char * data, u16_t len , void* subtopics_void);
void  Leds_Timer_Handler(TIM_HandleTypeDef *htim);

#endif /* INC_MQTT_LEDS_H_ */
