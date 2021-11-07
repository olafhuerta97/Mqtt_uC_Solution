/*
 * mqtt_cust.c
 *
 *  Created on: Oct 24, 2021
 *      Author: Olaf
 */
#include "mqtt.h"
#include "lwip/apps/mqtt_priv.h"
#include "mqtt_opts.h"
#include "config.h"
#include "mqtt_leds.h"
#include "utils_cust.h"
#include "button.h"
#include "SW_ID.h"
#include "HeartBeat.h"
#include "MQTT_main.h"

/*
 * THIS PROJECT IS A REMINDER TO MYSELF TO NOT USE DINAMIC MEMORY IN EMBEDDED SYSTEMS
 *
 * WHEN THIS WAS ORIGINALLY MADE ALLOCATING MEMORY IT WAS GETTING CORRUPTED.
 * */

/*Local Static Variables and Structures*/
static char mqtt_device_suscription[MAX_LENGTH_TOPIC];
static mqtt_client_t mqtt_client;
static topics_info_type device_topics[NUMBER_OF_TOPICS];
static uint8_t mqtt_topics_initializated_flag = FALSE;

/*Local Static Function Declarations*/

/*Publish Available Topics on device*/
static void Mqtt_Publish_Valid_Topics(topics_info_type* device_topics_print);

/*Initialize Topics*/
static void Mqtt_Init_All_Topics(topics_info_type* device_topics_init);
static void Mqtt_Intialize_Topic(topics_info_type* device_topics_init, Mqtt_topics Topic_To_Initialize ,
		const char* Topic_Name,void* TopicHandler, void * SubtopicHandler,uint8_t QoS );

/*Callback for events*/
static void Mqtt_Connection_CB(mqtt_client_t *client, void *arg, mqtt_connection_status_t status);
static void Mqtt_Sub_Request_CB(void *arg, err_t result) ;
static void Mqtt_Incoming_Publish_CB(void *arg, const char *topic, u32_t tot_len);
static void Mqtt_Incoming_Data_CB(void *arg, const u8_t *data, u16_t len, u8_t flags);
static void Mqtt_Pub_Request_CB(void *arg, err_t result);

/*Default handlers Declaration*/
static void Mqtt_Default_Topic_Handler(const char * data, u16_t len , void* subtopics_void);
static void* Mqtt_Default_SubTopic_Handler(const char *subtopic);

/*Function call every time the button is pressed*/
void MQTT_Cust_HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
	if (GPIO_Pin == Button_Pin_Pin)
	{
		button_handler_isr();
	}
}

/*Function call by ISR every time a timer is expired*/
void MQTT_PeriodElapsedTim(TIM_HandleTypeDef *htim)
{
	if(htim->Instance == LEDS_TIMER )
	{
		LedsTimerHandler(htim);
	}else if(htim->Instance == HB_TIMER )
	{
		HeartBeatTimerHandler(htim);
	}else if(htim->Instance == FREE_TIMER_1 )
	{

	}
	else if (htim->Instance == FREE_TIMER_2 ){

	}
	else
	{
		//Other Timer
	}
}

/*Function to publish messages from subtopics*/
void mqtt_publish_cust(const char *subtopic, const char *pub_payload,Mqtt_topics sender) {
	err_t err;
	char topic[MAX_LENGTH_TOPIC];
	size_t topic_len = strlen(device_topics[sender].Output_topic);
	size_t subtopic_len = strlen(subtopic);
	/*Calculate topic publish size and if most return*/
	if((topic_len + subtopic_len)>MAX_LENGTH_TOPIC)
	{
		PRINT_MESG_UART("Trying to publish a topic with max length \n");
		return;
	}

	u8_t qos = device_topics[sender].Qos; /* 0 1 or 2, see MQTT specification */
	u8_t retain = 0u;
	memcpy(topic, device_topics[sender].Output_topic, topic_len);
	memcpy(topic + topic_len, subtopic, subtopic_len+1);
	err = mqtt_publish(&mqtt_client, topic, pub_payload, strlen(pub_payload),
			qos, retain, Mqtt_Pub_Request_CB, (Mqtt_topics*)&sender);
	if(err != ERR_OK)
	{
		PRINT_MESG_UART("Publish err: %d\n", err);
	}
}


