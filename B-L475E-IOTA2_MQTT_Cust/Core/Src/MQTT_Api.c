/**
 ******************************************************************************
 * @file    MQTT_api.c
 * @author  MCD Application Team
 * @brief   Generic MQTT IoT connection example.
 *          Basic telemetry on sensor-equipped boards.
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

/* Includes ------------------------------------------------------------------*/
#include "MQTT_api.h"

/* The init/deinit netif functions are called from cloud.c.
 * However, the application needs to reinit whenever the connectivity seems to be broken. */
extern int net_if_reinit(void * if_ctxt);

/* Private define ------------------------------------------------------------*/
#define MODEL_MAC_SIZE                    13
#define MODEL_DEFAULT_MAC                 "0102030405"
#define MODEL_DEFAULT_LEDON               true
#define MODEL_DEFAULT_TELEMETRYINTERVAL   15
#define MQTT_SEND_BUFFER_SIZE             600

#define MQTT_READ_BUFFER_SIZE             600
#define MQTT_CMD_TIMEOUT                  5000
#define MAX_SOCKET_ERRORS_BEFORE_NETIF_RESET  3

#define MQTT_TOPIC_BUFFER_SIZE            100  /**< Maximum length of the application-defined topic names. */
#define MQTT_MSG_BUFFER_SIZE              MQTT_SEND_BUFFER_SIZE /**< Maximum length of the application-defined MQTT messages. */

/* Private typedef -----------------------------------------------------------*/
typedef struct {
	char      mac[MODEL_MAC_SIZE];      /*< To be read from the netif */
	uint32_t  ts;           /*< Tick count since MCU boot. */
} pub_data_t;

typedef struct {
	char      mac[MODEL_MAC_SIZE];      /*< To be read from the netif */
	bool      LedOn;
	uint32_t  TelemetryInterval;
} status_data_t;



/* Private macro -------------------------------------------------------------*/
#define MIN(a,b)  (((a) < (b)) ? (a) : (b))

/* Private variables ---------------------------------------------------------*/
static bool g_continueRunning;
static bool g_publishData;
static bool g_statusChanged;
static bool g_reboot;
static int g_connection_needed_score;

pub_data_t pub_data       = { MODEL_DEFAULT_MAC, 0 };
status_data_t status_data = { MODEL_DEFAULT_MAC, MODEL_DEFAULT_LEDON, MODEL_DEFAULT_TELEMETRYINTERVAL };

/* Warning: The subscribed topics names strings must be allocated separately,
 * because Paho does not copy them and uses references to dispatch the incoming message. */
char mqtt_subtopic[MQTT_TOPIC_BUFFER_SIZE];
static char mqtt_pubtopic[MQTT_TOPIC_BUFFER_SIZE];
static char mqtt_msg[MQTT_MSG_BUFFER_SIZE];

/* Private function prototypes -----------------------------------------------*/
void genericmqtt_client_XCube_sample_run(void);
int stiot_publish(void * mqtt_ctxt, const char * topic, const char * msg);
int32_t comp_left_ms(uint32_t init, uint32_t now, uint32_t timeout);
int network_read(Network* n, unsigned char* buffer, int len, int timeout_ms);
int network_write(Network* n, unsigned char* buffer, int len, int timeout_ms);
void allpurposeMessageHandler(MessageData* data);

int parse_and_fill_device_config(device_config_t ** pConfig, const char *string);
void free_device_config(device_config_t * config);
int string_allocate_from_token(char ** pDestString, char * tokenName, const char * sourceString);
static unsigned char mqtt_send_buffer[MQTT_SEND_BUFFER_SIZE];
static unsigned char mqtt_read_buffer[MQTT_READ_BUFFER_SIZE];
/* Exported functions --------------------------------------------------------*/

