/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
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
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>

#include <string.h> // for memcpy
#define SSD1306_ADDR 0x78  // 0x3C << 1

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

UART_HandleTypeDef huart2;

/* Definitions for LEDFlash_Task */
osThreadId_t LEDFlash_TaskHandle;
const osThreadAttr_t LEDFlash_Task_attributes = {
  .name = "LEDFlash_Task",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityLow,
};
/* Definitions for ReadButtonIN */
osThreadId_t ReadButtonINHandle;
const osThreadAttr_t ReadButtonIN_attributes = {
  .name = "ReadButtonIN",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityBelowNormal,
};
/* Definitions for LEDReset_Task */
osThreadId_t LEDReset_TaskHandle;
const osThreadAttr_t LEDReset_Task_attributes = {
  .name = "LEDReset_Task",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityLow1,
};
/* Definitions for Display_Task */
osThreadId_t Display_TaskHandle;
const osThreadAttr_t Display_Task_attributes = {
  .name = "Display_Task",
  //.stack_size = 128 * 4,
  .stack_size = 512 * 4,   // ← 2KB of stack
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for SensorRead_Task */
osThreadId_t SensorRead_TaskHandle;
const osThreadAttr_t SensorRead_Task_attributes = {
  .name = "SensorRead_Task",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityAboveNormal,
};
/* USER CODE BEGIN PV */
volatile uint8_t led_enabled = 1;  // 1 = ON, 0 = OFF
volatile uint8_t led_frequency_hz = 1;  // Starts at 1Hz

	//wheel speed variables
uint32_t last_pulse_time = 0;
float calculated_speed = 0.0f;
const float wheel_circumference_m = 2.1f;  // adjust for your tire
const uint32_t DEBOUNCE_TIME_MS = 80;


//Moving average for speed variables
#define SPEED_SMOOTH_WINDOW 4
float speed_buffer[SPEED_SMOOTH_WINDOW] = {0};
uint8_t speed_index = 0;


// Speed decay smoothly variable
uint32_t last_valid_speed_time = 0;




/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_I2C1_Init(void);
void StartLEDFlashTask(void *argument);
void StartReadButtonINTask(void *argument);
void StartLEDResetTask(void *argument);
void StartDisplayTask(void *argument);
void StartHallSensorTask(void *argument);

/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
void I2C_Scan_Bus(I2C_HandleTypeDef *hi2c)
{
    printf("Scanning I2C bus...\r\n");
    HAL_Delay(100);

    for (uint8_t addr = 1; addr < 127; addr++)
    {
        if (HAL_I2C_IsDeviceReady(hi2c, addr << 1, 1, 10) == HAL_OK)
        {
            printf("Found device at 0x%02X\r\n", addr << 1);
        }
        HAL_Delay(2);
    }

    printf("Scan complete.\r\n");
}


//Moving average of wheel speed = remove flactuations
float apply_speed_filter(float new_speed) {
    speed_buffer[speed_index++ % SPEED_SMOOTH_WINDOW] = new_speed;

    // Optional: reject outlier min and max
    float min = speed_buffer[0], max = speed_buffer[0], sum = 0;
    for (int i = 0; i < SPEED_SMOOTH_WINDOW; i++) {
        if (speed_buffer[i] < min) min = speed_buffer[i];
        if (speed_buffer[i] > max) max = speed_buffer[i];
        sum += speed_buffer[i];
    }

    float avg = (sum - min - max) / (SPEED_SMOOTH_WINDOW - 2);
    return avg;

//    speed_buffer[speed_index++ % SPEED_SMOOTH_WINDOW] = new_speed;
//    float sum = 0;
//    for (int i = 0; i < SPEED_SMOOTH_WINDOW; ++i) sum += speed_buffer[i];
//    return sum / SPEED_SMOOTH_WINDOW;
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

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USART2_UART_Init();
  MX_I2C1_Init();
  /* USER CODE BEGIN 2 */
  I2C_Scan_Bus(&hi2c1);
  /* USER CODE END 2 */

  /* Init scheduler */
  osKernelInitialize();

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of LEDFlash_Task */
  LEDFlash_TaskHandle = osThreadNew(StartLEDFlashTask, NULL, &LEDFlash_Task_attributes);

  /* creation of ReadButtonIN */
  ReadButtonINHandle = osThreadNew(StartReadButtonINTask, NULL, &ReadButtonIN_attributes);

  /* creation of LEDReset_Task */
  LEDReset_TaskHandle = osThreadNew(StartLEDResetTask, NULL, &LEDReset_Task_attributes);

  /* creation of Display_Task */
  Display_TaskHandle = osThreadNew(StartDisplayTask, NULL, &Display_Task_attributes);

  /* creation of SensorRead_Task */
  SensorRead_TaskHandle = osThreadNew(StartHallSensorTask, NULL, &SensorRead_Task_attributes);


  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

  /* Start scheduler */
  osKernelStart();

  /* We should never get here as control is now taken by the scheduler */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
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

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 16;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
  RCC_OscInitStruct.PLL.PLLQ = 2;
  RCC_OscInitStruct.PLL.PLLR = 2;
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
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

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
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : B1_Pin */
  GPIO_InitStruct.Pin = B1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : LD2_Pin */
  GPIO_InitStruct.Pin = LD2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LD2_GPIO_Port, &GPIO_InitStruct);


  //My Pin setups BEGIN

    // --- Configure PC8 (LED output) properly ---
    GPIO_InitStruct.Pin = GPIO_PIN_8;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);


    // --- Configure D7 (PA8) as output and set HIGH ---
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, GPIO_PIN_SET);  // Set HIGH
    GPIO_InitStruct.Pin = GPIO_PIN_8;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    // --- Configure D2 (PA10) as input ---
    GPIO_InitStruct.Pin = GPIO_PIN_10;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    // --- Configure PB8 (D15) as Hall sensor input ---
    GPIO_InitStruct.Pin = GPIO_PIN_8;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);


   //My Pin setups END



  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/* USER CODE BEGIN Header_StartLEDFlashTask */
