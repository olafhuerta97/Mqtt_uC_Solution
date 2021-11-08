/*
 * mqtt_leds.c
 *
 *  Created on: Oct 26, 2021
 *      Author: Olaf
 */
#include "MQTT_leds.h"

#define INFORM 	"/Inform"
#define TIME 	"/Time"

#define LEDNUMBERSTRINGSIZE        6
#define ON    	"ON"
#define OFF   	"OFF"
#define TOGGLE 	"TOGGLE"
#define STATUS 	"STATUS"

typedef enum Leds_enum
{
	Led1,
	Led2,
	Led3,
	Number_Of_Leds
} Leds_t;

#define COMPARE_STR(DATA,POINTER_TO_DATA,LEN)     strncmp(POINTER_TO_DATA, DATA,strlen(DATA)) == 0 && LEN == strlen(DATA)

typedef enum Leds_Commands_enum
{
	No_command        = 0x00,
	Action,
	Inform,
	Time
} Leds_Commands_t;

typedef enum Leds_Actions_enum
{
	Off      = 0x00U,
	On       = 0x01U,
	Toggle   = 0x02U,
	None
} Led_Action_t;


typedef struct leds_info_s{
	char LedTopic[LEDNUMBERSTRINGSIZE];
	u8_t 		action_pending;
	Leds_Commands_t 	command;
	u8_t      	inform;
	u16_t    	time;
}leds_info_struct_t;

#define LED_MASTER_ARRAY  									   				 \
		{"/Led1",	FALSE,	No_command , FALSE, 10u },			\
		{"/Led2",	FALSE,	No_command , FALSE, 10u },			\
		{"/Led3",	FALSE,	No_command , FALSE, 10u },			\


static leds_info_struct_t led_info_struct[Number_Of_Leds] = {LED_MASTER_ARRAY };


static void Hw_Led_Action(GPIO_TypeDef* GPIO_led, u16_t Pin, Led_Action_t Action);
static void Led_Action_Handler(Leds_t Led_no, const char* data, u16_t len);
static void Leds_Get_GPIO_and_Pin(Leds_t Led_no,GPIO_TypeDef** GPIO_led,u16_t *Pin);


/*Timer runs every second*/
void LedsTimerHandler(TIM_HandleTypeDef *htim){
	static u32_t seconds = 0;
	GPIO_TypeDef* GPIO_led = NULL;
	Leds_t Led_index;
	u16_t Pin;
	char msg[50];
	for(Led_index = 0;Led_index<Number_Of_Leds;++Led_index){
		if(led_info_struct[Led_index].inform == TRUE && 0 == ++seconds%led_info_struct[Led_index].time)
		{
			Leds_Get_GPIO_and_Pin(Led_index,&GPIO_led,&Pin);
			sprintf(msg, "Led %d status is %d",Led_index+1, HAL_GPIO_ReadPin(GPIO_led, Pin) );
			Mqtt_Publish_Cust(led_info_struct[Led_index].LedTopic,msg , LEDS);
		}
	}

}


void* mqtt_leds_get_subtopic(const char *subtopic){
	const char* command_info;
	Leds_t Led_index;
	//PRINT_MESG_UART("Led topic detected %s\n", subtopic);
	for(Led_index = 0;Led_index<Number_Of_Leds;++Led_index){
		if(strncmp(subtopic, led_info_struct[Led_index].LedTopic,LEDNUMBERSTRINGSIZE-1) == 0)
		{
			led_info_struct[Led_index].action_pending = TRUE;
			break;
		}
	}
	if(led_info_struct[Led_index].action_pending == FALSE)
	{
		Mqtt_Publish_Cust("","Invalid topic format", LEDS);
		PRINT_MESG_UART("Led topic invalid\n");
		return &led_info_struct;
	}
	command_info = &subtopic[LEDNUMBERSTRINGSIZE-1];

	if(strcmp(command_info, INFORM) == 0 ) {
		led_info_struct[Led_index].command=Inform;
	}
	else if (*command_info == 0)
	{
		led_info_struct[Led_index].command=Action;
	}
	else if (strcmp(command_info, TIME) == 0 )
	{
		led_info_struct[Led_index].command=Time;
	}
	else
	{
		PRINT_MESG_UART("Led action invalid\n");
		led_info_struct[Led_index].command= None;
		led_info_struct[Led_index].action_pending = FALSE;
		Mqtt_Publish_Cust(led_info_struct[Led_index].LedTopic,
		"Invalid subtopic format", LEDS);
	}
	return &led_info_struct;
}