uint8_t mqtt_client_connect(MQTTClient* client, device_config_t *device_config, uint16_t port,
		void *connectioncb,void *userdata, MQTTPacket_connectData *options){
	conn_sec_t connection_security  = CONN_SEC_NONE;
	Network network;

	memset(&pub_data, 0, sizeof(pub_data));
	platform_init();
	/* Initialize the defaults of the published messages. */
	net_macaddr_t mac = { 0 };
	net_get_mac_address(hnet, &mac);
	strncpy(pub_data.mac, status_data.mac, MODEL_MAC_SIZE - 1);

	status_data.TelemetryInterval = MODEL_DEFAULT_TELEMETRYINTERVAL;
	/* Re-connection loop: Socket level, with a netIf reconnect in case of repeated socket failures. */

	/* Init MQTT client */
	net_sockhnd_t socket;
	net_ipaddr_t ip;
	int ret = 0;

	/* If the socket connection failed MAX_SOCKET_ERRORS_BEFORE_NETIF_RESET times in a row,
	 * even if the netif still has a valid IP address, we assume that the network link is down
	 * and must be reset. */
	if ( (net_get_ip_address(hnet, &ip) == NET_ERR) || (g_connection_needed_score > MAX_SOCKET_ERRORS_BEFORE_NETIF_RESET) )
	{
		msg_info("Network link %s down. Trying to reconnect.\n", (g_connection_needed_score > MAX_SOCKET_ERRORS_BEFORE_NETIF_RESET) ? "may be" : "");
		if (net_reinit(hnet, (net_if_reinit)) != 0)
		{
			msg_error("Netif re-initialization failed.\n");
		}
		else
		{
			msg_info("Netif re-initialized successfully.\n");
			HAL_Delay(1000);
			g_connection_needed_score = 1;
		}
	}

	ret = net_sock_create(hnet, &socket, (connection_security == CONN_SEC_NONE) ? NET_PROTO_TCP :NET_PROTO_TLS);
	if (ret != NET_OK)
	{
		msg_error("Could not create the socket.\n");
	}
	else
	{
		switch(connection_security)
		{
		case CONN_SEC_NONE:
			break;
		default:
			msg_error("Invalid connection security mode. - %d\n", connection_security);
		}
		ret |= net_sock_setopt(socket, "sock_noblocking", NULL, 0);
	}

	if (ret != NET_OK)
	{
		msg_error("Could not retrieve the security connection settings and set the socket options.\n");
	}
	else
	{
		ret = net_sock_open(socket, device_config->HostName, atoi(device_config->HostPort), 0);
	}

	if (ret != NET_OK)
	{
		msg_error("Could not open the socket at %s port %d.\n", device_config->HostName, atoi(device_config->HostPort));
		g_connection_needed_score++;
		HAL_Delay(1000);
	}
	else
	{
		network.my_socket = socket;
		network.mqttread = (network_read);
		network.mqttwrite = (network_write);

		MQTTClientInit(client, &network, MQTT_CMD_TIMEOUT,
				mqtt_send_buffer, MQTT_SEND_BUFFER_SIZE,
				mqtt_read_buffer, MQTT_READ_BUFFER_SIZE);

		/* MQTT connect */
		//options = MQTTPacket_connectData_initializer;

		ret = MQTTConnect(client, options);
		if (ret != 0)
		{
			msg_error("MQTTConnect() failed: %d\n", ret);
		}
		else {
			ret = MQTTYield(client, 500);
		}

	}
	return ret;
}

int mqtt_client_suscribe(MQTTClient* client,const char *topic, enum QoS qos,  messageHandler messageHandler){
	return MQTTSubscribe(client, topic, qos, messageHandler);
}

int cloud_device_enter_credentials(void)
{
	int ret = 0;

	return ret;
}

bool app_needs_device_keypair()
{

	const char * config_string = NULL;
	device_config_t * device_config = NULL;
	conn_sec_t security = CONN_SEC_UNDEFINED;

	if(1)
	{
		msg_error("Failed retrieving the device configuration string.\n");
	}
	else
	{
		if (parse_and_fill_device_config(&device_config, config_string) == 0)
		{
			security = (conn_sec_t) atoi(device_config->ConnSecurity);
			free_device_config(device_config);
		}
		else
		{
			msg_error("Could not parse the connection security settings from the configuration string.\n");
		}
	}

	return (security == CONN_SEC_MUTUALAUTH) ? true : false;

}

/** Function to read data from the socket opened into provided buffer
 * @param - Address of Network Structure
 *        - Buffer to store the data read from socket
 *        - Expected number of bytes to read from socket
 *        - Timeout in milliseconds
 * @return - Number of Bytes read on SUCCESS
 *         - -1 on FAILURE
 **/