void Mqtt_Do_Connect(void) {

	ip4_addr_t broker_ipaddr;
	struct mqtt_connect_client_info_t ci;
	err_t err;

	IP4_ADDR(&broker_ipaddr, configIP_MQTT_ADDR0, configIP_MQTT_ADDR1, configIP_MQTT_ADDR2,
			configIP_MQTT_ADDR3);

	/* Setup an empty client info structure */
	memset(&ci, 0, sizeof(ci));

	/* Minimal amount of information required is client identifier, so set it here */
	ci.client_id = CONFIG_CLIENT_ID_NAME;
	ci.client_user = CONFIG_CLIENT_USER_NAME;
	ci.client_pass = CONFIG_CLIENT_USER_PASSWORD;
	ci.keep_alive = 60; /* timeout */

	/* Initiate client and connect to server, if this fails immediately an error code is returned
   otherwise Mqtt_Connection_CB will be called with connection result after attempting
   to establish a connection with the server.
   For now MQTT version 3.1.1 is always used */
	if (mqtt_topics_initializated_flag == FALSE)
	{
		Mqtt_Init_All_Topics(device_topics);
		mqtt_topics_initializated_flag = TRUE;
	}
	PRINT_MESG_UART("Trying to connect:\n");
	err = mqtt_client_connect(&mqtt_client, &broker_ipaddr, MQTT_PORT, Mqtt_Connection_CB, (topics_info_type*)device_topics, &ci);
	/* Just print the result code if something goes wrong */
	if(err != ERR_OK)
	{
		PRINT_MESG_UART("mqtt_connect error: %d\n", err);
	}
}

static void Mqtt_Publish_Valid_Topics(topics_info_type* device_topics_print)
{
	char Topicinfomsg[MAX_LENGTH_TOPIC*(NUMBER_OF_TOPICS-1)+strlen("H743 Available topics are: \n")];
	uint8_t Topic_Counter= 0;
	Mqtt_topics Topics_available;
	err_t err;
	u8_t qos =0u; /* 0 1 or 2, see MQTT specification */
	u8_t retain = 1u; /**This info should be on broker*/

	/*Print valid topics*/
	sprintf(&Topicinfomsg[Topic_Counter],"H743 Available topics are: \n");
	Topic_Counter= strlen(Topicinfomsg);
	for (Topics_available = INFO+1u; Topics_available < NUMBER_OF_TOPICS; Topics_available++)
	{
		sprintf(&Topicinfomsg[Topic_Counter],"%s \n",device_topics_print[Topics_available].Input_topic);
		Topic_Counter= strlen(Topicinfomsg);
	}
	err = mqtt_publish(&mqtt_client,device_topics_print[INFO].Output_topic , Topicinfomsg, strlen(Topicinfomsg),
			qos, retain, Mqtt_Pub_Request_CB, NULL);
	if(err != ERR_OK)
	{
		PRINT_MESG_UART("Publish err: %d\n", err);
	}
}

static void Mqtt_Init_All_Topics(topics_info_type* device_topics_init)
{
	concatenate(mqtt_device_suscription,CONFIG_CLIENT_ID_NAME,INPUT,SUSCRIBE_TOPIC);
	Mqtt_Intialize_Topic(device_topics_init, INFO ,INFO_TOPIC,NULL, NULL,0);
	Mqtt_Intialize_Topic(device_topics_init, LEDS ,LEDS_TOPIC,mqtt_leds_handler, mqtt_leds_get_subtopic,1);
	Mqtt_Intialize_Topic(device_topics_init, ID ,ID_TOPIC,mqtt_id_handler, mqtt_id_get_subtopic,2);
	Mqtt_Intialize_Topic(device_topics_init, HEARTBEAT ,HB_TOPIC,HeartBeat_TopicHandler, HeartBeat_SubTopicHandler,0);
	Mqtt_Intialize_Topic(device_topics_init, BUTTON ,BUTTON_TOPIC,NULL, NULL,0);
}

/*Function to initialize topics/subtopics modules with minimum information
 * Follow the structure
 *
 * SubTopic Output Topic (Device)/Output/Topic_Name
 * SubTopic Input Topic (Device)/Input/Topic_Name
 * initialize topic handlers
 * initialize module QoS
 *
 * */
static void Mqtt_Intialize_Topic(topics_info_type* device_topics_init, Mqtt_topics Topic_To_Initialize ,
		const char* Topic_Name,void* TopicHandler, void * SubtopicHandler,uint8_t QoS )
{
	concatenate(device_topics_init[Topic_To_Initialize].Input_topic,CONFIG_CLIENT_ID_NAME,INPUT,Topic_Name);
	concatenate(device_topics_init[Topic_To_Initialize].Output_topic ,CONFIG_CLIENT_ID_NAME,OUTPUT,Topic_Name);
	device_topics_init[Topic_To_Initialize].Topic_valid = FALSE;

	if (TopicHandler != NULL )/*Initialize Topic Handler*/
	{
		device_topics_init[Topic_To_Initialize].Topic_Handler = TopicHandler;
	}
	else
	{
		device_topics_init[Topic_To_Initialize].Topic_Handler = Mqtt_Default_Topic_Handler;
	}
	if (SubtopicHandler != NULL )/*Initialize Subtopic Handler*/
	{
		device_topics_init[Topic_To_Initialize].Subtopics_handler = SubtopicHandler;
	}
	else
	{
		device_topics_init[Topic_To_Initialize].Subtopics_handler = Mqtt_Default_SubTopic_Handler;
	}
	if (QoS > 2)  /*Initialize QoS*/
	{
		device_topics_init[Topic_To_Initialize].Qos = 0;
	}else
	{
		device_topics_init[Topic_To_Initialize].Qos = QoS;
	}
}

