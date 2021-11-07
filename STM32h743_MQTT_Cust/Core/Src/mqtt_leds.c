/*
 * mqtt_leds.c
 *
 *  Created on: Oct 26, 2021
 *      Author: Olaf
 */
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include "MQTT_main.h"
#include "main.h"
#include "mqtt.h"
#include "mqtt_leds.h"
#include "utils_cust.h"

#define INFORM 	"/Inform"
#define TIME 	"/Time"

#define LEDNUMBERSTRINGSIZE        6
#define ON    	"ON"
#define OFF   	"OFF"
#define TOGGLE 	"TOGGLE"
#define STATUS 	"STATUS"
typedef enum
{
	Led1,
	Led2,
	Led3,
	Number_of_leds
} Leds;

#define COMPARE_STR(DATA,POINTER_TO_DATA,LEN)     strncmp(POINTER_TO_DATA, DATA,strlen(DATA)) == 0 && LEN == strlen(DATA)

typedef enum
{
	Off      = 0x00U,
	On       = 0x01U,
	Toggle   = 0x02U,
	None
} Led_Action;

typedef enum
{
	No_command        = 0x00,
	Action,
	Inform,
	Time
} Leds_commands;


typedef struct {
	char LedTopic[LEDNUMBERSTRINGSIZE];
	uint8_t 		action_pending;
	Leds_commands 	command;
	uint8_t      	inform;
	uint16_t    	time;
}led_info_struct;

#define LED_MASTER_ARRAY  									   				 \
		{"/Led1",	FALSE,	No_command , FALSE, 1u },			\
		{"/Led2",	FALSE,	No_command , FALSE, 1u },			\
		{"/Led3",	FALSE,	No_command , FALSE, 1u },			\


static led_info_struct mqtt_led_info[Number_of_leds] = {LED_MASTER_ARRAY };


static void Hw_Led_Action(GPIO_TypeDef* GPIO_led, uint16_t Pin, Led_Action Action);
static void Led_Action_Handler(Leds Led_no, const char* data, u16_t len);
static void Leds_Get_GPIO_and_Pin(Leds Led_no,GPIO_TypeDef** GPIO_led,uint16_t *Pin);


/*Timer runs every second*/
void LedsTimerHandler(TIM_HandleTypeDef *htim){
	static uint32_t seconds = 0;
	GPIO_TypeDef* GPIO_led = NULL;
	Leds Led_index;
	uint16_t Pin;
	char msg[50];
	for(Led_index = 0;Led_index<Number_of_leds;++Led_index){
		if(mqtt_led_info[Led_index].inform == TRUE && 0 == seconds%mqtt_led_info[Led_index].time)
		{
			Leds_Get_GPIO_and_Pin(Led_index,&GPIO_led,&Pin);
			sprintf(msg, "Led %d status is %d",Led_index+1, HAL_GPIO_ReadPin(GPIO_led, Pin) );
			mqtt_publish_cust(mqtt_led_info[Led_index].LedTopic,msg , LEDS);
		}
	}
	seconds++;
}


void* mqtt_leds_get_subtopic(const char *subtopic){
	const char* command_info;
	Leds Led_index;
	PRINT_MESG_UART("Led topic detected %s\n", subtopic);
	for(Led_index = 0;Led_index<Number_of_leds;++Led_index){
		if(strncmp(subtopic, mqtt_led_info[Led_index].LedTopic,LEDNUMBERSTRINGSIZE-1) == 0)
		{
			mqtt_led_info[Led_index].action_pending = TRUE;
			break;
		}
	}
	if(mqtt_led_info[Led_index].action_pending == FALSE)
	{
		mqtt_publish_cust("","Invalid topic format", LEDS);
		PRINT_MESG_UART("Led topic invalid\n");
		return &mqtt_led_info;
	}
	command_info = &subtopic[LEDNUMBERSTRINGSIZE-1];

	if(strcmp(command_info, INFORM) == 0 ) {
		mqtt_led_info[Led_index].command=Inform;
	}
	else if (*command_info == 0)
	{
		mqtt_led_info[Led_index].command=Action;
	}
	else if (strcmp(command_info, TIME) == 0 )
	{
		mqtt_led_info[Led_index].command=Time;
	}
	else
	{
		PRINT_MESG_UART("Led action invalid\n");
		mqtt_led_info[Led_index].command= None;
		mqtt_led_info[Led_index].action_pending = FALSE;
		mqtt_publish_cust(mqtt_led_info[Led_index].LedTopic,
		"Invalid subtopic format", LEDS);
	}
	return &mqtt_led_info;
}

