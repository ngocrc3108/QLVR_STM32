/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2023 STMicroelectronics.
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
#include "stm32f1xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define LED3_Pin GPIO_PIN_13
#define LED3_GPIO_Port GPIOC
#define LED2_Pin GPIO_PIN_11
#define LED2_GPIO_Port GPIOB
#define BUTTON_Pin GPIO_PIN_15
#define BUTTON_GPIO_Port GPIOB

typedef enum {
	SYS_READ,
	SYS_WRITE
} Sys_Mode;

typedef enum {
	RS_READING = 0,
	RS_WAIT,
	RS_RESPONSE
} Read_State;

typedef enum {
	WRITE_OK = 0,
	WRITE_TIMEOUT
} Write_Status;

typedef enum {
	DM_HOME_SCREEN,
	DM_INFO
} Display_mode;

/* USER CODE BEGIN Private defines */

// function for check-in/check-out
Read_State onRead(uint32_t* readTime);
Read_State onWait(uint32_t readTime); // wait for reading response (open or deny)
Read_State onResponse();
void onOpen(uint8_t* Rx_data);
void onDeny(uint8_t* Rx_data);

// function for linking user account to a RFID tag
Write_Status onWrite();

Display_mode displayHomeScreen();

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
