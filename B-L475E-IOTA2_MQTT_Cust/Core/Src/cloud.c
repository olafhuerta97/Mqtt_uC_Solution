/**
  ******************************************************************************
  * @file    cloud.c
  * @author  MCD Application Team
  * @brief   .
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "main.h"
#include "net.h"
#include "timedate.h"
#include "heap.h"
#include "cloud.h"
#include "version.h"

extern int net_if_init(void * if_ctxt);
extern int net_if_deinit(void * if_ctxt);
extern int net_if_reinit(void * if_ctxt); 

/* Private defines -----------------------------------------------------------*/
#define CLOUD_TIMEDATE_TLS_VERIFICATION_IGNORE  /**< Accept to connect to a server which is not verified by TLS */

/* Private typedef -----------------------------------------------------------*/
typedef enum
{
  CLOUD_DEMO_WIFI_INITIALIZATION_ERROR      = -2,
  CLOUD_DEMO_MAC_ADDRESS_ERROR              = -3,
  CLOUD_DEMO_WIFI_CONNECTION_ERROR          = -4,
  CLOUD_DEMO_IP_ADDRESS_ERROR               = -5,
  CLOUD_DEMO_CONNECTION_ERROR               = -6,
  CLOUD_DEMO_TIMEDATE_ERROR                 = -7,
  CLOUD_DEMO_C2C_INITIALIZATION_ERROR       = -8
} CLOUD_DEMO_Error_t;

/* Private function prototypes -----------------------------------------------*/
void CLOUD_Error_Handler(int errorCode);

/* Exported functions --------------------------------------------------------*/
/**
  * @brief  Ask yes/no question.
  * @param  None
  * @retval None
  */
bool dialog_ask(char *s)
{
  char console_yn;
  do
  {
    printf("%s",s);
    console_yn= getchar();
    printf("\b");
  }
  while((console_yn != 'y') && (console_yn != 'n') && (console_yn != '\n'));
  if (console_yn == 'y') return true;
  return false;
}


/**
  * @brief  This function is executed in case of error occurrence.
  * @param  None
  * @retval None
  */
void CLOUD_Error_Handler(int errorCode)
{
  switch (errorCode)
  {
    case (CLOUD_DEMO_C2C_INITIALIZATION_ERROR):
    case (CLOUD_DEMO_WIFI_INITIALIZATION_ERROR):
    {
      printf("Error initializing the module!\n");
      
      break;
    }
    case (CLOUD_DEMO_MAC_ADDRESS_ERROR):
    {
      printf("Error detecting module!\n");
      
      break;
    }
    case (CLOUD_DEMO_WIFI_CONNECTION_ERROR):
    {
      printf("Error connecting to AP!\n");
      
      break;
    }
    case (CLOUD_DEMO_IP_ADDRESS_ERROR):
    {
      printf("Error retrieving IP address!\n");
      
      break;
    }
    case (CLOUD_DEMO_CONNECTION_ERROR):
    {
      printf("Error connecting to Cloud!\n");
      
      break;
    }
    case (CLOUD_DEMO_TIMEDATE_ERROR):
    {
      printf("Error initializing the RTC from the network time!\n");
      
      break;
    }
    default:
    {
      break;
    }
  }
  
  while (1)
  {
    //BSP_LED_Toggle(LED_GREEN);
    HAL_Delay(200);
  }
}

const firmware_version_t version = { FW_VERSION_NAME, FW_VERSION_MAJOR, FW_VERSION_MINOR, FW_VERSION_PATCH, FW_VERSION_DATE};

int platform_init(void)
{
  net_ipaddr_t ipAddr;
  net_macaddr_t macAddr;
  const firmware_version_t  *fw_version=&version;;
  unsigned int random_number = 0;
  
  /* Initialize the seed of the stdlib rand() SW implementation from the RNG. */
  if (HAL_RNG_GenerateRandomNumber(&hrng, (uint32_t *) &random_number) == HAL_OK)
  {
    srand(random_number);
  }

  printf("\n");
  printf("*************************************************************\n");
  printf("***   STM32 IoT Discovery kit for                         \n");
  printf("***      STM32F413/STM32F769/STM32L475/STM32L496 MCU      \n");
  printf("***   %s Cloud Connectivity Demonstration                 \n",fw_version->name);
  printf("***   FW version %d.%d.%d - %s      \n",
           fw_version->major, fw_version->minor, fw_version->patch, fw_version->packaged_date);
  printf("*************************************************************\n");

  
  printf("\n*** Board personalization ***\n\n");
  /* Network initialization */
  if (net_init(&hnet, NET_IF, (net_if_init)) != NET_OK)
  {
    CLOUD_Error_Handler(CLOUD_DEMO_IP_ADDRESS_ERROR);
    return -1;
  }

  if (net_get_mac_address(hnet, &macAddr) == NET_OK)
  {
    PRINT_MESG_UART("Mac address: %02x:%02x:%02x:%02x:%02x:%02x\n",
             macAddr.mac[0], macAddr.mac[1], macAddr.mac[2], macAddr.mac[3], macAddr.mac[4], macAddr.mac[5]);
  }
  else
  {
    CLOUD_Error_Handler(CLOUD_DEMO_MAC_ADDRESS_ERROR);
    return -1;
  }
    
  /* Slight delay since the netif seems to take some time prior to being able
   to retrieve its IP address after a connection. */
  HAL_Delay(500);

  PRINT_MESG_UART("Retrieving the IP address.\n");

  if (net_get_ip_address(hnet, &ipAddr) != NET_OK)
  {
    CLOUD_Error_Handler(CLOUD_DEMO_IP_ADDRESS_ERROR);
    return -1;
  }
  else
  {
    switch(ipAddr.ipv)
    {
      case NET_IP_V4:
    	  PRINT_MESG_UART("IP address: %d.%d.%d.%d\n", ipAddr.ip[12], ipAddr.ip[13], ipAddr.ip[14], ipAddr.ip[15]);
        break;
      case NET_IP_V6:
      default:
        CLOUD_Error_Handler(CLOUD_DEMO_IP_ADDRESS_ERROR);
        return -1;
    }
  }
  /* End of network initialisation */

  /* Security and cloud parameters definition */
  /* Define, or allow to update if the user button is pushed. */

   
 return 0;
}


void    platform_deinit()
{
   /* Close Cloud connectivity demonstration */
  printf("\n*** Cloud connectivity demonstration ***\n\n");
  printf("Cloud connectivity demonstration completed\n");


  (void)net_deinit(hnet, (net_if_deinit));

}

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
