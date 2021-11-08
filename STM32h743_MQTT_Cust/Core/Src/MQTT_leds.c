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


static void Leds_Hw_Action(GPIO_TypeDef* GPIO_led, u16_t Pin, Led_Action_t Action);
static void Led_Action_Handler(Leds_t Led_no, const char* data, u16_t len);
static void Leds_Get_GPIO_and_Pin(Leds_t Led_no,GPIO_TypeDef** GPIO_led,u16_t *Pin);


/*Timer runs every second*/
void Leds_Timer_Handler(TIM_HandleTypeDef *htim){
	static u32_t seconds = 0;
	GPIO_TypeDef* gpio_led = NULL;
	Leds_t led_index;
	u16_t Pin;
	char msg[50];
	for(led_index = 0;led_index<Number_Of_Leds;++led_index){
		if(led_info_struct[led_index].inform == TRUE && 0 == ++seconds%led_info_struct[led_index].time)
		{
			Leds_Get_GPIO_and_Pin(led_index,&gpio_led,&Pin);
			sprintf(msg, "Led %d status is %d",led_index+1, HAL_GPIO_ReadPin(gpio_led, Pin) );
			Mqtt_Publish_Cust(led_info_struct[led_index].LedTopic,msg , Leds);
		}
	}
}


void* Leds_Subtopics_Handler(const char *subtopic){
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
		Mqtt_Publish_Cust("","Invalid topic format", Leds);
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
		"Invalid subtopic format", Leds);
	}
	return &led_info_struct;
}

void Leds_Data_Handler(const char * data, u16_t len , void* subtopics_void){
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
				atoi_succeed = Utils_Atoi_Cust((char*)data, len, &new_time);
				if (atoi_succeed == TRUE){
					subtopics->time = new_time;
				}
				else
				{
					//PRINT_MESG_UART("Format incorrect\n");
					Mqtt_Publish_Cust(led_info_struct[Led_index].LedTopic,
					"Invalid format for time, please just enter seconds number", Leds);
				}
			}else
			{
				PRINT_MESG_UART("Command invalid");
			}

			break;
		}
	}

}


static void Led_Action_Handler(Leds_t led_no, const char* data, u16_t len){
	GPIO_TypeDef* gpio_led = NULL ;
	u16_t pin;
	Led_Action_t action;
	char msg[50];

	if (led_no < Number_Of_Leds)
	{
		Leds_Get_GPIO_and_Pin(led_no,&gpio_led,&pin);
	}
	else
	{
		PRINT_MESG_UART("Invalid subtopic in led number: %d\n", led_no+1);
		Mqtt_Publish_Cust("","Internal error", Leds);
		return;
	}
	action=None;
	if(COMPARE_STR(ON,data,len)) {
		action=On;
		Mqtt_Publish_Cust(led_info_struct[led_no].LedTopic,"Led switched on" , Leds);
	}
	else if(COMPARE_STR(OFF,data,len))
	{
		action=Off;
		Mqtt_Publish_Cust(led_info_struct[led_no].LedTopic,"Led switched off" , Leds);
	}
	else if(COMPARE_STR(TOGGLE,data,len))
	{
		action=Toggle;
		Mqtt_Publish_Cust(led_info_struct[led_no].LedTopic,"Led Toggled" , Leds);
	}
	else if(COMPARE_STR(STATUS,data,len))
	{
		sprintf(msg, "Led %d status is %d",led_no+1, HAL_GPIO_ReadPin(gpio_led, pin) );
		Mqtt_Publish_Cust(led_info_struct[led_no].LedTopic,msg , Leds);
	}
	else
	{
		Mqtt_Publish_Cust(led_info_struct[led_no].LedTopic,"Invalid action" , Leds);
		PRINT_MESG_UART("Invalid action in LEDs \n");
	}
	if (action != None){
		Leds_Hw_Action(gpio_led, pin, action);
	}
}


static void Leds_Hw_Action(GPIO_TypeDef* gpio_led, u16_t pin, Led_Action_t action){
	if (action == Toggle){
		HAL_GPIO_TogglePin(gpio_led, pin);
	}else if (action == On || action == Off){
		HAL_GPIO_WritePin(gpio_led, pin , action);
	}else {
		//error
		Mqtt_Publish_Cust("","Internal error" , Leds);
	}
}

static void Leds_Get_GPIO_and_Pin(Leds_t led_no,GPIO_TypeDef ** gpio_led, u16_t *pin){
	if(led_no == Led1){
		*gpio_led = LD1_GPIO_Port;
		*pin = LD1_Pin;
	}else if(led_no == Led2){
		*gpio_led = LD2_GPIO_Port;
		*pin = LD2_Pin;
	}
	else if (led_no ==Led3){
		*gpio_led = LD3_GPIO_Port;
		*pin = LD3_Pin;
	}else  {
		PRINT_MESG_UART("Failed to set pointer \n");
		Mqtt_Publish_Cust("","Internal error", Leds);
	}
}




