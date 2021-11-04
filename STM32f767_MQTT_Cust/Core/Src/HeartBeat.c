/*
 * HeartBeat.c
 *
 *  Created on: Oct 31, 2021
 *      Author: Olaf
 */
#include "mqtt_cust.h"
#include "HeartBeat.h"
#include "utils_cust.h"

#define TIMESUBTOPIC   "/Time"

typedef enum{
	Time,
	Status,
	None
}HB_Action;

typedef struct {
	HB_Action action_pending;
	uint8_t Status;  //ON or OFF
	uint16_t time;   // Time in ms Min 0 Max 32700
}HB_info_type;

static HB_info_type HB_info = {FALSE,TRUE,5000};

void HeartBeatTimerHandler(TIM_HandleTypeDef *htim){
	char msg[50];
	if (HB_info.Status == TRUE)
	{
		sprintf(msg, "HeartBeat every %d ms", HB_info.time);
		mqtt_publish_cust("",msg,HEARTBEAT);
	}

}

void HeartBeat_TopicHandler(const char * data, u16_t len , void* subtopics_void){
	HB_info_type* HB_info_incoming  = (HB_info_type*) subtopics_void;
	uint32_t new_time;
	uint8_t atoi_succeed;
	PRINT_MESG_UART("HeartBeat Topic Handler\n");
	if (HB_info_incoming->action_pending == None){
		return;
	}
	else if (HB_info_incoming->action_pending == Time){
		atoi_succeed = Atoi_Cust((char*)data, len, &new_time);
		if (atoi_succeed == TRUE){
			if (new_time >= 500 && new_time <= 30000){

				__HAL_TIM_SET_AUTORELOAD(&htim3,new_time*2);
				__HAL_TIM_SET_COUNTER(&htim3,0);
				HB_info_incoming->time = new_time;
				mqtt_publish_cust("","Set Heartbeat time", HEARTBEAT);
			}
			else{
				mqtt_publish_cust("","Heartbeat out of bounds", HEARTBEAT);
			}

		}else {
			PRINT_MESG_UART("Format incorrect\n");
			mqtt_publish_cust("","Format incorrect\n", HEARTBEAT);
		}
	}
	else if (HB_info_incoming->action_pending == Status){
		if (strncmp(data, "ON",strlen("ON")) == 0  && len == strlen ("ON")){
			HB_info.Status = TRUE;
			mqtt_publish_cust("","Heartbeat turn on", HEARTBEAT);
		}
		else if (strncmp(data, "OFF",strlen("OFF")) == 0  && len == strlen("OFF")){
			HB_info.Status = FALSE;
			mqtt_publish_cust("","Heartbeat turn off", HEARTBEAT);
		}
		else {
			PRINT_MESG_UART("Invalid data ON/OFF\n");
			mqtt_publish_cust("","Invalid data ON/OFF", HEARTBEAT);
		}
	}
	else {
		PRINT_MESG_UART("Should never be here \n");
	}

}

void* HeartBeat_SubTopicHandler(const char *subtopic){
	PRINT_MESG_UART("HeartBeat topic detected %s\n", subtopic);

	if(strncmp(subtopic, TIMESUBTOPIC,strlen(TIMESUBTOPIC)) == 0)
	{
		HB_info.action_pending = Time;
		PRINT_MESG_UART("Time change detected%d\n");
	}else if(*subtopic == 0){
		HB_info.action_pending = Status;
	}
	else
	{
		mqtt_publish_cust("","Heartbeat topic invalid\n", HEARTBEAT);
		HB_info.action_pending = None;
		PRINT_MESG_UART("Heartbeat topic invalid\n");
	}
	return &HB_info;
}


