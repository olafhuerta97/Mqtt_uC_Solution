/*
 * mqtt_cust.c
 *
 *  Created on: Oct 24, 2021
 *      Author: Olaf
 */
/*File Includes*/

#include "MQTT_leds.h"
#include "MQTT_button.h"
#include "MQTT_sw_id.h"
#include "MQTT_heartbeat.h"
#include "MQTT_main.h"
#include "MQTT_api.h"

#define WELCOMEMESSAGE          " Hi, Device online... \n"
#define AVAILABLETOPICS         "Available topics are: \n"
#define OUTPUT     				"/Output"
#define INPUT     				"/Input"
#define SUSCRIBE_TOPIC   		"/#"
#define MOREINFOMSG             "For more information please send"
#define MOREINFOMSG2            "/Input/(desired_topic)/Info with data GET or empty"
#define FREE_TIMER_2 		    TIM5

typedef MQTTClient mqtt_client_t;
typedef uint8_t mqtt_connection_status_t;
typedef uint8_t mqtt_connect_client_info_t;
#define MQTT_PORT 8090
#define MQTT_OUTPUT_RINGBUF_SIZE 200

static const char *topics_array_names[Number_Of_Topics] =
{
	"/Info",
	"/Leds",
	"/Id",
	"/Button",
	"/HeartBeat"
};

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
static mqtt_topics_info_t mqtt_topics_info[Number_Of_Topics];
static u8_t mqtt_topics_initializated_flag = FALSE;

/*Local Static Function Declarations*/

/*Publish Available Topics on device*/
static void Mqtt_Publish_Valid_Topics(mqtt_topics_info_t* device_topics_print, u8_t is_welcome_msg_flag);

/*Initialize Topics*/
static void Mqtt_Init_All_Topics(mqtt_topics_info_t* device_topics_init);
static void Mqtt_Intialize_Topic(mqtt_topics_info_t* device_topics_init, Mqtt_Topics_t Topic_To_Initialize,
		const char* Topic_Name,void* TopicHandler, void * SubtopicHandler,u8_t QoS );

/*Callback for events*/
void Mqtt_Sub_Request_CB(void *arg, err_t result) ;
static void Mqtt_Incoming_Publish_CB(void *arg, const char *topic, u32_t tot_len);
static void Mqtt_Incoming_Data_CB(void *arg, const u8_t *data, u16_t len, u8_t flags);
static void Mqtt_Pub_Request_CB(void *arg, err_t result);
void Mqtt_Incoming_Publish_CB_handler(MessageData* data);
/*Default handlers Declaration*/
static void* Mqtt_Default_SubTopics_Handler(const char *subtopic);
static void Mqtt_Default_Data_Handler(const char * data, u16_t len , void* subtopics_void);

/*Function call every time the button is pressed*/
void Mqtt_Ext_Int_ISR_Handler(u16_t GPIO_Pin)
{
	if (GPIO_Pin == GPIO_PIN_13)
	{
		Button_ISR_Handler();
	}
}

/*Function call by ISR every time a timer is expired*/
void Mqtt_Timer_ISR_Handler(TIM_HandleTypeDef *htim)
{
	if (htim->Instance == FREE_TIMER_2 ){
		Leds_Timer_Handler(htim);
		Hb_Timer_Handler(htim);
	}
}

/*Function to publish messages from subtopics*/
void Mqtt_Publish_Cust(const char *subtopic, const char *pub_payload,Mqtt_Topics_t sender) {
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
			qos, retain, Mqtt_Pub_Request_CB, (Mqtt_Topics_t*)&sender);
	if(err != ERR_OK)
	{
		PRINT_MESG_UART("Publish err: %d\n", err);
	}
}


void MQTTYield_Cust(void){
	MQTTYield(&mqtt_client, 5000);
}

/**
 * Mqtt_Do_Connect
 *
 * brief. Connect to the Mqtt Broker and initialize topics.
 */