int network_read(Network* n, unsigned char* buffer, int len, int timeout_ms)
{
	int bytes;

	bytes = net_sock_recv((net_sockhnd_t) n->my_socket, buffer, len);
	if(bytes < 0)
	{
		msg_error("net_sock_recv failed - %d\n", bytes);
		bytes = -1;
	}

	return bytes;
}

/** Function to write data to the socket opened present in provided buffer
 * @param - Address of Network Structure
 *        - Buffer storing the data to write to socket
 *        - Number of bytes of data to write to socket
 *        - Timeout in milliseconds
 * @return - Number of Bytes written on SUCCESS
 *         - -1 on FAILURE
 **/
int network_write(Network* n, unsigned char* buffer, int len, int timeout_ms)
{
	int rc;

	rc = net_sock_send((net_sockhnd_t) n->my_socket, buffer, len);
	if(rc < 0)
	{
		msg_error("net_sock_send failed - %d\n", rc);
		rc = -1;
	}

	return rc;
}

/** Message callback
 *
 *  Note: No context handle is passed by the callback. Must rely on static variables.
 *        TODO: Maybe store couples of hander/contextHanders so that the context could
 *              be retrieved from the handler address. */
void allpurposeMessageHandler(MessageData* data)
{
	snprintf(mqtt_msg, MIN(MQTT_MSG_BUFFER_SIZE, data->message->payloadlen + 1),
			"%s", (char *)data->message->payload);
	PRINT_MESG_UART("Received message: topic: %.*s content: %s.\n",
			data->topicName->lenstring.len, data->topicName->lenstring.data,
			mqtt_msg);

}


