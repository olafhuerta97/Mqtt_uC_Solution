/*
 * mqtt_cust.c
 *
 *  Created on: Oct 24, 2021
 *      Author: Olaf
 */
/*File Includes*/
#include "mqtt.h"
#include "lwip/apps/mqtt_priv.h"
#include "mqtt_opts.h"
#include "MQTT_leds.h"
#include "MQTT_button.h"
#include "MQTT_sw_id.h"
#include "MQTT_heartbeat.h"
#include "MQTT_main.h"

/*
 * THIS PROJECT IS A REMINDER TO MYSELF TO NOT USE DINAMIC MEMORY IN EMBEDDED SYSTEMS
 *
 * WHEN THIS WAS ORIGINALLY MADE ALLOCATING MEMORY IT WAS GETTING CORRUPTED.
 * */

#define OUTPUT     				"/Output"
#define INPUT     				"/Input"
#define SUSCRIBE_TOPIC   		"/#"
#define INFO_TOPIC 				"/Info"
#define LEDS_TOPIC 				"/Leds"
#define ID_TOPIC 				"/Id"
#define BUTTON_TOPIC 			"/Button"
#define HB_TOPIC 			    "/HeartBeat"

#define FREE_TIMER_1			TIM2
#define HB_TIMER 			    TIM3
#define LEDS_TIMER 	     	    TIM4
#define FREE_TIMER_2 		    TIM5


/*FileStructures*/
typedef struct mqtt_topics_info_s{
	u8_t    Topic_valid;
	u8_t 	Qos;
	void*   Subtopics;
	char    Output_topic[MAX_LENGTH_TOPIC];
	char    Input_topic[MAX_LENGTH_TOPIC];
	void    (*(*Subtopics_handler)(const char* ));
	void    (*Topic_Handler)(const char* ,u16_t, void*);
}mqtt_topics_info_t;

/*Local Static Variables and Structures*/
static char mqtt_device_suscription[MAX_LENGTH_TOPIC];
static mqtt_client_t mqtt_client;
static mqtt_topics_info_t mqtt_topics_info[NUMBER_OF_TOPICS];
static u8_t mqtt_topics_initializated_flag = FALSE;

/*Local Static Function Declarations*/

/*Publish Available Topics on device*/
static void Mqtt_Publish_Valid_Topics(mqtt_topics_info_t* device_topics_print);

/*Initialize Topics*/
static void Mqtt_Init_All_Topics(mqtt_topics_info_t* device_topics_init);
static void Mqtt_Intialize_Topic(mqtt_topics_info_t* device_topics_init, Mqtt_topics Topic_To_Initialize ,
		const char* Topic_Name,void* TopicHandler, void * SubtopicHandler,u8_t QoS );

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
void Mqtt_Ext_Int_ISR_Handler(u16_t GPIO_Pin)
{
	if (GPIO_Pin == Button_Pin_Pin)
	{
		button_handler_isr();
	}
}

/*Function call by ISR every time a timer is expired*/
void Mqtt_Timer_ISR_Handler(TIM_HandleTypeDef *htim)
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
void Mqtt_Publish_Cust(const char *subtopic, const char *pub_payload,Mqtt_topics sender) {
	err_t err;
	u8_t retain = 0u;
	char topic[MAX_LENGTH_TOPIC];
	size_t topic_len = strlen(mqtt_topics_info[sender].Output_topic);
	size_t subtopic_len = strlen(subtopic);
	u8_t qos = mqtt_topics_info[sender].Qos; /* 0 1 or 2, see MQTT specification */
	/*Calculate topic publish size and if most return*/
	if((topic_len + subtopic_len)>MAX_LENGTH_TOPIC)
	{
		PRINT_MESG_UART("Trying to publish a topic with max length \n");
		return;
	}
	memcpy(topic, mqtt_topics_info[sender].Output_topic, topic_len);
	memcpy(topic + topic_len, subtopic, subtopic_len+1);
	err = mqtt_publish(&mqtt_client, topic, pub_payload, strlen(pub_payload),
			qos, retain, Mqtt_Pub_Request_CB, (Mqtt_topics*)&sender);
	if(err != ERR_OK)
	{
		PRINT_MESG_UART("Publish err: %d\n", err);
	}
}


