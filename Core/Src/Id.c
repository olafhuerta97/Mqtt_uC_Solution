/*
 * Id.c
 *
 *  Created on: Oct 27, 2021
 *      Author: Olaf
 */
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include "config.h"
#include "Id.h"
#include "Mqtt_cust.h"
#define SW_VERSION 								"0.0.1"
#define  GET_ID          						"GET"

static id_subtopics Id_Topic_Info;

void* mqtt_id_get_subtopic(const char *subtopic){
	if(*subtopic == 0) {
		Id_Topic_Info.Valid=TRUE;
	}else{
		Id_Topic_Info.Valid=FALSE;
	}
	return &Id_Topic_Info;
}
void mqtt_id_handler(const char * data, u16_t len , void* subtopics_void){
	id_subtopics* subtopics =(id_subtopics*)subtopics_void;
	if(subtopics->Valid != TRUE){
		PRINT_MESG_UART("Invalid subtopic in topic ID%\n");
		return;
	}else{
		if(strncmp(data, GET_ID,strlen(GET_ID)) == 0) {
			mqtt_publish_cust("",SW_VERSION,ID);
		}else {
			PRINT_MESG_UART("Invalid subtopic in topic ID%\n");
		}
	}

}