/** Main loop */
void genericmqtt_client_XCube_sample_run(void)
{
	bool b_mqtt_connected           = false;
	device_config_t  device_config;
	conn_sec_t connection_security  = CONN_SEC_NONE;
	MQTTClient client;
	Network network;
	static unsigned char mqtt_send_buffer[MQTT_SEND_BUFFER_SIZE];
	static unsigned char mqtt_read_buffer[MQTT_READ_BUFFER_SIZE];

	g_continueRunning = true;
	g_publishData     = false;
	g_statusChanged   = true;
	g_reboot          = false;
	g_connection_needed_score = 1;

	memset(&pub_data, 0, sizeof(pub_data));

	platform_init();
	/* Initialize the defaults of the published messages. */
	net_macaddr_t mac = { 0 };
	net_get_mac_address(hnet, &mac);
	strncpy(pub_data.mac, status_data.mac, MODEL_MAC_SIZE - 1);

	status_data.TelemetryInterval = MODEL_DEFAULT_TELEMETRYINTERVAL;

	/* Re-connection loop: Socket level, with a netIf reconnect in case of repeated socket failures. */
	do
	{
		/* Init MQTT client */
		net_sockhnd_t socket;
		net_ipaddr_t ip;
		int ret = 0;

		/* If the socket connection failed MAX_SOCKET_ERRORS_BEFORE_NETIF_RESET times in a row,
		 * even if the netif still has a valid IP address, we assume that the network link is down
		 * and must be reset. */
		if ( (net_get_ip_address(hnet, &ip) == NET_ERR) || (g_connection_needed_score > MAX_SOCKET_ERRORS_BEFORE_NETIF_RESET) )
		{
			msg_info("Network link %s down. Trying to reconnect.\n", (g_connection_needed_score > MAX_SOCKET_ERRORS_BEFORE_NETIF_RESET) ? "may be" : "");
			if (net_reinit(hnet, (net_if_reinit)) != 0)
			{
				msg_error("Netif re-initialization failed.\n");
				continue;
			}
			else
			{
				msg_info("Netif re-initialized successfully.\n");
				HAL_Delay(1000);
				g_connection_needed_score = 1;
			}
		}

		ret = net_sock_create(hnet, &socket, (connection_security == CONN_SEC_NONE) ? NET_PROTO_TCP :NET_PROTO_TLS);
		if (ret != NET_OK)
		{
			msg_error("Could not create the socket.\n");
		}
		else
		{
			switch(connection_security)
			{
			case CONN_SEC_NONE:
				break;
			default:
				msg_error("Invalid connection security mode. - %d\n", connection_security);
			}
			ret |= net_sock_setopt(socket, "sock_noblocking", NULL, 0);
		}

		if (ret != NET_OK)
		{
			msg_error("Could not retrieve the security connection settings and set the socket options.\n");
		}
		else
		{
			device_config.HostName = "colinashdlv.dyndns.org";
			device_config.HostPort = "8090";
			ret = net_sock_open(socket, device_config.HostName, atoi(device_config.HostPort), 0);
		}

		if (ret != NET_OK)
		{
			msg_error("Could not open the socket at %s port %d.\n", device_config.HostName, atoi(device_config.HostPort));
			g_connection_needed_score++;
			HAL_Delay(1000);
		}
		else
		{
			network.my_socket = socket;
			network.mqttread = (network_read);
			network.mqttwrite = (network_write);

			MQTTClientInit(&client, &network, MQTT_CMD_TIMEOUT,
					mqtt_send_buffer, MQTT_SEND_BUFFER_SIZE,
					mqtt_read_buffer, MQTT_READ_BUFFER_SIZE);

			/* MQTT connect */
			MQTTPacket_connectData options = MQTTPacket_connectData_initializer;
			//device_config.MQClientId = "B-L475";
			//device_config.MQUserName = "olaf";
			//device_config.MQUserPwd = "olaf97";
			//options.clientID.cstring = device_config.MQClientId;
			//options.username.cstring = device_config.MQUserName;
			//options.password.cstring = device_config.MQUserPwd;

			ret = MQTTConnect(&client, &options);
			if (ret != 0)
			{
				msg_error("MQTTConnect() failed: %d\n", ret);
			}
			else
			{
				g_connection_needed_score = 0;
				b_mqtt_connected = true;
				ret = MQTTSubscribe(&client, "#", QOS0, (allpurposeMessageHandler));
			}

			ret = MQTTYield(&client, 500);
			/* Send the telemetry data, and send the device status if it was changed by a received message. */
			uint32_t last_telemetry_time_ms = HAL_GetTick();
			do
			{
				uint8_t command = 0;
				bool b_sample_data = (command == 0); /* If short button push, publish once. */
				if (command == 0)                  /* If long button push, toggle the telemetry publication. */
				{
					g_publishData = !g_publishData;
				}

				int32_t left_ms = comp_left_ms(last_telemetry_time_ms, HAL_GetTick(), status_data.TelemetryInterval * 1000);

				if ( ((g_publishData == true) && (left_ms <= 0))
						|| (b_sample_data == true) )
				{
					last_telemetry_time_ms = HAL_GetTick();

					pub_data.ts = time(NULL); /* last_telemetry_time_ms; */

					/* Create and send the message. */
					/* Note: The state.reported object hierarchy is used to help the inter-operability with 1st tier cloud providers. */

					ret = stiot_publish(&client, "hola", "holaxd");  /* Wrapper for MQTTPublish() */
					if (ret == MQSUCCESS)
					{

						msg_info("#\n");
						msg_info("publication topic: %s \tpayload: %s\n", mqtt_pubtopic, mqtt_msg);
					}
					else
					{
						msg_error("Telemetry publication failed.\n");
						g_connection_needed_score++;
					}

					ret = MQTTYield(&client, 5000);
					if (ret != MQSUCCESS)
					{
						msg_error("Yield failed. Reconnection needed?.\n");
						/* g_connection_needed_score++; */
					}

				}

			} while ( g_connection_needed_score == 0 );


			/* The publication loop is exited.
           NB: No need to unsubscribe as we are disconnecting.
           NB: MQTTDisconnect() will raise additional error logs if the network link is already broken,
               but it is the only way to clean the MQTT session. */
			if (b_mqtt_connected == true)
			{
				ret = MQTTDisconnect(&client);
				if (ret != MQSUCCESS)
				{
					msg_error("MQTTDisconnect() failed.\n");
				}
				b_mqtt_connected = false;
			}
			if (NET_OK !=  net_sock_close(socket))
			{
				msg_error("net_sock_close() failed.\n");
			}
		}

		if (NET_OK != net_sock_destroy(socket))
		{
			msg_error("net_sock_destroy() failed.\n");
		}
	} while (!g_reboot && (g_connection_needed_score > 0));

	platform_deinit();

}

int mqtt_publish(MQTTClient *client, const char * topic, const char * msg,uint16_t len,
			uint8_t qos, uint8_t retain, void * Mqtt_Pub_Request_CB, void * user){
	return stiot_publish(client, topic,  msg);
}