/**
  * @brief  Function implementing the LEDFlash_Task thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartLEDFlashTask */
void StartLEDFlashTask(void *argument)
{
  /* USER CODE BEGIN 5 */

  /* Infinite loop */
  for(;;)
  {
	printf("[StartLEDFlashTask]\n");

	if (led_enabled)
	{
		printf("Turning LED ON...\n");
		HAL_GPIO_WritePin(GPIOC, GPIO_PIN_8, GPIO_PIN_RESET);   // Turn ON LED
		osDelay(500 / led_frequency_hz);                      // ON time

		printf("Turning LED OFF...\n");
		HAL_GPIO_WritePin(GPIOC, GPIO_PIN_8, GPIO_PIN_SET); // Turn OFF LED
		osDelay(500 / led_frequency_hz);                      // OFF time

	}else
	{
		printf("Button: Turning LED OFF.  -  ");
		HAL_GPIO_WritePin(GPIOC, GPIO_PIN_8, GPIO_PIN_SET); // LED OFF
		printf("Button: Turned LED OFF\n");
		osDelay(400); // short delay to avoid busy-loop
	}
  }
  /* USER CODE END 5 */
}

/* USER CODE BEGIN Header_StartReadButtonINTask */
/**
* @brief Function implementing the ReadButtonIN thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartReadButtonINTask */
void StartReadButtonINTask(void *argument)
{
  /* USER CODE BEGIN StartReadButtonINTask */
  GPIO_PinState previous_state = GPIO_PIN_SET; // unpressed
  /* Infinite loop */
  for(;;)
  {
	printf("[StartReadButtonINTask]\n");

	GPIO_PinState current_state = HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_13);

	// Detect falling edge (press event)
	if (previous_state == GPIO_PIN_SET && current_state == GPIO_PIN_RESET)
	{
	  led_frequency_hz++;
	  if (led_frequency_hz > 10) led_frequency_hz = 1;

	  printf("Button Pressed: LED Frequency = %d Hz\n", led_frequency_hz);

	  // Simple debounce delay
	  osDelay(200);
	}

	previous_state = current_state;
	osDelay(20);
  }
  /* USER CODE END StartReadButtonINTask */
}

