/*
 * button.c
 *
 *  Created on: Oct 27, 2021
 *      Author: Olaf
 */
#include "MQTT_main.h"
#include "button.h"

#define MSG_BUTTON     "Button Pressed"

void button_handler_isr(void){
	Mqtt_Publish_Cust("", MSG_BUTTON,BUTTON);
}
