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

typedef struct availableCommands_type {
	ID_Commands_t command_number;
	char *id_command_name;
	char **id_command_options;
}id_commands_struct_t;

char const *Default_options[1] ={"GET"};
char const *Info_options[1] =   {"GET"};


#define ID_COMMANDS_MASTER_ARRAY  			            				 \
		{Default_id,	    "",	             (char**)Default_options },			\
		{Info_Id,   	"/Info",	        (char**)Info_options     },			\

static const id_commands_struct_t id_commands_struct[Id_Commands_Size]={ID_COMMANDS_MASTER_ARRAY};


typedef struct id_subtopics_type {
	ID_Commands_t current_id_command;
	uint8_t valid;
}id_subtopics_t;


static id_subtopics_t id_subtopics;


static void Id_Publish_Info_Topics(void);
// COMPARE_STR(DATA,POINTER_TO_DATA,LEN)     strncmp(POINTER_TO_DATA, DATA,strlen(DATA)) == 0 && LEN == strlen(DATA)

void* Id_Subtopics_Handler(const char *subtopic){
	id_subtopics.valid=TRUE;
	if(*subtopic == 0)
	{
		id_subtopics.current_id_command = Default_id;
	}
	else if (!strcmp(subtopic,id_commands_struct[Info_Id].id_command_name)){
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
		if(COMPARE_STR(id_commands_struct[Default_id].id_command_options[0],data,len)
				&& subtopics->current_id_command == Default_id)
		{
			Mqtt_Publish_Cust("",SW_VERSION,Id);
		}
		else if (COMPARE_STR(id_commands_struct[Info_Id].id_command_options[0],data,len)
				&& subtopics->current_id_command == Info_Id)
		{
			Id_Publish_Info_Topics();
		}
		else
		{
			PRINT_MESG_UART("Invalid data in topic ID%\n");
			Mqtt_Publish_Cust("","Wrong command",Id);
		}
	}
}

static void Id_Publish_Info_Topics(void){
	char id_info_msg[MQTT_OUTPUT_RINGBUF_SIZE];
	u8_t message_char_counter= 0;
	ID_Commands_t topics_available;


	/*Print valid topics*/

	sprintf(&id_info_msg[message_char_counter],AVAILABLETOPICS);
	for (topics_available = 0u; topics_available < Id_Commands_Size; topics_available++)
	{
		message_char_counter= strlen(id_info_msg);
		if((message_char_counter+strlen(id_commands_struct[topics_available].id_command_name))
				> MQTT_OUTPUT_RINGBUF_SIZE)
		{
			PRINT_MESG_UART("Array not big enough for printing all topics\n");
			return;
		}
		sprintf(&id_info_msg[message_char_counter],"%s \n",id_commands_struct[topics_available].id_command_name);
	}


	Mqtt_Publish_Cust("Info", id_info_msg , Id);

}