void mqtt_leds_handler(const char * data, u16_t len , void* subtopics_void){
	led_info_struct* subtopics =(led_info_struct*)subtopics_void;
	Leds Led_index;
	uint32_t new_time;
	uint8_t atoi_succeed;
	for(Led_index = 0;Led_index<Number_of_leds;++Led_index)
	{
		if(subtopics[Led_index].action_pending == TRUE)
		{
			subtopics[Led_index].action_pending = FALSE;
			PRINT_MESG_UART("detected led %d\n", Led_index);
			if(subtopics[Led_index].command == Action)
			{
				Led_Action_Handler(Led_index, data , len);
			}
			else if(subtopics[Led_index].command == Inform)
			{
				if(COMPARE_STR(ON,data,len))
				{
					subtopics[Led_index].inform = TRUE;
				}else if(COMPARE_STR(OFF,data,len))
				{
					subtopics[Led_index].inform = FALSE;
				}else {
					PRINT_MESG_UART("Command invalid");
				}
			}
			else if(subtopics[Led_index].command == Time)
			{
				atoi_succeed = Atoi_Cust((char*)data, len, &new_time);
				if (atoi_succeed == TRUE){
					subtopics->time = new_time;
				}
				else
				{
					PRINT_MESG_UART("Format incorrect\n");
					mqtt_publish_cust(mqtt_led_info[Led_index].LedTopic,
					"Invalid format for time, please just enter seconds number", LEDS);
				}
			}else
			{
				PRINT_MESG_UART("Command invalid");
			}

			break;
		}
	}

}


static void Led_Action_Handler(Leds Led_no, const char* data, u16_t len){
	GPIO_TypeDef* GPIO_led = NULL ;
	uint16_t Pin;
	Led_Action Action;
	char msg[50];

	if (Led_no < Number_of_leds)
	{
		Leds_Get_GPIO_and_Pin(Led_no,&GPIO_led,&Pin);
	}
	else{
		PRINT_MESG_UART("Invalid subtopic in led number: %d\n", Led_no+1);
		mqtt_publish_cust("","Internal error", LEDS);
		return;
	}
	Action=None;
	if(COMPARE_STR(ON,data,len)) {
		Action=On;
		mqtt_publish_cust(mqtt_led_info[Led_no].LedTopic,"Led switched on" , LEDS);
	}
	else if(COMPARE_STR(OFF,data,len))
	{
		Action=Off;
		mqtt_publish_cust(mqtt_led_info[Led_no].LedTopic,"Led switched off" , LEDS);
	}
	else if(COMPARE_STR(TOGGLE,data,len))
	{
		Action=Toggle;
		mqtt_publish_cust(mqtt_led_info[Led_no].LedTopic,"Led Toggled" , LEDS);
	}
	else if(COMPARE_STR(STATUS,data,len))
	{
		sprintf(msg, "Led %d status is %d",Led_no+1, HAL_GPIO_ReadPin(GPIO_led, Pin) );
		mqtt_publish_cust(mqtt_led_info[Led_no].LedTopic,msg , LEDS);
	}
	else
	{
		mqtt_publish_cust(mqtt_led_info[Led_no].LedTopic,"Invalid action" , LEDS);
		PRINT_MESG_UART("Invalid action in LEDs \n");
	}
	if (Action != None){
		Hw_Led_Action(GPIO_led, Pin, Action);
	}
}


static void Hw_Led_Action(GPIO_TypeDef* GPIO_led, uint16_t Pin, Led_Action Action){
	if (Action == Toggle){
		HAL_GPIO_TogglePin(GPIO_led, Pin);
	}else if (Action == On || Action == Off){
		HAL_GPIO_WritePin(GPIO_led, Pin , Action);
	}else {
		//error
		mqtt_publish_cust("","Internal error" , LEDS);
	}
}

static void Leds_Get_GPIO_and_Pin(Leds Led_no,GPIO_TypeDef** GPIO_led,uint16_t *Pin){
	if(Led_no == Led1){
		*GPIO_led = LD1_GPIO_Port;
		*Pin = LD1_Pin;
	}else if(Led_no == Led2){
		*GPIO_led = LD2_GPIO_Port;
		*Pin = LD2_Pin;
	}
	else if (Led_no ==Led3){
		*GPIO_led = LD3_GPIO_Port;
		*Pin = LD3_Pin;
	}else  {
		PRINT_MESG_UART("Failed to set pointer \n");
		mqtt_publish_cust("","Internal error", LEDS);
	}
}