u8_t Mqtt_Do_Connect(void) {
	char willmessage[50];
	MQTTPacket_connectData ci = MQTTPacket_connectData_initializer;
	err_t err;
	device_config_t device_config;

	device_config.HostName = "colinashdlv.dyndns.org";
	device_config.HostPort = "8090";

	/*Prepare the will message to be display in broker when client is disconnected*/
	sprintf(willmessage,"%s is offline", CONFIG_CLIENT_ID_NAME);

	/*If is the first time that connection is called then initialize the
	 * topics and callbacks*/
	if (mqtt_topics_initializated_flag == FALSE)
	{
		Mqtt_Init_All_Topics(mqtt_topics_info);
		mqtt_topics_initializated_flag = TRUE;
	}

	/* Setup an empty client info structure */
	/* Minimal amount of information required is client identifier, so set it here */
	/* Also set the username and password to connect to broker*/
	//ci.client_id = CONFIG_CLIENT_ID_NAME;
	ci.clientID.cstring = CONFIG_CLIENT_ID_NAME;
	ci.username.cstring = CONFIG_CLIENT_USER_NAME;
	ci.password.cstring = CONFIG_CLIENT_USER_PASSWORD;
	ci.keepAliveInterval = 30u; /* keep alive functionality 30 seconds since module is not sleeping*/
	/*Set the will message for registration*/
//	ci.will = willmessage;
	/*QoS is not that important in this message for information purposues*/
//	ci.willFlag=2u;
	/*It should be retained*/
//	ci.will_retain = 1u;
	/*Publishing will to info topic*/
//	ci.will_topic = mqtt_topics_info[Info].Output_topic;

	/* Initiate client and connect to server, if this fails immediately an error code is returned
   otherwise Mqtt_Connection_CB will be called with connection result after attempting
   to establish a connection with the server.
   For now MQTT version 3.1.1 is always used */
	PRINT_MESG_UART("Trying to connect:\n");
	err = mqtt_client_connect(&mqtt_client, &device_config, MQTT_PORT,
			NULL, (mqtt_topics_info_t*)mqtt_topics_info, &ci);
	/* Just print the result code if something goes wrong */
	if(err != ERR_OK)
	{
		/*For now if not success we will just print and error*/
		PRINT_MESG_UART("mqtt_connect error: %d\n", err);
	}
	else
	{
		err = mqtt_client_suscribe(&mqtt_client,"#", 0, Mqtt_Incoming_Publish_CB_handler);
	/* Just print the result code if something goes wrong */
		if(err != ERR_OK)
		{
			/*For now if not success we will just print and error*/
			PRINT_MESG_UART("mqtt_connect error: %d\n", err);
		}else{
			Mqtt_Publish_Valid_Topics(mqtt_topics_info, TRUE);
		}
	}
	return err;
}

/** Message handler
 *
 *  Note: No context handle is passed by the callback. Must rely on static variables.
 *        TODO: Maybe store couples of hander/contextHanders so that the context could
 *              be retrieved from the handler address. */
void Mqtt_Incoming_Publish_CB_handler(MessageData* data)
{
	Mqtt_Incoming_Publish_CB(mqtt_topics_info, data->topicName->cstring, data->message->payloadlen);
	Mqtt_Incoming_Data_CB(mqtt_topics_info, data->message->payload, data->message->payloadlen, 1);
}


void Mqtt_Publish_Subtopic_Info(const commands_info_struct_t *topic_info,uint8_t number_of_commands,
		Mqtt_Topics_t sender)
{
	char info_msg[100];
	u16_t message_char_counter= 0;
	uint8_t topics_available;

	/*Print valid topics*/
	sprintf(&info_msg[message_char_counter],AVAILABLETOPICS);
	for (topics_available = 0u; topics_available < number_of_commands; topics_available++)
	{
		message_char_counter= strlen(info_msg);
		if((message_char_counter+strlen(topic_info[topics_available].command_name))
				> MQTT_OUTPUT_RINGBUF_SIZE)
		{
			PRINT_MESG_UART("Array not big enough for printing all topics\n");
			return;
		}
		sprintf(&info_msg[message_char_counter],"*%s%s \n With option(s)\n",
				mqtt_topics_info[sender].Input_topic,topic_info[topics_available].command_name);

		for (u8_t number_of_commands = 0; number_of_commands < topic_info[topics_available].number_commands;
				number_of_commands++)
		{
			message_char_counter= strlen(info_msg);
			sprintf(&info_msg[message_char_counter],"  -%s \n",
							topic_info[topics_available].command_options[number_of_commands]);
		}
	}
	message_char_counter= strlen(info_msg);
	PRINT_MESG_UART("%d\n",message_char_counter);

	Mqtt_Publish_Cust("/Info", info_msg , sender);

}

