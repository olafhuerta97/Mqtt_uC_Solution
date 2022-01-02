/*
 * HeartBeat.c
 *
 *  Created on: Oct 31, 2021
 *      Author: Olaf
 */
#include "MQTT_main.h"
#include "MQTT_heartbeat.h"


typedef enum Hb_Actions_enum{
	Time,
	Status,
	Info_HB,
	Number_Of_Actions
}Hb_Actions;

typedef struct hb_info_struct_type{
	Hb_Actions action_pending;
	uint8_t status;  //ON or OFF
	uint16_t time;   // Time in seconds
}hb_info_struct_t;

static char const *Status_options[] ={"ON", "OFF"};
static char const *Info_options[] =   {"GET","(null)"};
static char const *Time_options[] =   {"(Number of seconds to set HeartBeat Pulse)"};

#define HB_COMMANDS_MASTER_ARRAY  			            				 \
		{Time      ,	"/Time", (char**)Time_options   , sizeof(Time_options)  /sizeof(char**) },\
		{Status    ,       	"" , (char**)Status_options , sizeof(Status_options)/sizeof(char**) },\
		{Info_HB   ,   	"/Info", (char**)Info_options   , sizeof(Info_options)  /sizeof(char**) },\

static const commands_info_struct_t hb_commands_struct[Number_Of_Actions]={HB_COMMANDS_MASTER_ARRAY};
static hb_info_struct_t hb_info_struct = {Number_Of_Actions,TRUE,10};


void Hb_Timer_Handler(TIM_HandleTypeDef *htim){
	char msg[50];
	static uint32_t seconds = 0;
	if (hb_info_struct.status == TRUE &&  0 == ++seconds%hb_info_struct.time)
	{
		sprintf(msg, "HeartBeat every %ds", hb_info_struct.time);
		Mqtt_Publish_Cust("",msg,Heartbeat);
	}
}

void* Hb_Subtopics_Handler(const char *subtopic){
	PRINT_MESG_UART("HeartBeat topic detected %s\n", subtopic);

	if(strcmp(subtopic, hb_commands_struct[Time].command_name) == 0)
	{
		hb_info_struct.action_pending = Time;
		PRINT_MESG_UART("Time change detected%d\n");
	}
	else if(*subtopic == 0)
	{
		hb_info_struct.action_pending = Status;
	}
	else if (strcmp(subtopic, hb_commands_struct[Info_HB].command_name) == 0)
	{
		hb_info_struct.action_pending = Info_HB;
	}
	else
	{
		Mqtt_Publish_Cust("","Heartbeat topic invalid\n", Heartbeat);
		hb_info_struct.action_pending = Number_Of_Actions;
		PRINT_MESG_UART("Heartbeat topic invalid\n");
	}
	return &hb_info_struct;
}


void Hb_Data_Handler(const char * data, u16_t len , void* subtopics_void){
	hb_info_struct_t* HB_info_incoming  = (hb_info_struct_t*) subtopics_void;
	uint32_t new_time;
	uint8_t atoi_succeed;
	PRINT_MESG_UART("HeartBeat Topic Handler\n");
	if (HB_info_incoming->action_pending == Number_Of_Actions){
		return;
	}
	else if (HB_info_incoming->action_pending == Time){
		atoi_succeed = Utils_Atoi_Cust((char*)data, len, &new_time);
		if (atoi_succeed == TRUE){
			if (new_time >= 1 && new_time <= 3600){ //max interval 3600 seconds (1 hour)
				HB_info_incoming->time = new_time;
				Mqtt_Publish_Cust("","Set Heartbeat time", Heartbeat);
			}
			else{
				Mqtt_Publish_Cust("","Heartbeat out of bounds", Heartbeat);
			}

		}else {
			PRINT_MESG_UART("Format incorrect\n");
			Mqtt_Publish_Cust("","Format incorrect\n", Heartbeat);
		}
	}
	else if (HB_info_incoming->action_pending == Status){
		if (COMPARE_STR(hb_commands_struct[Status].command_options[0],data,len)){
			HB_info_incoming->status = TRUE;
			Mqtt_Publish_Cust("","Heartbeat turn on", Heartbeat);
		}
		else if (COMPARE_STR(hb_commands_struct[Status].command_options[1],data,len)){
			HB_info_incoming->status = FALSE;
			Mqtt_Publish_Cust("","Heartbeat turn off", Heartbeat);
		}
		else {
			PRINT_MESG_UART("Invalid data ON/OFF\n");
			Mqtt_Publish_Cust("","Invalid data ON/OFF", Heartbeat);
		}
	}else if (HB_info_incoming->action_pending == Info_HB){
		if ((COMPARE_STR(hb_commands_struct[Info_HB].command_options[0],data,len))
				||(COMPARE_STR("",data,len))){
			Mqtt_Publish_Subtopic_Info(hb_commands_struct, Number_Of_Actions, Heartbeat);
		}
		else
		{
			PRINT_MESG_UART("Invalid data Info\n");
			Mqtt_Publish_Cust("","Invalid data Info", Heartbeat);
		}
	}
	else {
		PRINT_MESG_UART("Should never be here \n");
	}

}
