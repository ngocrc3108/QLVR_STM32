/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
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
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "query_string.h"
#include "i2c-lcd.h"
#include "RFID.h"
#include "string.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef hi2c1;

SPI_HandleTypeDef hspi1;

TIM_HandleTypeDef htim3;

UART_HandleTypeDef huart1;
DMA_HandleTypeDef hdma_usart1_rx;

/* USER CODE BEGIN PV */
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_I2C1_Init(void);
static void MX_SPI1_Init(void);
static void MX_TIM3_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
Sys_Mode mode;
Read_State readState;
char id[ID_SIZE + 1] = {0xEE, 0xEE, 0xEE};
char str_id[ID_SIZE*2+1];
char Rx_data[UART_BUFFER_SIZE];
char Tx_data[UART_BUFFER_SIZE];
Display display;
uint8_t servoPinValue = 1;

void onOpen(char* Rx_data) {
	char name[NAME_SIZE];
	char fee[FEE_SIZE];
	char dir[5]; // in or out
	char buf[17];

	getParameter(Rx_data, "name=", name);
	getParameter(Rx_data, "dir=", dir);

	lcd_clear_display();
	HAL_Delay(50);
	lcdPrintTitle(&display, name);

	if(strcmp(dir, "out") == 0) {
		getParameter(Rx_data, "fee=", fee);
		sprintf(buf, "OUT: %s", fee);
	}
	else
		sprintf(buf, "IN: ---");

	lcdPrintInfo(&display, buf);
}

void onDeny(char* Rx_data) {
	char reason[LCD_LENGTH+1];
	getParameter(Rx_data, "reason=", reason);

	lcd_clear_display();
	HAL_Delay(50);
	lcdPrintTitle(&display, "ACCESS DENY");
	lcdPrintInfo(&display, reason);
}

Write_Status onWrite() {
	char username[LCD_LENGTH + 1];
	uint32_t startTime;
	uint32_t currentTime;
	uint32_t toggleTime;
	uint8_t isTimeOut;
	uint8_t dotCount = 0;

	getParameter(Rx_data, "username=", username);

	getParameter(Rx_data, "id=", str_id);
	convertStringToHexId(str_id, id);

	lcd_clear_display();
	HAL_Delay(50);

	lcdPrintTitle(&display, "LINK TAG");
	lcdPrintInfo(&display, username);

	startTime = HAL_GetTick();
	toggleTime = startTime;
	currentTime = startTime;
	isTimeOut = 0;

	lcd_goto_XY(1, 9);
	while(writeID(id) != RFID_OK  && !isTimeOut) {
		if(currentTime - toggleTime > TOGGLE_TIME_MS*2) {
			toggleTime = currentTime;
			HAL_GPIO_TogglePin(LED3_GPIO_Port, LED3_Pin);
			lcd_send_string(".");
			dotCount++;

			if(dotCount == 8) {
				dotCount = 0;
				lcd_goto_XY(1, 9);
				lcd_send_string("       ");
				lcd_goto_XY(1, 9);
			}
		}
		currentTime = HAL_GetTick();
		isTimeOut = currentTime - startTime > 10*1000;
	}

	if(isTimeOut) {
	  lcdPrintTitle(&display, "LINK TAG TIMEOUT");
	  return WRITE_TIMEOUT;
	}
	else {
	  lcdPrintTitle(&display, "LINK TAG OK");
	  sprintf(Tx_data, "cmd=writeRes&status=ok&id=%s", str_id);
	  HAL_UART_Transmit(&ESP32_UART, (uint8_t*)Tx_data, strlen(Tx_data), 100);
	}

	display.time = HAL_GetTick();
	display.mode = DM_INFO;

	return WRITE_OK;
}

Read_State onRead(uint32_t* readTime) {
	RFID_Status status = readID(id);

	if(status == RFID_AUTH_ERR)
		lcdPrintInfo(&display, "WRONG CARD");

	if(status != RFID_OK)
		return RS_READING; // read again

	convertToString(id, str_id);
	sprintf(Tx_data, "cmd=read&id=%s",str_id);

	HAL_UART_Transmit(&ESP32_UART, (uint8_t*)Tx_data, strlen(Tx_data), 100);

	*readTime = HAL_GetTick();

	return RS_WAIT; // wait for response
}

Read_State onWait(uint32_t readTime) {
	uint8_t count = 0;
	uint32_t currentTime = HAL_GetTick();
	uint32_t toggleTime = currentTime;

	lcd_goto_XY(2, 0);
	while(readState == RS_WAIT && currentTime - readTime < WAIT_TIME*1000 ) {
		if(currentTime - toggleTime > TOGGLE_TIME_MS) {
			toggleTime = currentTime;
			HAL_GPIO_TogglePin(LED3_GPIO_Port, LED3_Pin);
			lcd_send_string(".");
			count++;

			if(count == 16) {
				count = 0;
				lcdPrintInfo(&display, "");
				lcd_goto_XY(2, 0);
			}
		}
		currentTime = HAL_GetTick(); //update current time.
	}

	// time out
	if(readState == RS_WAIT) {
		lcdPrintInfo(&display, "TIMEOUT");
		return RS_READING;
	}

	return RS_RESPONSE;
}

