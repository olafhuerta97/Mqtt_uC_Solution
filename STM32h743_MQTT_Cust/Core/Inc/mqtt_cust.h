/*
 * mqtt_cust.h
 *
 *  Created on: Oct 24, 2021
 *      Author: Olaf
 */

#ifndef INC_MQTT_CUST_H_
#define INC_MQTT_CUST_H_
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include "main.h"
#include "config.h"

typedef enum
{
  LEDS       = 0x00U,
  ID,
  BUTTON,
  HEARTBEAT,
  NUMBER_OF_TOPICS
} Mqtt_topics;

#define OUTPUT     				"/Output"
#define INPUT     				"/Input"
#define SUSCRIBE_TOPIC   		"/#"
#define LEDS_TOPIC 				"/Leds"
#define ID_TOPIC 				"/Id"
#define BUTTON_TOPIC 			"/Button"
#define HB_TOPIC 			    "/HeartBeat"

#define FREE_TIMER_1			TIM2
#define HB_TIMER 			    TIM3
#define LEDS_TIMER 	     	    TIM4
#define FREE_TIMER_2 		    TIM5

typedef struct {
	uint8_t Topic_valid;
	u8_t 	Qos;
	void*   Subtopics;
	 char 	Output_topic[MAX_LENGTH_TOPIC];
	 char	    Input_topic[MAX_LENGTH_TOPIC];
	void   (*(*Subtopics_handler)(const char* ));
	void   (*Topic_Handler)(const char* ,uint16_t, void*);
}topics_info_type;

void mqtt_do_connect(void);
void MQTT_Cust_HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin);
void MQTT_PeriodElapsedTim(TIM_HandleTypeDef *htim);

void mqtt_publish_cust(const char *subtopic, const char *pub_payload,Mqtt_topics sender);

#endif /* INC_MQTT_CUST_H_ */
