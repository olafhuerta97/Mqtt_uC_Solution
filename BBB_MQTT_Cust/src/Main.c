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
static char TopicBBB[] = "BBB/greetings";
static char PayloadBBB[] = "Hello from BBB _ 2";
size_t timer1;

/*mosquitto_publish(struct mosquitto *mosq, int *mid, const char *topic,
 *  int payloadlen, const void *payload, int qos, bool retain)
*/
void time_handler1(size_t timer_id, void * user_data)
{
    //printf("timer expired.(%d)\n", timer_id);
    //printf("Publishing hi from BBB.(%d)\n", timer_id);
    mosquitto_publish(mosq,NULL,TopicBBB,sizeof(PayloadBBB),PayloadBBB,2,0);
}

static int run = 2;

void handle_signal(int s)
{
    stop_timer(timer1);
    finalize();
	printf("Signal Handler signal:%d\n", s);
	run--;
}

void connect_callback(struct mosquitto *mosq, void *obj, int result)
{
	printf("connect callback, rc=%d\n", result);
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

	char clientid[] = "CLiente_BBB";

	int rc = 0;
	signal(SIGINT, handle_signal);
	signal(SIGTERM, handle_signal);
	initialize();
	timer1 = start_timer(2000, time_handler1, TIMER_PERIODIC, NULL);


	mosquitto_lib_init();

	mosq = (struct mosquitto *)mosquitto_new(clientid, true, 0);
	mosq->username = "olaf";
	mosq->password = "olaf97";

	if(mosq){
		mosquitto_connect_callback_set(mosq, connect_callback);
		mosquitto_message_callback_set(mosq, message_callback);

	    rc = mosquitto_connect(mosq, mqtt_host, mqtt_port, 60);

		mosquitto_subscribe(mosq, NULL, "F767/ld1/#", 0);

		while(run){
			rc = mosquitto_loop(mosq, -1, 1);
			if(run && rc){
				printf("connection error!\n");
				sleep(10);
				mosquitto_reconnect(mosq);
			}
		}
		mosquitto_destroy(mosq);
		printf("Destroided mosq instance\n");
	}

	mosquitto_lib_cleanup();
	printf("Lib cleanup\n");
	return rc;
}


