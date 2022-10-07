/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2022 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32l4xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include "net.h"
#include <stdbool.h>
#include "cloud.h"
/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

void PRINT_MESG_UART(const char * format, ... );

#define msg_error(...)  \
	{ \
	printf("ERROR: %s L#%d ", __func__, __LINE__); \
	printf(__VA_ARGS__); \
	}
#define msg_info(...)  \
	{ \
	printf("ERROR: %s L#%d ", __func__, __LINE__); \
	printf(__VA_ARGS__); \
	}
#define USE_WIFI
#if defined(USE_WIFI)
#define NET_IF  NET_IF_WLAN
#endif

void PRINT_MESG_UART(const char * format, ... );

typedef uint8_t u8_t;
typedef uint16_t u16_t;
typedef uint32_t u32_t;
typedef u8_t    err_t;

/** Definitions for error constants. */
typedef enum {
/** No error, everything OK. */
  ERR_OK         = 0,
/** Out of memory error.     */
  ERR_MEM        = -1,
/** Buffer error.            */
  ERR_BUF        = -2,
/** Timeout.                 */
  ERR_TIMEOUT    = -3,
/** Routing problem.         */
  ERR_RTE        = -4,
/** Operation in progress    */
  ERR_INPROGRESS = -5,
/** Illegal value.           */
  ERR_VAL        = -6,
/** Operation would block.   */
  ERR_WOULDBLOCK = -7,
/** Address in use.          */
  ERR_USE        = -8,
/** Already connecting.      */
  ERR_ALREADY    = -9,
/** Conn already established.*/
  ERR_ISCONN     = -10,
/** Not connected.           */
  ERR_CONN       = -11,
/** Low-level netif error    */
  ERR_IF         = -12,

/** Connection aborted.      */
  ERR_ABRT       = -13,
/** Connection reset.        */
  ERR_RST        = -14,
/** Connection closed.       */
  ERR_CLSD       = -15,
/** Illegal argument.        */
  ERR_ARG        = -16
} err_enum_t;

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */
void SPI3_IRQHandler(void);
extern  SPI_HandleTypeDef hspi;
extern RNG_HandleTypeDef hrng;
extern net_hnd_t         hnet; /* Is initialized by cloud_main(). */
extern RTC_HandleTypeDef hrtc;
/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
void   MX_SPI3_Init(void);
/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
