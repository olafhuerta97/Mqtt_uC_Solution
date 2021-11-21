/*
 ============================================================================
 Name        : Main.c
 Author      : Olaf Huerta
 Version     : 0.0.1
 Copyright   : Your copyright notice
 ============================================================================
 */
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>

#include "my_timer.h"
#include "../mqttlib/mosquitto.h"
#include "../mqttlib/mosquitto_internal.h"

#define mqtt_host "192.168.0.160"
#define mqtt_port 1883
static struct mosquitto *mosq;
static char TopicBBB[] = "Mac/greetings";
static char PayloadBBB[] = "Hello from Mac";
size_t timer1;

/*mosquitto_publish(struct mosquitto *mosq, int *mid, const char *topic,
 *  int payloadlen, const void *payload, int qos, bool retain)
*/
void time_handler1(size_t timer_id, void * user_data)
{
    printf("Publishing hi from Mac_M1.\n" );
    mosquitto_publish(mosq,NULL,TopicBBB,sizeof(PayloadBBB),PayloadBBB,2,0);
}

static int run = 1;

void handle_signal(int s)
{
    finalize();
	printf("Signal Handler signal:%d\n", s);
	run--;
	//mosquitto_destroy(mosq);
}

void connect_callback(struct mosquitto *mosq, void *obj, int result)
{
	//printf("connect callback, rc=%d\n", result);
}

void message_callback(struct mosquitto *mosq, void *obj, const struct mosquitto_message *message)
{
	bool match = 0;
	printf("got message '%.*s' for topic '%s'\n", message->payloadlen, (char*) message->payload, message->topic);

	mosquitto_topic_matches_sub("/devices/wb-adc/controls/+", message->topic, &match);
	if (match) {
		printf("got message for ADC topic\n");
	}

}

int main(int argc, char *argv[])
{
	(void)(argc);
	(void)(argv);
	char clientid[] = "CLiente_Mac";

	int rc = 0;
	signal(SIGINT, handle_signal);
	signal(SIGTERM, handle_signal);
	start_timer(5000, time_handler1, NULL);

	mosquitto_lib_init();

	mosq = (struct mosquitto *)mosquitto_new(clientid, true, 0);
	mosq->username = "olaf";
	mosq->password = "olaf97";

	if(mosq){
		mosquitto_connect_callback_set(mosq, connect_callback);
		mosquitto_message_callback_set(mosq, message_callback);

	    rc = mosquitto_connect(mosq, mqtt_host, mqtt_port, 60);

		mosquitto_subscribe(mosq, NULL, "#", 0);

		while(run){
			rc = mosquitto_loop(mosq, -1, 2);
			if(run && rc){
				if (rc != 14){
					printf("connection error! : %d \n" , rc);
					sleep(5);
				}
				mosquitto_reconnect(mosq);
			}
		}
	}
	return rc;
}