static void Mqtt_Publish_Valid_Topics(mqtt_topics_info_t* device_topics_print, u8_t is_welcome_msg_flag)
{
	char Topicinfomsg[200];
	u8_t message_char_counter= 0;
	Mqtt_Topics_t topics_available;
	err_t err;
	u8_t qos =0u; /* 0 1 or 2, see MQTT specification */
	u8_t retain = 1u; /**This info should be on broker*/

	/*Print valid topics*/
	if (TRUE == is_welcome_msg_flag){
		sprintf(&Topicinfomsg[message_char_counter],CONFIG_CLIENT_ID_NAME);
		message_char_counter= strlen(Topicinfomsg);
		sprintf(&Topicinfomsg[message_char_counter],WELCOMEMESSAGE);
		message_char_counter= strlen(Topicinfomsg);
	}
	sprintf(&Topicinfomsg[message_char_counter],AVAILABLETOPICS);
	for (topics_available = 0u; topics_available < Number_Of_Topics; topics_available++)
	{
		message_char_counter= strlen(Topicinfomsg);
		if((message_char_counter+strlen(device_topics_print[topics_available].Input_topic))
				> 200)
		{
			PRINT_MESG_UART("Array not big enough for printing all topics\n");
			return;
		}
		sprintf(&Topicinfomsg[message_char_counter],"%s \n",device_topics_print[topics_available].Input_topic);
	}
	message_char_counter= strlen(Topicinfomsg);

	if((message_char_counter+strlen(MOREINFOMSG)+ strlen(MOREINFOMSG2) + strlen(CONFIG_CLIENT_ID_NAME))
			> 200)
	{
		PRINT_MESG_UART("Array not big enough for printing all topics\n");
		return;
	}

	sprintf(&Topicinfomsg[message_char_counter],"%s %s%s",MOREINFOMSG, CONFIG_CLIENT_ID_NAME,MOREINFOMSG2);

	err = mqtt_publish(&mqtt_client,device_topics_print[Info].Output_topic , Topicinfomsg, strlen(Topicinfomsg),
			qos, retain, Mqtt_Pub_Request_CB, NULL);
	if(err != ERR_OK)
	{
		PRINT_MESG_UART("Publish err: %d\n", err);
	}
}

