/*
 * Id.c
 *
 *  Created on: Oct 27, 2021
 *      Author: Olaf
*/

#include "MQTT_main.h"
#include "MQTT_sw_id.h"

#define  SW_VERSION 								"SW ID 0.1.0"
#define  GET_ID          						     "GET"
#define  GET_INFO          						     "GET"
#define  ID_INFO									"/Info"

typedef enum Leds_Commands_enum
{
	No_command        = 0x00,
	Default,
	Info_Id
} ID_Commands_t;

typedef struct {
	ID_Commands_t id_commands;
	uint8_t valid;
}id_subtopics;

static id_subtopics Id_Topic_Info;

// COMPARE_STR(DATA,POINTER_TO_DATA,LEN)     strncmp(POINTER_TO_DATA, DATA,strlen(DATA)) == 0 && LEN == strlen(DATA)

void* Id_Subtopics_Handler(const char *subtopic){
	Id_Topic_Info.valid=TRUE;
	if(*subtopic == 0)
	{
		Id_Topic_Info.id_commands = Default;
	}
	else if (!strcmp(subtopic,ID_INFO)){
		Id_Topic_Info.id_commands = Info_Id;
	}
	else
	{
		Id_Topic_Info.valid=FALSE;
		Mqtt_Publish_Cust("","Wrong topic",Id);
	}
	PRINT_MESG_UART("ID subtopic %d\n" , Id_Topic_Info.valid);
	return &Id_Topic_Info;
}

void Id_Data_Handler(const char * data, u16_t len , void* subtopics_void){
	id_subtopics* subtopics =(id_subtopics*)subtopics_void;
	if(subtopics->valid != TRUE)
	{
		PRINT_MESG_UART("Invalid subtopic in topic ID%\n");
		return;
	}
	else{
		if(COMPARE_STR(GET_ID,data,len) && subtopics->id_commands == Default)
		{
			Mqtt_Publish_Cust("",SW_VERSION,Id);
		}
		else if (COMPARE_STR(GET_INFO,data,len) && subtopics->id_commands == Info_Id)
		{
			Mqtt_Publish_Cust("","Available ID topics are No subtopic and Info ",Id);
		}
		else
		{
			PRINT_MESG_UART("Invalid data in topic ID%\n");
			Mqtt_Publish_Cust("","Wrong command",Id);
		}
	}

}
