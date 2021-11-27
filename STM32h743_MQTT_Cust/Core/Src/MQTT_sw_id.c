/*
 * Id.c
 *
 *  Created on: Oct 27, 2021
 *      Author: Olaf
*/

#include "MQTT_main.h"
#include "MQTT_sw_id.h"

#define  SW_VERSION 								"SW ID 0.1.0"


typedef enum Leds_Commands_enum
{
	Default_id,
	Info_Id,
	Id_Commands_Size
} ID_Commands_t;


static char const *Default_options[] ={"GET"};
static char const *Info_options[] =   {"GET"};


#define ID_COMMANDS_MASTER_ARRAY  			            				 \
		{Default_id,	     "", (char**)Default_options , sizeof(Default_options)/sizeof(char**) },\
		{Info_Id   ,   	"/Info", (char**)Info_options    , sizeof(Info_options)/sizeof(char**) },\

static const commands_info_struct_t id_commands_struct[Id_Commands_Size]={ID_COMMANDS_MASTER_ARRAY};


typedef struct id_subtopics_type {
	ID_Commands_t current_id_command;
	uint8_t valid;
}id_subtopics_t;


static id_subtopics_t id_subtopics;

// COMPARE_STR(DATA,POINTER_TO_DATA,LEN)     strncmp(POINTER_TO_DATA, DATA,strlen(DATA)) == 0 && LEN == strlen(DATA)

void* Id_Subtopics_Handler(const char *subtopic){
	id_subtopics.valid=TRUE;
	if(*subtopic == 0)
	{
		id_subtopics.current_id_command = Default_id;
	}
	else if (!strcmp(subtopic,id_commands_struct[Info_Id].command_name)){
		id_subtopics.current_id_command = Info_Id;
	}
	else
	{
		id_subtopics.valid=FALSE;
		Mqtt_Publish_Cust("","Wrong topic",Id);
	}
	PRINT_MESG_UART("ID subtopic %d\n" , id_subtopics.valid);
	return &id_subtopics;
}


void Id_Data_Handler(const char * data, u16_t len , void* subtopics_void){
	id_subtopics_t* subtopics =(id_subtopics_t*)subtopics_void;
	if(id_subtopics.valid != TRUE)
	{
		PRINT_MESG_UART("Invalid subtopic in topic ID%\n");
		return;
	}
	else{
		if(COMPARE_STR(id_commands_struct[Default_id].command_options[0],data,len)
				&& subtopics->current_id_command == Default_id)
		{
			Mqtt_Publish_Cust("",SW_VERSION,Id);
		}
		else if (COMPARE_STR(id_commands_struct[Info_Id].command_options[0],data,len)
				&& subtopics->current_id_command == Info_Id)
		{
			Mqtt_Publish_Subtopic_Info(id_commands_struct, Id_Commands_Size, Id);
		}
		else
		{
			PRINT_MESG_UART("Invalid data in topic ID%\n");
			Mqtt_Publish_Cust("","Wrong command",Id);
		}
	}
}

