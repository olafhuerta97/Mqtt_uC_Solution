/*
 * HeartBeat.c
 *
 *  Created on: Oct 31, 2021
 *      Author: Olaf
 */
#include "MQTT_main.h"
#include "HeartBeat.h"

#define TIMESUBTOPIC   "/Time"

typedef enum{
	Time,
	Status,
	None
}HB_Action;

typedef struct {
	HB_Action action_pending;
	uint8_t Status;  //ON or OFF
	uint16_t time;   // Time in seconds
}HB_info_type;

static HB_info_type HB_info = {FALSE,TRUE,5};

void HeartBeatTimerHandler(TIM_HandleTypeDef *htim){
	char msg[50];
	static uint32_t seconds = 0;
	if (HB_info.Status == TRUE &&  0 == seconds%HB_info.time)
	{
		sprintf(msg, "HeartBeat every %ds", HB_info.time);
		Mqtt_Publish_Cust("",msg,HEARTBEAT);
	}
	seconds++;

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
			if (new_time >= 1 && new_time <= 3600){ //max interval 3600 seconds (1 hour)
				HB_info_incoming->time = new_time;
				Mqtt_Publish_Cust("","Set Heartbeat time", HEARTBEAT);
			}
			else{
				Mqtt_Publish_Cust("","Heartbeat out of bounds", HEARTBEAT);
			}

		}else {
			PRINT_MESG_UART("Format incorrect\n");
			Mqtt_Publish_Cust("","Format incorrect\n", HEARTBEAT);
		}
	}
	else if (HB_info_incoming->action_pending == Status){
		if (strncmp(data, "ON",strlen("ON")) == 0  && len == strlen ("ON")){
			HB_info.Status = TRUE;
			Mqtt_Publish_Cust("","Heartbeat turn on", HEARTBEAT);
		}
		else if (strncmp(data, "OFF",strlen("OFF")) == 0  && len == strlen("OFF")){
			HB_info.Status = FALSE;
			Mqtt_Publish_Cust("","Heartbeat turn off", HEARTBEAT);
		}
		else {
			PRINT_MESG_UART("Invalid data ON/OFF\n");
			Mqtt_Publish_Cust("","Invalid data ON/OFF", HEARTBEAT);
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
	}
	else if(*subtopic == 0)
	{
		HB_info.action_pending = Status;
	}
	else
	{
		Mqtt_Publish_Cust("","Heartbeat topic invalid\n", HEARTBEAT);
		HB_info.action_pending = None;
		PRINT_MESG_UART("Heartbeat topic invalid\n");
	}
	return &HB_info;
}