/**
 * MQTT publish API abstraction called by the metering loop.
 */
int stiot_publish(void * mqtt_ctxt, const char * topic, const char * msg)
{
	int rc;
	MQTTMessage mqmsg;
	memset(&mqmsg, 0, sizeof(MQTTMessage));
	mqmsg.qos = QOS0;
	mqmsg.payload = (char *) msg;
	mqmsg.payloadlen = strlen(msg);

	rc = MQTTPublish(mqtt_ctxt, topic, &mqmsg);
	if (rc != MQSUCCESS)
	{
		msg_error("Failed publishing %s on %s\n", (char *)(mqmsg.payload), topic);
	}
	return rc;
}


/** Look for a 'key=value' pair in the passed configuration string, and return a new buffer
 *  holding the 'value' field.
 */
int string_allocate_from_token(char ** pDestString, char * tokenName, const char * sourceString)
{
	int ret = 0;
	char * key = NULL;
	char * value = NULL;

	if ((key = strstr(sourceString, tokenName)) != NULL)
	{
		int size = 0;
		value = key + strlen(tokenName);    /* '=' key=value separator is part of tokenName. */
		if ((key = strstr(value, ";")) != NULL)
		{
			size = key - value;
		}
		*pDestString = malloc(size + 1);
		if (*pDestString != NULL)
		{
			memcpy(*pDestString, value, size);
			(*pDestString)[size] = '\0';
		}
		else
		{
			msg_error("Allocation failed\n");
		}
	}

	return ret;
}


/** Allocate and return a device_config_t structure initialized with the values defined by the passed
 *  configuration string.
 *  The buffers holding the structure and those fields are allocated dynamically by the callee, and
 *  must be freed after usage by free_device_config().
 */
int parse_and_fill_device_config(device_config_t ** pConfig, const char *string)
{
	int ret = -1;
	device_config_t * config = NULL;

	if (strlen(string) > 300)
	{
		msg_error("Cannot parse the configuration string:  It is not null-terminated!\n");
	}
	else
	{
		if (pConfig == NULL)
		{
			msg_error("Null parameter\n");
		}
		else
		{
			config = malloc(sizeof(device_config_t));
			memset(config, 0, sizeof(device_config_t));

			ret = string_allocate_from_token(&config->HostName, "HostName=", string);
			ret |= string_allocate_from_token(&config->HostPort, "HostPort=", string);
			ret |= string_allocate_from_token(&config->ConnSecurity, "ConnSecurity=", string);
			//ret |= string_allocate_from_token(&config->MQClientId, "MQClientId=", string);
			//ret |= string_allocate_from_token(&config->MQUserName, "MQUserName=", string);
			//ret |= string_allocate_from_token(&config->MQUserPwd, "MQUserPwd=", string);

			if (ret != 0)
			{
				msg_error("Failed parsing the device configuration string.\n");
				free_device_config(config);
			}
			else
			{
				*pConfig = config;
				ret = 0;
			}
		}
	}

	return ret;
}


/** Free a device_config_t allocated by parse_and_fill_device_config().
 */
void free_device_config(device_config_t * config)
{
	if (config != NULL)
	{
		if (config->HostName != NULL) free(config->HostName);
		if (config->HostPort != NULL) free(config->HostPort);
		if (config->ConnSecurity != NULL) free(config->ConnSecurity);
		//if (config->MQClientId != NULL) free(config->MQClientId);
		//if (config->MQUserName != NULL) free(config->MQUserName);
		//if (config->MQUserPwd != NULL) free(config->MQUserPwd);
		free(config);
	}
}


/**
 * @brief   Return the integer difference between 'init + timeout' and 'now'.
 *          The implementation is robust to uint32_t overflows.
 * @param   In:   init      Reference index.
 * @param   In:   now       Current index.
 * @param   In:   timeout   Target index.
 * @retval  Number of units from now to target.
 */
int32_t comp_left_ms(uint32_t init, uint32_t now, uint32_t timeout)
{
	int32_t ret = 0;
	uint32_t wrap_end = 0;

	if (now < init)
	{ /* Timer wrap-around detected */
		/* printf("Timer: wrap-around detected from %d to %d\n", init, now); */
		wrap_end = UINT32_MAX - init;
	}
	ret = wrap_end - (now - init) + timeout;

	return ret;
}


/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
