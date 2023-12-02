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
#define UART_BUFFER_SIZE 100
#define ESP32_UART huart1
#define HDMA_ESP32_UART_RX hdma_usart1_rx
#define CMD_SIZE 20
#define NAME_SIZE 17 //include '\0'
#define FEE_SIZE 6
#define LCD_LENGTH 16
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef hi2c1;

SPI_HandleTypeDef hspi1;

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
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
Sys_Mode mode;
Read_State readState;
uint8_t id[ID_SIZE + 1];
uint8_t str_id[ID_SIZE*2 + 1];
uint8_t Rx_data[UART_BUFFER_SIZE];
uint8_t Tx_data[UART_BUFFER_SIZE];

void displayHomeCreen() {
	lcd_clear_display();
	HAL_Delay(50);
	lcd_goto_XY(1, 0);
	lcd_send_string("SCAN HERE");
}

void convertStringToHexId(uint8_t* str, uint8_t* id) {
	for(uint8_t i = 0, j = 0; i < ID_SIZE; i++, j+=2) {
		// first character
		if(str[j] >= '0' && str[j] <= '9')
			id[i] = ((str[j] - '0') << 4);
		else
			id[i] = ((str[j] - 'a' + 10) << 4);

		// second character
		if(str[j+1] >= '0' && str[j+1] <= '9')
			id[i] += str[j+1] - '0';
		else
			id[i] += str[j+1] - 'a' + 10;
	}
}

void onOpen(uint8_t* Rx_data) {
	uint8_t name[NAME_SIZE];
	uint8_t fee[FEE_SIZE];
	uint8_t buf[17];
	getKey(Rx_data, "name=", name);
	getKey(Rx_data, "fee=", fee);
	sprintf((char*)buf, "Fee: %s", fee);

	lcd_clear_display();
	HAL_Delay(50);
	lcd_goto_XY(1, 0);
	lcd_send_string((char*)name);
	lcd_goto_XY(2, 0);
	lcd_send_string((char*)buf);
}

void onDeny(uint8_t* Rx_data) {
	uint8_t reason[LCD_LENGTH+1];
	getKey(Rx_data, "reason=", reason);

	lcd_clear_display();
	HAL_Delay(50);
	lcd_goto_XY(1, 0);
	lcd_send_string((char*)reason);
}

Write_Status onWrite() {
	uint8_t username[LCD_LENGTH + 1];
	uint8_t count = 0;
	uint32_t startTime = HAL_GetTick();
	uint8_t isTimeOut = 0;
	uint8_t countDot = 0;

	getKey(Rx_data, "username=", username);

	getKey(Rx_data, "id=", str_id);
	convertStringToHexId(str_id, id);

	lcd_clear_display();
	HAL_Delay(50);

	lcd_goto_XY(1, 0);
	lcd_send_string("LINK TAG");

	lcd_goto_XY(2, 0);
	lcd_send_string((char*)username);

	lcd_goto_XY(1, 9);

	while(writeID(id) != RFID_OK  && !isTimeOut) {
		isTimeOut = HAL_GetTick() - startTime > 10*1000;
		count++;
		if(count == 20) {
			count = 0;
			countDot++;
			if(countDot == 6) {
				countDot = 0;
				lcd_goto_XY(1, 9);
				lcd_send_string("       ");
				lcd_goto_XY(1, 9);
			}
			HAL_GPIO_TogglePin(LED3_GPIO_Port, LED3_Pin);
			lcd_send_string(".");
		}
	}

	lcd_goto_XY(1, 0);

	if(isTimeOut) {
	  lcd_send_string("LINK TAG TIMEOUT");
	  return WRITE_TIMEOUT;
	}
	else {
	  lcd_send_string("LINK TAG OK     ");
	  sprintf((char*)Tx_data, "cmd=writeRes&status=ok&id=%s", str_id);
	  HAL_UART_Transmit(&ESP32_UART, Tx_data, strlen((char*)Tx_data), 100);
	}

	return WRITE_OK;
}