/* USER CODE BEGIN Header_StartLEDResetTask */
/**
* @brief Function implementing the LEDReset_Task thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartLEDResetTask */
void StartLEDResetTask(void *argument)
{
  /* USER CODE BEGIN StartLEDResetTask */

  /* Infinite loop */
  for(;;)
  {
	printf("[StartLEDResetTask]\n");

	GPIO_PinState signal = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_10); // Read D2

	if (signal == GPIO_PIN_SET)  // If signal is HIGH
	{
	  led_frequency_hz = 1;
	  printf("D2 HIGH detected, LED frequency reset to 1Hz.\n");
	  osDelay(200); // debounce delay
	}

	osDelay(10);
  }
  /* USER CODE END StartLEDResetTask */
}

/* USER CODE BEGIN Header_StartDisplayTask */
/**
* @brief Function implementing the Display_Task thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartDisplayTask */
void StartDisplayTask(void *argument)
{
  /* USER CODE BEGIN StartDisplayTask */

  ssd1306_init();
  ssd1306_clear();

  char buffer[32];
  /* Infinite loop */
  for(;;)
  {
	  printf("[StartDisplayTask]\n");

      //ssd1306_clear();

      snprintf(buffer, sizeof(buffer), "Speed: %.1f km/h", calculated_speed);

      // Set OLED page and column for text
      ssd1306_send_command(0xB0); // Page 0
      ssd1306_send_command(0x00); // Column low
      ssd1306_send_command(0x10); // Column high

      ssd1306_write_string(buffer);  // Display it!

      osDelay(100);
  }
  /* USER CODE END StartDisplayTask */
}

/* USER CODE BEGIN Header_StartHallSensorTask */
/**
* @brief Function implementing the SensorRead_Task thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartHallSensorTask */
void StartHallSensorTask(void *argument)
{
  /* USER CODE BEGIN StartHallSensorTask */

	GPIO_PinState current_state, last_state = GPIO_PIN_RESET;
  /* Infinite loop */
  for(;;)
  {
	//printf("[StartHallSensorTask]\n");

	current_state = HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_8);

	// Detect rising edge (0 → 1)
	if (last_state == GPIO_PIN_RESET && current_state == GPIO_PIN_SET)
	{
		uint32_t now = HAL_GetTick();
		uint32_t delta = now - last_pulse_time;

		if (delta > DEBOUNCE_TIME_MS && delta < 5000)
		{
			float time_sec = delta / 1000.0f;
			float raw_speed = (wheel_circumference_m / time_sec) * 3.6f;

			if (raw_speed <= 120.0f) {
				calculated_speed = apply_speed_filter(raw_speed);
			}
		}
		last_pulse_time = now;
	}

	// Set speed to zero if no pulse for a while
	if (HAL_GetTick() - last_pulse_time > 2000) {
		calculated_speed = 0.0f;
	}

	last_state = current_state;
	osDelay(10);

  }
  /* USER CODE END StartHallSensorTask */
}

/**
  * @brief  Period elapsed callback in non blocking mode
  * @note   This function is called  when TIM6 interrupt took place, inside
  * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
  * a global variable "uwTick" used as application time base.
  * @param  htim : TIM handle
  * @retval None
  */






