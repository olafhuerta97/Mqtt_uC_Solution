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
#include "mqtt_cust.h"

/*
 * THIS PROJECT IS A REMINDER TO MYSELF TO NOT USE DINAMIC MEMORY IN EMBEDDED SYSTEMS
 *
 * WHEN THIS WAS ORIGINALLY MADE ALLOCATING MEMORY IT WAS GETTING CORRUPTED.
 * */

static char Device_suscription[MAX_LENGTH_TOPIC];
static mqtt_client_t mqtt_client;
static topics_info_type device_topics[NUMBER_OF_TOPICS];
static uint8_t Topics_initializated = FALSE;


static void mqtt_connection_cb(mqtt_client_t *client, void *arg, mqtt_connection_status_t status);
static void mqtt_sub_request_cb(void *arg, err_t result) ;
static void mqtt_incoming_data_cb(void *arg, const u8_t *data, u16_t len, u8_t flags);
static void mqtt_incoming_publish_cb(void *arg, const char *topic, u32_t tot_len);
static void mqtt_pub_request_cb(void *arg, err_t result);
static void Initialize_Topic(topics_info_type* device_topics_init, Mqtt_topics Topic_To_Initialize ,const char* Topic_Name,
		void* TopicHandler, void * SubtopicHandler );
static void Default_TopicHandler(const char * data, u16_t len , void* subtopics_void);
static void* Default_SubTopicHandler(const char *subtopic);


void MQTT_Cust_HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin){
	if (GPIO_Pin == Button_Pin){
		button_handler_isr();
	}
}