static void Mqtt_Init_All_Topics(mqtt_topics_info_t* device_topics_init)
{
	Utils_Concatenate(mqtt_device_suscription,CONFIG_CLIENT_ID_NAME,INPUT,SUSCRIBE_TOPIC);
	Mqtt_Intialize_Topic(device_topics_init, Info ,topics_array_names[Info],NULL, NULL,0);
	Mqtt_Intialize_Topic(device_topics_init, Leds ,topics_array_names[Leds],Leds_Data_Handler, Leds_Subtopics_Handler,1);
	Mqtt_Intialize_Topic(device_topics_init, Id ,topics_array_names[Id],Id_Data_Handler, Id_Subtopics_Handler,2);
	Mqtt_Intialize_Topic(device_topics_init, Heartbeat ,topics_array_names[Heartbeat],Hb_Data_Handler, Hb_Subtopics_Handler,0);
	Mqtt_Intialize_Topic(device_topics_init, Button ,topics_array_names[Button],NULL, NULL,0);
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
static void Mqtt_Intialize_Topic(mqtt_topics_info_t* device_topics_init, Mqtt_Topics_t Topic_To_Initialize,
		const char* Topic_Name,void* TopicHandler, void * SubtopicHandler,u8_t QoS )
{
	Utils_Concatenate(device_topics_init[Topic_To_Initialize].Input_topic
			,CONFIG_CLIENT_ID_NAME,INPUT,Topic_Name);
	Utils_Concatenate(device_topics_init[Topic_To_Initialize].Output_topic
			,CONFIG_CLIENT_ID_NAME,OUTPUT,Topic_Name);
	device_topics_init[Topic_To_Initialize].Topic_valid = FALSE;

	if (TopicHandler != NULL )/*Initialize Topic Handler*/
	{
		device_topics_init[Topic_To_Initialize].Topic_Handler = TopicHandler;
	}
	else
	{
		device_topics_init[Topic_To_Initialize].Topic_Handler =
				Mqtt_Default_Data_Handler;
	}
	if (SubtopicHandler != NULL )/*Initialize Subtopic Handler*/
	{
		device_topics_init[Topic_To_Initialize].Subtopics_handler = SubtopicHandler;
	}
	else
	{
		device_topics_init[Topic_To_Initialize].Subtopics_handler =
				Mqtt_Default_SubTopics_Handler;
	}
	if (QoS > 2)  /*Initialize QoS*/
	{
		device_topics_init[Topic_To_Initialize].Qos = 0;
	}else
	{
		device_topics_init[Topic_To_Initialize].Qos = QoS;
	}
}


void Mqtt_Sub_Request_CB(void *arg, err_t result)
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
	Mqtt_Topics_t Topic_received;
	u8_t topic_valid_flag = FALSE;
	PRINT_MESG_UART("Incoming publish at topic %s with total length %u\n", topic, (unsigned int)tot_len);
	for (Topic_received = 0; Topic_received< Number_Of_Topics; ++Topic_received)
	{
		if(strncmp(topic,Incoming_publish[Topic_received].Input_topic ,
				strlen(Incoming_publish[Topic_received].Input_topic)) == 0)
		{
			Incoming_publish[Topic_received].Topic_valid = TRUE;
			Incoming_publish[Topic_received].Subtopics = Incoming_publish[Topic_received].Subtopics_handler
					(&(topic[strlen(Incoming_publish[Topic_received].Input_topic)]));
			topic_valid_flag = TRUE;
			break;
		}
	}
	if (FALSE == topic_valid_flag)
	{
		/* For all other topics */
		PRINT_MESG_UART("Invalid Topic\n");
		Mqtt_Publish_Cust("", "Wrong topic",Info);
	}
}



/*Callback to incoming data
 * Redirects the information to subtopic handler information
 * */
static void Mqtt_Incoming_Data_CB(void *arg, const u8_t *data, u16_t len, u8_t flags) {
	//u8_t msg[MQTT_VAR_HEADER_BUFFER_LEN+1u];
	mqtt_topics_info_t* Incoming_data = arg;
	Mqtt_Topics_t Topic_received;
	//MEMCPY(msg,data,len);
	//msg[len]=0;
	//PRINT_MESG_UART("Incoming publish payload with length %d, flags %u\n", len, (unsigned int)flags);
	//PRINT_MESG_UART("Data: %s, \n\n", msg );

	/* Last fragment of payload received (or whole part if payload fits receive buffer
	See MQTT_VAR_HEADER_BUFFER_LEN) */
	if(flags & 1)
	{
		for (Topic_received = 0; Topic_received< Number_Of_Topics; Topic_received++)
		{
			if(Incoming_data[Topic_received].Topic_valid == TRUE)
			{
				Incoming_data[Topic_received].Topic_valid = FALSE;

				// Call specific topic function handler
				Incoming_data[Topic_received].Topic_Handler((char*)data,len, // Data and len
							Incoming_data[Topic_received].Subtopics); // Subtopic structure handler

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

/*Default subtopic handler is no sub topic handler if declared on initialization*/
static void* Mqtt_Default_SubTopics_Handler(const char *subtopic){
	PRINT_MESG_UART("Default Sub_Topic Handler \n");
	return NULL;
}

/*Default topic handler if no topic handler if declared on initialization*/
static void Mqtt_Default_Data_Handler(const char * data, u16_t len , void* subtopics_void){
	PRINT_MESG_UART("Default Topic Handler \n");
	if (strncmp(data, "RESET",strlen(data)) == 0 && len == strlen(data))
	{
		PRINT_MESG_UART("Resetting\n");
		HAL_NVIC_SystemReset();
	}
}