u8_t Mqtt_Do_Connect(void) {
	char willmessage[50];
	ip4_addr_t broker_ipaddr;
	struct mqtt_connect_client_info_t ci;
	err_t err;
	sprintf(willmessage,"%s is offline", CONFIG_CLIENT_ID_NAME);

	IP4_ADDR(&broker_ipaddr, configIP_MQTT_ADDR0, configIP_MQTT_ADDR1, configIP_MQTT_ADDR2,
			configIP_MQTT_ADDR3);


	/* Initiate client and connect to server, if this fails immediately an error code is returned
   otherwise Mqtt_Connection_CB will be called with connection result after attempting
   to establish a connection with the server.
   For now MQTT version 3.1.1 is always used */
	if (mqtt_topics_initializated_flag == FALSE)
	{
		Mqtt_Init_All_Topics(mqtt_topics_info);
		mqtt_topics_initializated_flag = TRUE;
	}

	/* Setup an empty client info structure */
	memset(&ci, 0, sizeof(ci));
	/* Minimal amount of information required is client identifier, so set it here */
	ci.client_id = CONFIG_CLIENT_ID_NAME;
	ci.client_user = CONFIG_CLIENT_USER_NAME;
	ci.client_pass = CONFIG_CLIENT_USER_PASSWORD;
	ci.keep_alive = 30u; /* keep alive functionality 30 seconds since module is not sleeping*/
	ci.will_msg = willmessage;
	ci.will_qos=0u;
	ci.will_retain = 1u;
	ci.will_topic = mqtt_topics_info[INFO].Output_topic;

	PRINT_MESG_UART("Trying to connect:\n");
	err = mqtt_client_connect(&mqtt_client, &broker_ipaddr, MQTT_PORT, Mqtt_Connection_CB, (mqtt_topics_info_t*)mqtt_topics_info, &ci);
	/* Just print the result code if something goes wrong */
	if(err != ERR_OK)
	{
		PRINT_MESG_UART("mqtt_connect error: %d\n", err);
	}
	return err;
}

static void Mqtt_Publish_Valid_Topics(mqtt_topics_info_t* device_topics_print)
{
	char Topicinfomsg[MAX_LENGTH_TOPIC*(NUMBER_OF_TOPICS-1)+strlen("H743 Available topics are: \n")];
	u8_t Topic_Counter= 0;
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

static void Mqtt_Init_All_Topics(mqtt_topics_info_t* device_topics_init)
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
static void Mqtt_Intialize_Topic(mqtt_topics_info_t* device_topics_init, Mqtt_topics Topic_To_Initialize ,
		const char* Topic_Name,void* TopicHandler, void * SubtopicHandler,u8_t QoS )
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
		Mqtt_Publish_Valid_Topics(mqtt_topics_info);
	}
	else
	{
		PRINT_MESG_UART("Mqtt_Connection_CB: Disconnected, reason: %d\n", status);
		/* Its more nice to be connected, so try to reconnect */
		(void)Mqtt_Do_Connect();
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
	mqtt_topics_info_t* Incoming_publish = arg;
	Mqtt_topics Topic_received;
	//PRINT_MESG_UART("Incoming publish at topic %s with total length %u\n", topic, (unsigned int)tot_len);
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
		Mqtt_Publish_Cust("", "Wrong topic",INFO);
		Mqtt_Publish_Valid_Topics(mqtt_topics_info);
	}
}



/*Callback to incoming data
 * Redirects the information to subtopic handler information
 * */
static void Mqtt_Incoming_Data_CB(void *arg, const u8_t *data, u16_t len, u8_t flags) {
	u8_t msg[MQTT_VAR_HEADER_BUFFER_LEN+1u];
	mqtt_topics_info_t* Incoming_data = arg;
	Mqtt_topics Topic_received;
	MEMCPY(msg,data,len);
	msg[len]=0;
	//PRINT_MESG_UART("Incoming publish payload with length %d, flags %u\n", len, (unsigned int)flags);
	//PRINT_MESG_UART("Data: %s, \n\n", msg );

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