Read_State onRead(uint32_t* readTime) {
	if(readID(id) != RFID_OK)
		return RS_Reading; // read again

  convertToString(id, str_id);
  sprintf((char*)Tx_data, "cmd=read&id=%s",(char*)str_id);

  HAL_UART_Transmit(&ESP32_UART, Tx_data, strlen((char*)Tx_data), 100);

  *readTime = HAL_GetTick();

  return RS_WAIT; // wait for response
}

Read_State onWait(uint32_t readTime) {
	uint8_t count = 0;
	lcd_goto_XY(2, 0);
	while(readState == RS_WAIT && HAL_GetTick() - readTime < 20*1000 ) {
		count++;
		if(count == 12) {
			count = 0;
			lcd_goto_XY(2, 0);
			lcd_send_string("            ");
			lcd_goto_XY(2, 0);
		}

		HAL_Delay(100);
		HAL_GPIO_TogglePin(LED3_GPIO_Port, LED3_Pin);
		lcd_send_string(".");
	}

	// time out
	if(readState == RS_WAIT) {
		lcd_goto_XY(2, 0);
		lcd_send_string("            ");
		return RS_Reading;
	}

	return RS_RESPONSE;
}

Read_State onResponse() {
	uint8_t cmd[CMD_SIZE];
	getKey(Rx_data, "cmd=", cmd);

	if(strcmp((char*)cmd, "open") == 0)
		onOpen(Rx_data);
	else if(strcmp((char*)cmd, "deny") == 0)
		onDeny(Rx_data);

	return RS_Reading;
}


void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
    // handle interrupt here
	Rx_data[Size] = '\0'; // ESP32 serial printf ignore the \0, so we have to add it manually
	uint8_t cmd[CMD_SIZE];
	getKey(Rx_data, "cmd=", cmd);

	if(strcmp((char*)cmd, "write") == 0)
		mode = SYS_WRITE;
	else if(readState == RS_WAIT)
		readState = RS_RESPONSE;

	// enable receive in dma mode again
    HAL_UARTEx_ReceiveToIdle_DMA(&ESP32_UART, Rx_data, UART_BUFFER_SIZE);
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
  /* USER CODE BEGIN 2 */
  HAL_UARTEx_ReceiveToIdle_DMA(&ESP32_UART, Rx_data, UART_BUFFER_SIZE);
  __HAL_DMA_DISABLE_IT(&HDMA_ESP32_UART_RX, DMA_IT_HT);

  lcd_init();
  displayHomeCreen();
  RFID_Init();

  uint32_t readTime;
  uint32_t displayTime;
  mode = SYS_READ;
  readState = RS_Reading;
  uint8_t displayDone = 1;

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
	  // writing mode is determined by UART interrupt
	  if(mode == SYS_WRITE) {
		  onWrite();
		  mode = SYS_READ;
		  readState = RS_Reading;
		  displayTime = HAL_GetTick();
		  displayDone = 0;
	  }
	  else {
		  // reading mode
		  if(readState == RS_Reading)
			  readState = onRead(&readTime);
		  else if(readState == RS_WAIT)
			  readState = onWait(readTime);
		  else if(readState == RS_RESPONSE) {
			  readState = onResponse();
			  displayTime = HAL_GetTick();
			  displayDone = 0;
		  }
	  }

	  // clear the screen after 5 second
	  if(!displayDone && HAL_GetTick() - displayTime > 5*1000) {
		  displayHomeCreen();
		  displayDone = 1;
	  }

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
  HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin, GPIO_PIN_RESET);

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

  /*Configure GPIO pin : LED2_Pin */
  GPIO_InitStruct.Pin = LED2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LED2_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : BUTTON_Pin */
  GPIO_InitStruct.Pin = BUTTON_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  HAL_GPIO_Init(BUTTON_GPIO_Port, &GPIO_InitStruct);

/* USER CODE BEGIN MX_GPIO_Init_2 */
/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

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
