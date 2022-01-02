/*
 * button.c
 *
 *  Created on: Oct 27, 2021
 *      Author: Olaf
 */
#include "MQTT_main.h"
#include "MQTT_button.h"

#define MSG_BUTTON     "Button Pressed"

void Button_ISR_Handler(void){
	Mqtt_Publish_Cust("", MSG_BUTTON,Button);
}
