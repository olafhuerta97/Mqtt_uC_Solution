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
	Default,
	Info_Id,
	Id_Commands_Size
} ID_Commands_t;

typedef struct availableCommands_type {
	ID_Commands_t command_number;
	char *id_command_name;
	char **id_command_options;
}id_commands_struct_t;

char const *Default_options[1] ={"GET"};
char const *Info_options[1] =   {"GET"};


#define ID_COMMANDS_MASTER_ARRAY  			    				 \
		{Default,	    "/Info",	(char**)Default_options },			\
		{Info_Id,   	"",	        (char**)Info_options     },			\

id_commands_struct_t id_commands_struct[Id_Commands_Size]={ID_COMMANDS_MASTER_ARRAY};

typedef struct id_subtopics_type {
	ID_Commands_t id_commands;
	uint8_t valid;
}id_subtopics_t;




static id_subtopics_t id_subtopics;

// COMPARE_STR(DATA,POINTER_TO_DATA,LEN)     strncmp(POINTER_TO_DATA, DATA,strlen(DATA)) == 0 && LEN == strlen(DATA)

void* Id_Subtopics_Handler(const char *subtopic){
	id_subtopics.valid=TRUE;
	if(*subtopic == 0)
	{
		id_subtopics.id_commands = Default;
	}
	else if (!strcmp(subtopic,ID_INFO)){
		id_subtopics.id_commands = Info_Id;
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
