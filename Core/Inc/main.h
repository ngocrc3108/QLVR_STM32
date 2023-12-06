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
#include "i2c-lcd.h"

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
#define SERVO_Pin GPIO_PIN_14
#define SERVO_GPIO_Port GPIOB
#define BUTTON_Pin GPIO_PIN_15
#define BUTTON_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */
#define UART_BUFFER_SIZE 100
#define ESP32_UART huart1
#define SERVO_TIMER htim3
#define HDMA_ESP32_UART_RX hdma_usart1_rx
#define CMD_SIZE 20
#define NAME_SIZE 17 //include '\0'
#define FEE_SIZE 6
#define INFO_DISPLAY_TIME 5 // return to home screen after display info (name, fee...)
#define WAIT_TIME 10
#define TOGGLE_TIME_MS 50

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

// function for check-in/check-out
Read_State onRead(uint32_t* readTime);
Read_State onWait(uint32_t readTime); // wait for reading response (open or deny)
Read_State onResponse();
void onOpen();
void onDeny();

// function for linking user account to a RFID tag
Write_Status onWrite();

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
