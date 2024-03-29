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
#include "main.h"
#include "MQTTClient.h"
#include "MQTT_config.h"
#include "MQTT_utils_cust.h"

#define COMPARE_STR(DATA,POINTER_TO_DATA,LEN)     strncmp(POINTER_TO_DATA, DATA,strlen(DATA)) == 0 && LEN == strlen(DATA)

/*This enum should be global so all modules can now their enum*/
typedef enum Mqtt_Topics_Enum
{
  Info    = 0,
  Leds,
  Id,
  Button,
  Heartbeat,
  Number_Of_Topics
} Mqtt_Topics_t;


typedef struct availableCommands_type {
	uint8_t command_number;
	char *command_name;
	char **command_options;
	uint8_t number_commands;
}commands_info_struct_t;


#define TRUE 	0u
#define FALSE 	1u

u8_t Mqtt_Do_Connect(void);
void Mqtt_Ext_Int_ISR_Handler(u16_t GPIO_Pin);
void Mqtt_Timer_ISR_Handler(TIM_HandleTypeDef *htim);
void MQTTYield_Cust(void);
void Mqtt_Publish_Cust(const char *subtopic, const char *pub_payload,Mqtt_Topics_t sender);
void Mqtt_Publish_Subtopic_Info(const commands_info_struct_t *topic_info,uint8_t number_of_commands,Mqtt_Topics_t sender);

#endif /* INC_MQTT_MAIN_H_ */