Read_State onResponse() {
	char cmd[CMD_SIZE];
	getParameter(Rx_data, "cmd=", cmd);

	if(strcmp(cmd, "open") == 0)
		onOpen(Rx_data);
	else if(strcmp(cmd, "deny") == 0)
		onDeny(Rx_data);

	return RS_READING;
}

// UART interrupt
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
    // handle interrupt here
	Rx_data[Size] = '\0'; // ESP32 serial printf ignore the \0, so we have to add it manually
	char cmd[CMD_SIZE];
	getParameter(Rx_data, "cmd=", cmd);

	if(strcmp(cmd, "write") == 0)
		mode = SYS_WRITE;
	else if(readState == RS_WAIT)
		readState = RS_RESPONSE;

	// enable receive in dma mode again
    HAL_UARTEx_ReceiveToIdle_DMA(&ESP32_UART, (uint8_t*)Rx_data, UART_BUFFER_SIZE);
    __HAL_DMA_DISABLE_IT(&HDMA_ESP32_UART_RX, DMA_IT_HT);
}


/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */
  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */
  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_USART1_UART_Init();
  MX_I2C1_Init();
  MX_SPI1_Init();
  MX_TIM3_Init();
  /* USER CODE BEGIN 2 */
  HAL_TIM_Base_Start_IT(&SERVO_TIMER);
  // using UART in IDLE DMA mode (https://tapit.vn/huong-dan-su-dung-chuc-nang-uart-idle-dma)
  HAL_UARTEx_ReceiveToIdle_DMA(&ESP32_UART, (uint8_t*)Rx_data, UART_BUFFER_SIZE);
  __HAL_DMA_DISABLE_IT(&HDMA_ESP32_UART_RX, DMA_IT_HT);

  lcdInit(&display);
  RFID_Init();

  uint32_t readTime;
  mode = SYS_READ;
  readState = RS_READING;

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
	  // writing mode is determined by UART interrupt.
	  if(mode == SYS_WRITE) {
		  onWrite();

		  // after write id, change mode back to read.
		  mode = SYS_READ;
		  readState = RS_READING;
	  }
	  else {
		  // reading mode
		  if(readState == RS_READING)
			  readState = onRead(&readTime);
		  else if(readState == RS_WAIT)
			  readState = onWait(readTime);
		  else if(readState == RS_RESPONSE)
			  readState = onResponse();
	  }

	  // return to home screen 5 second after display info
	  if(display.mode != DM_HOME_SCREEN && HAL_GetTick() - display.time > INFO_DISPLAY_TIME*1000)
		  display.mode = lcdDipsplayHomeScreen();

    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.ClockSpeed = 100000;
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

/**
  * @brief SPI1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI1_Init(void)
{

  /* USER CODE BEGIN SPI1_Init 0 */

  /* USER CODE END SPI1_Init 0 */

  /* USER CODE BEGIN SPI1_Init 1 */

  /* USER CODE END SPI1_Init 1 */
  /* SPI1 parameter configuration*/
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_4;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 10;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI1_Init 2 */

  /* USER CODE END SPI1_Init 2 */

}

/**
  * @brief TIM3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM3_Init(void)
{

  /* USER CODE BEGIN TIM3_Init 0 */

  /* USER CODE END TIM3_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM3_Init 1 */

  /* USER CODE END TIM3_Init 1 */
  htim3.Instance = TIM3;
  htim3.Init.Prescaler = 1000-1;
  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim3.Init.Period = 36-1;
  htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim3, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM3_Init 2 */

  /* USER CODE END TIM3_Init 2 */

}

/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

}

/**
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMA1_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA1_Channel5_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel5_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel5_IRQn);

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
/* USER CODE BEGIN MX_GPIO_Init_1 */
/* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LED3_GPIO_Port, LED3_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, LED2_Pin|SERVO_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : LED3_Pin */
  GPIO_InitStruct.Pin = LED3_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LED3_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : PA4 */
  GPIO_InitStruct.Pin = GPIO_PIN_4;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : LED2_Pin SERVO_Pin */
  GPIO_InitStruct.Pin = LED2_Pin|SERVO_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : BUTTON_Pin */
  GPIO_InitStruct.Pin = BUTTON_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  HAL_GPIO_Init(BUTTON_GPIO_Port, &GPIO_InitStruct);

/* USER CODE BEGIN MX_GPIO_Init_2 */
/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
	HAL_TIM_Base_Stop_IT(&SERVO_TIMER);
	uint16_t period = 36*6-1;// 0.5ms


	HAL_GPIO_TogglePin(SERVO_GPIO_Port, SERVO_Pin);

	if(servoPinValue)
		period = 1440 - period; // 19.5ms

	servoPinValue = !servoPinValue;

	__HAL_TIM_SET_AUTORELOAD(&SERVO_TIMER, period); // update period
	__HAL_TIM_SET_COUNTER(&SERVO_TIMER, 0); // reset counter
	HAL_TIM_Base_Start_IT(&SERVO_TIMER); // restart timer
}

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