void MQTT_PeriodElapsedTim(TIM_HandleTypeDef *htim){
	if(htim->Instance == LEDS_TIMER )
	{

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

void mqtt_publish_cust(const char *subtopic, const char *pub_payload,Mqtt_topics sender) {
	err_t err;
	char topic[MAX_LENGTH_TOPIC];
	size_t topic_len = strlen(device_topics[sender].Output_topic);
	size_t subtopic_len = strlen(subtopic);
	if((topic_len + subtopic_len)>MAX_LENGTH_TOPIC){
		PRINT_MESG_UART("Trying to publish a topic with max length \n");
		return;
	}

	u8_t qos = 2u; /* 0 1 or 2, see MQTT specification */
	u8_t retain = 0;
	memcpy(topic, device_topics[sender].Output_topic, topic_len);
	memcpy(topic + topic_len, subtopic, subtopic_len+1);
	err = mqtt_publish(&mqtt_client, topic, pub_payload, strlen(pub_payload), qos, retain, mqtt_pub_request_cb, (Mqtt_topics*)&sender);
	if(err != ERR_OK)
	{
		PRINT_MESG_UART("Publish err: %d\n", err);
	}
}

static void Init_Topics(topics_info_type* device_topics_init){
	concatenate(Device_suscription,CONFIG_CLIENT_ID_NAME,INPUT,SUSCRIBE_TOPIC);
	Initialize_Topic(device_topics_init, LEDS ,LEDS_TOPIC,mqtt_leds_handler, mqtt_leds_get_subtopic);
	Initialize_Topic(device_topics_init, ID ,ID_TOPIC,mqtt_id_handler, mqtt_id_get_subtopic);
	Initialize_Topic(device_topics_init, HEARTBEAT ,HB_TOPIC,HeartBeat_TopicHandler, HeartBeat_SubTopicHandler);
	Initialize_Topic(device_topics_init, BUTTON ,BUTTON_TOPIC,NULL, NULL);
}


void mqtt_do_connect(void) {

	ip4_addr_t broker_ipaddr;
	struct mqtt_connect_client_info_t ci;
	err_t err;

	IP4_ADDR(&broker_ipaddr, configIP_MQTT_ADDR0, configIP_MQTT_ADDR1, configIP_MQTT_ADDR2, configIP_MQTT_ADDR3);

	/* Setup an empty client info structure */
	memset(&ci, 0, sizeof(ci));

	/* Minimal amount of information required is client identifier, so set it here */
	ci.client_id = CONFIG_CLIENT_ID_NAME;
	ci.client_user = CONFIG_CLIENT_USER_NAME;
	ci.client_pass = CONFIG_CLIENT_USER_PASSWORD;
	ci.keep_alive = 60; /* timeout */

	/* Initiate client and connect to server, if this fails immediately an error code is returned
   otherwise mqtt_connection_cb will be called with connection result after attempting
   to establish a connection with the server.
   For now MQTT version 3.1.1 is always used */
	if (Topics_initializated == FALSE){
	Init_Topics(device_topics);
	Topics_initializated = TRUE;
	}
	err = mqtt_client_connect(&mqtt_client, &broker_ipaddr, MQTT_PORT, mqtt_connection_cb, (topics_info_type*)device_topics, &ci);
	/* For now just print the result code if something goes wrong */
	if(err != ERR_OK)
	{
		PRINT_MESG_UART("mqtt_connect error: %d\n", err);
	}
}


static void mqtt_connection_cb(mqtt_client_t *client, void *arg, mqtt_connection_status_t status) {
	err_t err;
	if(status == MQTT_CONNECT_ACCEPTED) {
		PRINT_MESG_UART("mqtt_connection_cb: Successfully connected\n");

		/* Setup callback for incoming publish requests */
		mqtt_set_inpub_callback(client, mqtt_incoming_publish_cb, mqtt_incoming_data_cb, arg);

		/* Subscribe to a topic named "subtopic" with QoS level 2, call mqtt_sub_request_cb with result */
		err = mqtt_subscribe(client, Device_suscription, 2u, mqtt_sub_request_cb, arg);
		if(err != ERR_OK)
		{
			PRINT_MESG_UART("mqtt_subscribe return: %d\n", err);
		}
	} else
	{
		PRINT_MESG_UART("mqtt_connection_cb: Disconnected, reason: %d\n", status);
		/* Its more nice to be connected, so try to reconnect */
		mqtt_do_connect();
	}
}


/* The idea is to demultiplex topic and create some reference to be used in data callbacks
 If RAM and CPU budget allows it, the easiest implementation might be to just take a copy of
 the topic string and use it in mqtt_incoming_data_cb
 */
static void mqtt_incoming_publish_cb(void *arg, const char *topic, u32_t tot_len) {
	topics_info_type* Incoming_publish = arg;
	Mqtt_topics Topic_received;
	PRINT_MESG_UART("Incoming publish at topic %s with total length %u\n", topic, (unsigned int)tot_len);
	for (Topic_received = 0; Topic_received<= NUMBER_OF_TOPICS; Topic_received++)
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
	}
}

static void mqtt_incoming_data_cb(void *arg, const u8_t *data, u16_t len, u8_t flags) {
	uint8_t msg[MQTT_VAR_HEADER_BUFFER_LEN+1u];
	topics_info_type* Incoming_data = arg;
	Mqtt_topics Topic_received;
	MEMCPY(msg,data,len);
	msg[len]=0;

	PRINT_MESG_UART("Incoming publish payload with length %d, flags %u\n", len, (unsigned int)flags);
	PRINT_MESG_UART("Data: %s, flags %u\n\n", msg);
	/* Last fragment of payload received (or whole part if payload fits receive buffer
	See MQTT_VAR_HEADER_BUFFER_LEN) */
	if(flags & MQTT_DATA_FLAG_LAST) {

		for (Topic_received = 0; Topic_received< NUMBER_OF_TOPICS; Topic_received++)
		{
			if(Incoming_data[Topic_received].Topic_valid == TRUE)
			{
				Incoming_data[Topic_received].Topic_valid = FALSE;
				Incoming_data[Topic_received].Topic_Handler(( char *)data,len,Incoming_data[Topic_received].Subtopics);
				break;
			}
		}
	} else {
		/* Handle fragmented payload, store in buffer, write to file or whatever */

	}

}

static void mqtt_sub_request_cb(void *arg, err_t result) {
 /* Just print the result code here for simplicity,
 normal behavior would be to take some action if subscribe fails like
 notifying user, retry subscribe or disconnect from server */
 PRINT_MESG_UART("Subscribe result: %d\n", result);
}

/* Called when publish is complete either with success or failure */
static void mqtt_pub_request_cb(void *arg, err_t result) {
 if(result != ERR_OK) {
   PRINT_MESG_UART("Publish result: %d\n", result);
 }
}

static void Initialize_Topic(topics_info_type* device_topics_init, Mqtt_topics Topic_To_Initialize ,const char* Topic_Name,
		void* TopicHandler, void * SubtopicHandler ){
	concatenate(device_topics_init[Topic_To_Initialize].Input_topic,CONFIG_CLIENT_ID_NAME,INPUT,Topic_Name);
	concatenate(device_topics_init[Topic_To_Initialize].Output_topic ,CONFIG_CLIENT_ID_NAME,OUTPUT,Topic_Name);
	device_topics_init[Topic_To_Initialize].Topic_valid = FALSE;

	if (TopicHandler != NULL )
	{
		device_topics_init[Topic_To_Initialize].Topic_Handler = TopicHandler;
	}else
	{
		device_topics_init[Topic_To_Initialize].Topic_Handler = Default_TopicHandler;
	}
	if (SubtopicHandler != NULL )
	{
		device_topics_init[Topic_To_Initialize].Subtopics_handler = SubtopicHandler;
	}else
	{
		device_topics_init[Topic_To_Initialize].Subtopics_handler = Default_SubTopicHandler;
	}

}

static void Default_TopicHandler(const char * data, u16_t len , void* subtopics_void){
	PRINT_MESG_UART("Default Topic Handler \n");
}

static void* Default_SubTopicHandler(const char *subtopic){
	PRINT_MESG_UART("Default Sub_Topic Handler \n");
	return NULL;
}



