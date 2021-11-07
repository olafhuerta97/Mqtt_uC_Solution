/*
 * button.c
 *
 *  Created on: Oct 27, 2021
 *      Author: Olaf
 */
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include "MQTT_main.h"
#include "button.h"

#define MSG_BUTTON     "Button Pressed"

void button_handler_isr(void){
	mqtt_publish_cust("", MSG_BUTTON,BUTTON);
}
