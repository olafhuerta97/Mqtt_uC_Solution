/**
  ******************************************************************************
  * @file    MQTT_api.h
  * @author  MCD Application Team
  * @brief   External definitions for GenericMQTTCubeSample.c
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2017 STMicroelectronics International N.V. 
  * All rights reserved.</center></h2>
  *
  * Redistribution and use in source and binary forms, with or without 
  * modification, are permitted, provided that the following conditions are met:
  *
  * 1. Redistribution of source code must retain the above copyright notice, 
  *    this list of conditions and the following disclaimer.
  * 2. Redistributions in binary form must reproduce the above copyright notice,
  *    this list of conditions and the following disclaimer in the documentation
  *    and/or other materials provided with the distribution.
  * 3. Neither the name of STMicroelectronics nor the names of other 
  *    contributors to this software may be used to endorse or promote products 
  *    derived from this software without specific written permission.
  * 4. This software, including modifications and/or derivative works of this 
  *    software, must execute solely and exclusively on microcontroller or
  *    microprocessor devices manufactured by or for STMicroelectronics.
  * 5. Redistribution and use of this software other than as permitted under 
  *    this license is void and will automatically terminate your rights under 
  *    this license. 
  *
  * THIS SOFTWARE IS PROVIDED BY STMICROELECTRONICS AND CONTRIBUTORS "AS IS" 
  * AND ANY EXPRESS, IMPLIED OR STATUTORY WARRANTIES, INCLUDING, BUT NOT 
  * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A 
  * PARTICULAR PURPOSE AND NON-INFRINGEMENT OF THIRD PARTY INTELLECTUAL PROPERTY
  * RIGHTS ARE DISCLAIMED TO THE FULLEST EXTENT PERMITTED BY LAW. IN NO EVENT 
  * SHALL STMICROELECTRONICS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
  * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
  * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, 
  * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF 
  * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING 
  * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
  * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */

#ifndef __GenericMQTTXCubeSample_H
#define __GenericMQTTXCubeSample_H

#include "timingSystem.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "main.h"
#include "MQTTClient.h"
#include "paho_mqtt_platform.h"

typedef struct {
	char *HostName;
	char *HostPort;
	char *ConnSecurity;
} device_config_t;

int mqtt_client_suscribe(MQTTClient* client,const char *topic, enum QoS qos,  messageHandler messageHandler);

uint8_t mqtt_client_connect(MQTTClient* client, device_config_t*  device_config, uint16_t port,
		void *connectioncb,void *userdata, MQTTPacket_connectData *options);

void MQTTYield_Cust(void);

int mqtt_publish(MQTTClient *client, const char * topic,const char * msg,uint16_t len,
			uint8_t qos, uint8_t retain, void * Mqtt_Pub_Request_CB, void *user);

typedef enum {
  CONN_SEC_UNDEFINED = -1,    
  CONN_SEC_NONE = 0,          /**< Clear connection */
  CONN_SEC_SERVERNOAUTH = 1,  /**< Encrypted TLS connection, with no authentication of the remote host: Shall NOT be used in a production environment. */ 
  CONN_SEC_SERVERAUTH = 2,    /**< Encrypted TLS connection, with authentication of the remote host. */
  CONN_SEC_MUTUALAUTH = 3     /**< Encrypted TLS connection, with mutual authentication. */
} conn_sec_t;

#endif /* __GenericMQTTXCubeSample_H */


/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