//My Display Functions BEGIN
//fonts
const uint8_t Font_6x8[][6] = {
  [0x20] = {0x00,0x00,0x00,0x00,0x00,0x00}, // space
  ['S'] = {0x7E,0x89,0x89,0x89,0x72,0x00},
  ['p'] = {0xFE,0x90,0x90,0x90,0x60,0x00},
  ['e'] = {0x7C,0x92,0x92,0x92,0x4C,0x00},
  ['d'] = {0x60,0x90,0x90,0x90,0xFE,0x00},
  [':'] = {0x00,0x6C,0x6C,0x00,0x00,0x00},
  ['.'] = {0x00,0x60,0x60,0x00,0x00,0x00},
  ['f'] = {0x10,0xFC,0x12,0x02,0x04,0x00},
  ['k'] = {0xFE,0x10,0x28,0x44,0x82,0x00},
  ['m'] = {0xFC,0x04,0xF8,0x04,0xF8,0x00},
  ['h'] = {0xFE,0x10,0x10,0x10,0xE0,0x00},
  ['0'] = {0x7C,0xA2,0x92,0x8A,0x7C,0x00},
  ['1'] = {0x00,0x84,0xFE,0x80,0x00,0x00},
  ['2'] = {0x84,0xC2,0xA2,0x92,0x8C,0x00},
  ['3'] = {0x42,0x82,0x92,0x92,0x6C,0x00},
  ['4'] = {0x30,0x28,0x24,0xFE,0x20,0x00},
  ['5'] = {0x4E,0x8A,0x8A,0x8A,0x72,0x00},
  ['6'] = {0x7C,0x8A,0x8A,0x8A,0x70,0x00},
  ['7'] = {0x02,0xE2,0x12,0x0A,0x06,0x00},
  ['8'] = {0x6C,0x92,0x92,0x92,0x6C,0x00},
  ['9'] = {0x0C,0x92,0x92,0x92,0x7C,0x00}
};


		void ssd1306_send_command(uint8_t cmd)
		{
			uint8_t data[2] = {0x00, cmd};
			HAL_I2C_Master_Transmit(&hi2c1, SSD1306_ADDR, data, 2, HAL_MAX_DELAY);
		}

		void ssd1306_send_data(uint8_t* data, size_t size)
		{
			uint8_t buffer[129];
			buffer[0] = 0x40;
			memcpy(&buffer[1], data, size);
			HAL_I2C_Master_Transmit(&hi2c1, SSD1306_ADDR, buffer, size + 1, HAL_MAX_DELAY);
		}

		void ssd1306_init(void)
		{
			HAL_Delay(100);
			uint8_t init_cmds[] = {
				0xAE, 0xD5, 0x80, 0xA8, 0x3F, 0xD3, 0x00,
				0x40, 0x8D, 0x14, 0x20, 0x00, 0xA1, 0xC8,
				0xDA, 0x12, 0x81, 0x7F, 0xD9, 0xF1, 0xDB,
				0x40, 0xA4, 0xA6, 0xAF
			};
			for (uint8_t i = 0; i < sizeof(init_cmds); i++)
				ssd1306_send_command(init_cmds[i]);
		}

		void ssd1306_clear(void)
		{
			uint8_t zero[128] = {0};
			for (uint8_t page = 0; page < 8; page++) {
				ssd1306_send_command(0xB0 + page);
				ssd1306_send_command(0x00);
				ssd1306_send_command(0x10);
				ssd1306_send_data(zero, 128);
			}
		}

		void ssd1306_draw_square(void)
		{
			uint8_t line[128] = {0};
			for (int i = 30; i < 90; i++) line[i] = 0xFF;

			ssd1306_send_command(0xB3);  // Page 3
			ssd1306_send_command(0x00);
			ssd1306_send_command(0x10);
			ssd1306_send_data(line, 128);
		}


		//write the character
		void ssd1306_write_char(char c)
		{
		    if (c < 0x20 || c > 0x7F) c = ' ';
		    ssd1306_send_data((uint8_t*)Font_6x8[(int)c], 6);
		}

		void ssd1306_write_string(const char* str)
		{
		    while (*str) ssd1306_write_char(*str++);
		}

//My Display Functions END






void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  /* USER CODE BEGIN Callback 0 */

  /* USER CODE END Callback 0 */
  if (htim->Instance == TIM6)
  {
    HAL_IncTick();
  }
  /* USER CODE BEGIN Callback 1 */

  /* USER CODE END Callback 1 */
}

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
