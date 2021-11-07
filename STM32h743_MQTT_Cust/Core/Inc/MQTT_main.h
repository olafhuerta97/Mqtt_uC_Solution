/*
 * mqtt_cust.h
 *
 *  Created on: Oct 24, 2021
 *      Author: Olaf
 */

#ifndef INC_MQTT_MAIN_H_
#define INC_MQTT_MAIN_H_
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include "lwip/arch.h"
#include "config.h"
#include "utils_cust.h"
#include "main.h"


/*This enum should be global so all modules can now their enum*/
typedef enum
{
  INFO    = 0,
  LEDS,
  ID,
  BUTTON,
  HEARTBEAT,
  NUMBER_OF_TOPICS
} Mqtt_topics;

#define TRUE 	0u
#define FALSE 	1u

#define OUTPUT     				"/Output"
#define INPUT     				"/Input"
#define SUSCRIBE_TOPIC   		"/#"
#define INFO_TOPIC 				"/Info"
#define LEDS_TOPIC 				"/Leds"
#define ID_TOPIC 				"/Id"
#define BUTTON_TOPIC 			"/Button"
#define HB_TOPIC 			    "/HeartBeat"

#define FREE_TIMER_1			TIM2
#define HB_TIMER 			    TIM3
#define LEDS_TIMER 	     	    TIM4
#define FREE_TIMER_2 		    TIM5

extern TIM_HandleTypeDef htim3;

void Mqtt_Do_Connect(void);
void Mqtt_Ext_Int_ISR_Handler(u16_t GPIO_Pin);
void Mqtt_Timer_ISR_Handler(TIM_HandleTypeDef *htim);

void Mqtt_Publish_Cust(const char *subtopic, const char *pub_payload,Mqtt_topics sender);

#endif /* INC_MQTT_MAIN_H_ */
