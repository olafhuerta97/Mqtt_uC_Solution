/*
 * Id.c
 *
 *  Created on: Oct 27, 2021
 *      Author: Olaf
*/

#include "MQTT_main.h"
#include "SW_ID.h"

#define  SW_VERSION 								"SW ID 0.1.0"
#define  GET_ID          						     "GET"

static id_subtopics Id_Topic_Info;

void* mqtt_id_get_subtopic(const char *subtopic){
	if(*subtopic == 0)
	{
		Id_Topic_Info.Valid=TRUE;
		Mqtt_Publish_Cust("","Wrong topic",ID);
	}else
	{
		Id_Topic_Info.Valid=FALSE;
	}
	PRINT_MESG_UART("ID subtopic %d\n" , Id_Topic_Info.Valid);
	return &Id_Topic_Info;
}

void mqtt_id_handler(const char * data, u16_t len , void* subtopics_void){
	id_subtopics* subtopics =(id_subtopics*)subtopics_void;
	if(subtopics->Valid != TRUE)
	{
		PRINT_MESG_UART("Invalid subtopic in topic ID%\n");
		return;
	}
	else{
		if(strncmp(data, GET_ID,strlen(GET_ID)) == 0 && len == strlen(GET_ID))
		{
			Mqtt_Publish_Cust("",SW_VERSION,ID);
		}
		else
		{
			PRINT_MESG_UART("Invalid data in topic ID%\n");
			Mqtt_Publish_Cust("","Wrong command",ID);
		}
	}

}
