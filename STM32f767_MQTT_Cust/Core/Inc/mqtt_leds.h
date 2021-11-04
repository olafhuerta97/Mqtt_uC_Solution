/*
 * mqtt_leds.h
 *
 *  Created on: Oct 26, 2021
 *      Author: Olaf
 */

#ifndef INC_MQTT_LEDS_H_
#define INC_MQTT_LEDS_H_

#include "config.h"


void* mqtt_leds_get_subtopic(const char *subtopic);
void mqtt_leds_handler(const char * data, u16_t len , void* subtopics_void);
void LedsTimerHandler(TIM_HandleTypeDef *htim);

#endif /* INC_MQTT_LEDS_H_ */
