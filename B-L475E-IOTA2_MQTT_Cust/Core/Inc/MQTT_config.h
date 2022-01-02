/*
 * config.h
 *
 *  Created on: Oct 20, 2021
 *      Author: Olaf
 */
#ifndef SRC_CONFIG_H_
#define SRC_CONFIG_H_

/* broker settings */
#define CONFIG_USE_BROKER_LOCAL                 (1) /* 1: use local mosquitto broker; 0: do not use local broker */
#define CONFIG_USE_BROKER_MOSQUITTO_TEST        (0) /* 1: use mosquitto test broker; 0: do not use mosquitto test broker */
#define CONFIG_USE_BROKER_ADAFRUIT              (0) /* 1: use Adafruit I/O broker; 0: do not use Adafruit broker */
#define CONFIG_USE_BROKER_AZURE                 (0) /* 1: use Azure I/O broker; 0: do not use Azure broker */
#define CONFIG_USE_BROKER_HSLU                  (0) /* 1: use HSLU broker; 0: do not use HSLU broker */

/* various configuration settings */
#define CONFIG_USE_DNS                          (0) /* 1: use DNS to get broker IP address; 0: do not use DNS, use fixed address instead */

/* Client IP address configuration. */
#define configIP_ADDR0 192
#define configIP_ADDR1 168
#define configIP_ADDR2 0
#define configIP_ADDR3 153

#define configIP_MQTT_ADDR0 192
#define configIP_MQTT_ADDR1 168
#define configIP_MQTT_ADDR2 0
#define configIP_MQTT_ADDR3 160

/* Client Netmask configuration. */
#define configNET_MASK0 255
#define configNET_MASK1 255
#define configNET_MASK2 255
#define configNET_MASK3 0

/* Client Gateway address configuration. */
#define configGW_ADDR0 192
#define configGW_ADDR1 168
#define configGW_ADDR2 0
#define configGW_ADDR3 1


#if CONFIG_USE_DNS
  #define COFNIG_DNS_ADDR0 10
  #define COFNIG_DNS_ADDR1 1
  #define COFNIG_DNS_ADDR2 194
  #define COFNIG_DNS_ADDR3 41

  #define COFNIG_DNS_ADDR0 configGW_ADDR0
  #define COFNIG_DNS_ADDR1 configGW_ADDR1
  #define COFNIG_DNS_ADDR2 configGW_ADDR2
  #define COFNIG_DNS_ADDR3 configGW_ADDR3
#endif

/* connection settings to broker */
#if CONFIG_USE_BROKER_ADAFRUIT
  #define CONFIG_BROKER_HOST_NAME       "io.adafruit.com"
  #define CONFIG_BROKER_HOST_IP         NULL
  #define CONFIG_CLIENT_ID_NAME         "FRDM-K64F" /* each client connected to the host has to use a unique ID */
  #define CONFIG_CLIENT_USER_NAME       "user name" /* Adafruit IO Username, keep it SECRET! */
  #define CONFIG_CLIENT_USER_PASSWORD   "AIO Key" /* Adafruit AIO Key, keep it SECRET! */
  #define CONFIG_TOPIC_NAME             "erichs/feeds/test"
#elif CONFIG_USE_BROKER_MOSQUITTO_TEST
  #define CONFIG_BROKER_HOST_NAME       "test.mosquitto.org"
  #define CONFIG_BROKER_HOST_IP         NULL
  #define CONFIG_CLIENT_ID_NAME         "FRDM-K64F" /* each client connected to the host has to use a unique ID */
  #define CONFIG_CLIENT_USER_NAME       NULL /* no user name */
  #define CONFIG_CLIENT_USER_PASSWORD   NULL /* no password */
  #define CONFIG_TOPIC_NAME             "HSLU/test"
#elif CONFIG_USE_BROKER_HSLU
  #define CONFIG_BROKER_HOST_NAME       "eee-00031"
  #define CONFIG_BROKER_HOST_IP         "192.168.0.118" /* raspberry pi running mosquitto */
  #define CONFIG_CLIENT_ID_NAME         "FRDM-K64F" /* each client connected to the host has to use a unique ID */
  #define CONFIG_CLIENT_USER_NAME       "tastyger" /* user name */
  #define CONFIG_CLIENT_USER_PASSWORD   "1234" /* dummy password */
  #define CONFIG_TOPIC_NAME             "tastyger/test" /* hard coded topic name to subscribe to */
#elif CONFIG_USE_BROKER_LOCAL
  #define CONFIG_BROKER_HOST_NAME       "Rasp-Pi"
  #define CONFIG_BROKER_HOST_IP       	"192.168.0.160"
  #define CONFIG_CLIENT_ID_NAME         "H743" /* each client connected to the host has to use a unique ID */
  #define CONFIG_CLIENT_USER_NAME       "olaf" /* no user name */
  #define CONFIG_CLIENT_USER_PASSWORD   "olaf97" /* no password */
  #define MAX_LENGTH_TOPIC              30
#elif CONFIG_USE_BROKER_AZURE
  #define CONFIG_BROKER_HOST_NAME       "GrilloIOTHub.azure-devices.net" /* {iothubhostname} */
  #define CONFIG_CLIENT_ID_NAME         "Erich_Device" /* {deviceId} */
  #define CONFIG_CLIENT_USER_NAME       "GrilloIOTHub.azure-devices.net/Erich_Device/api-version=2016-11-14" /* {iothubhostname}/{device_id}/api-version=2016-11-14  */
  #define CONFIG_CLIENT_USER_PASSWORD   "SharedAccessSignature sr=GrilloIOTHub.azure-devices.net%2Fdevices%2FErich_Device&sig=ovxhASw4alEsJ6ncPAY8aaCZXFTT6OSz5%2BHvs7Rj7TY%3D&se=1493909811" /* part of SAS token */
  #define CONFIG_TOPIC_NAME             "devices/Erich_Device/messages/events/" /* devices/{deviceId}/events */
#endif


#endif /* SRC_CONFIG_H_ */