void mqtt_leds_handler(const char * data, u16_t len , void* subtopics_void){
	leds_info_struct_t* subtopics =(leds_info_struct_t*)subtopics_void;
	Leds_t Led_index;
	u32_t new_time;
	u8_t atoi_succeed;
	for(Led_index = 0;Led_index<Number_Of_Leds;++Led_index)
	{
		if(subtopics[Led_index].action_pending == TRUE)
		{
			subtopics[Led_index].action_pending = FALSE;
			//PRINT_MESG_UART("detected led %d\n", Led_index);
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
					//PRINT_MESG_UART("Format incorrect\n");
					Mqtt_Publish_Cust(led_info_struct[Led_index].LedTopic,
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


static void Led_Action_Handler(Leds_t Led_no, const char* data, u16_t len){
	GPIO_TypeDef* GPIO_led = NULL ;
	u16_t Pin;
	Led_Action_t Action;
	char msg[50];

	if (Led_no < Number_Of_Leds)
	{
		Leds_Get_GPIO_and_Pin(Led_no,&GPIO_led,&Pin);
	}
	else{
		PRINT_MESG_UART("Invalid subtopic in led number: %d\n", Led_no+1);
		Mqtt_Publish_Cust("","Internal error", LEDS);
		return;
	}
	Action=None;
	if(COMPARE_STR(ON,data,len)) {
		Action=On;
		Mqtt_Publish_Cust(led_info_struct[Led_no].LedTopic,"Led switched on" , LEDS);
	}
	else if(COMPARE_STR(OFF,data,len))
	{
		Action=Off;
		Mqtt_Publish_Cust(led_info_struct[Led_no].LedTopic,"Led switched off" , LEDS);
	}
	else if(COMPARE_STR(TOGGLE,data,len))
	{
		Action=Toggle;
		Mqtt_Publish_Cust(led_info_struct[Led_no].LedTopic,"Led Toggled" , LEDS);
	}
	else if(COMPARE_STR(STATUS,data,len))
	{
		sprintf(msg, "Led %d status is %d",Led_no+1, HAL_GPIO_ReadPin(GPIO_led, Pin) );
		Mqtt_Publish_Cust(led_info_struct[Led_no].LedTopic,msg , LEDS);
	}
	else
	{
		Mqtt_Publish_Cust(led_info_struct[Led_no].LedTopic,"Invalid action" , LEDS);
		PRINT_MESG_UART("Invalid action in LEDs \n");
	}
	if (Action != None){
		Hw_Led_Action(GPIO_led, Pin, Action);
	}
}


static void Hw_Led_Action(GPIO_TypeDef* GPIO_led, u16_t Pin, Led_Action_t Action){
	if (Action == Toggle){
		HAL_GPIO_TogglePin(GPIO_led, Pin);
	}else if (Action == On || Action == Off){
		HAL_GPIO_WritePin(GPIO_led, Pin , Action);
	}else {
		//error
		Mqtt_Publish_Cust("","Internal error" , LEDS);
	}
}

static void Leds_Get_GPIO_and_Pin(Leds_t Led_no,GPIO_TypeDef ** GPIO_led, u16_t *Pin){
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
		Mqtt_Publish_Cust("","Internal error", LEDS);
	}
}