/*Conection callback*/
static void Mqtt_Connection_CB(mqtt_client_t *client, void *arg, mqtt_connection_status_t status) {
	err_t err;
	if(status == MQTT_CONNECT_ACCEPTED)
	{
		PRINT_MESG_UART("Mqtt_Connection_CB: Successfully connected\n");
		/* Setup callback for incoming publish requests */
		mqtt_set_inpub_callback(client, Mqtt_Incoming_Publish_CB, Mqtt_Incoming_Data_CB, arg);
		/* Subscribe to Device specific topic with QoS level 2, call Mqtt_Sub_Request_CB with result */
		err = mqtt_subscribe(client, mqtt_device_suscription, 2u, Mqtt_Sub_Request_CB, arg);
		if(err != ERR_OK)
		{
			PRINT_MESG_UART("mqtt_subscribe return: %d\n", err);
		}
		Mqtt_Publish_Valid_Topics(device_topics);
	}
	else
	{
		PRINT_MESG_UART("Mqtt_Connection_CB: Disconnected, reason: %d\n", status);
		/* Its more nice to be connected, so try to reconnect */
		Mqtt_Do_Connect();
	}
}

static void Mqtt_Sub_Request_CB(void *arg, err_t result)
{
	/* Just print the result code here for simplicity,
	 * normal behavior would be to take some action if subscribe fails like
	 * notifying user, retry subscribe or disconnect from server */
	PRINT_MESG_UART("Subscribe result: %d\n", result);
}

/* Callback to incoming publish
 * Redirects the information to subtopic handler data
 * */
static void Mqtt_Incoming_Publish_CB(void *arg, const char *topic, u32_t tot_len)
{
	topics_info_type* Incoming_publish = arg;
	Mqtt_topics Topic_received;
	PRINT_MESG_UART("Incoming publish at topic %s with total length %u\n", topic, (unsigned int)tot_len);
	for (Topic_received = 0; Topic_received< NUMBER_OF_TOPICS; Topic_received++)
	{
		if(strncmp(topic,Incoming_publish[Topic_received].Input_topic ,strlen(Incoming_publish[Topic_received].Input_topic)) == 0)
		{
			Incoming_publish[Topic_received].Topic_valid = TRUE;
			Incoming_publish[Topic_received].Subtopics =
					Incoming_publish[Topic_received].Subtopics_handler(&(topic[strlen(Incoming_publish[Topic_received].Input_topic)]));
			break;
		}
	}
	if (TRUE != Incoming_publish[Topic_received].Topic_valid)
	{
		/* For all other topics */
		PRINT_MESG_UART("Invalid Topic\n");
		mqtt_publish_cust("", "Wrong topic",INFO);
		Mqtt_Publish_Valid_Topics(device_topics);
	}
}



/*Callback to incoming data
 * Redirects the information to subtopic handler information
 * */
static void Mqtt_Incoming_Data_CB(void *arg, const u8_t *data, u16_t len, u8_t flags) {
	uint8_t msg[MQTT_VAR_HEADER_BUFFER_LEN+1u];
	topics_info_type* Incoming_data = arg;
	Mqtt_topics Topic_received;
	MEMCPY(msg,data,len);
	msg[len]=0;
	PRINT_MESG_UART("Incoming publish payload with length %d, flags %u\n", len, (unsigned int)flags);
	PRINT_MESG_UART("Data: %s, \n\n", msg );

	/* Last fragment of payload received (or whole part if payload fits receive buffer
	See MQTT_VAR_HEADER_BUFFER_LEN) */
	if(flags & MQTT_DATA_FLAG_LAST)
	{
		for (Topic_received = 0; Topic_received< NUMBER_OF_TOPICS; Topic_received++)
		{
			if(Incoming_data[Topic_received].Topic_valid == TRUE)
			{
				Incoming_data[Topic_received].Topic_valid = FALSE;
				if (Incoming_data[Topic_received].Subtopics != NULL)
				{
					// Call specific topic function handler
					Incoming_data[Topic_received].Topic_Handler((char*)data,len, // Data and len
							Incoming_data[Topic_received].Subtopics); // Subtopic structure handler

				}
				else
				{
					//internal error
				}
				break;
			}
		}
	}
	else
	{
		/* Handle fragmented payload, store in buffer, write to file or whatever */

	}

}


/* Called when publish is complete either with success or failure */
static void Mqtt_Pub_Request_CB(void *arg, err_t result)
{
	if(result != ERR_OK)
	{
		PRINT_MESG_UART("Publish result: %d\n", result);
	}
}

/*Default topic handler is no topic handler if declared on initialization*/
static void Mqtt_Default_Topic_Handler(const char * data, u16_t len , void* subtopics_void){
	PRINT_MESG_UART("Default Topic Handler \n");
}


/*Default subtopic handler is no sub topic handler if declared on initialization*/
static void* Mqtt_Default_SubTopic_Handler(const char *subtopic){
	PRINT_MESG_UART("Default Sub_Topic Handler \n");
	return NULL;
}



