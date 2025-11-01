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

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "led_driver.h"
#include "button_driver.h"
#include "usart_drv.h"
#include "stm32f4xx_hal.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
typedef struct {
    uint8_t seq[8]; // последовательность битов: 0->none, 1->g,2->r,4->y (только 1 бит установлен)
    uint8_t len;
    uint32_t period_ms;
} seq_t;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define MAX_PRESET 8
#define CMD_BUF_SZ 64
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
 UART_HandleTypeDef huart6;

/* USER CODE BEGIN PV */
 static seq_t presets[MAX_PRESET+1]; // индекс 1..8
 static int active_seq = 1;
 static uint8_t seq_index[9]; // позиция в каждой seq
 static int next_user_slot = 5; // куда сохранять new (5..8 cyclic)
 static int total_presets = 4;

 static bool awaiting_period = false;
 static char pending_seq[8];
 static int pending_len = 0;

 static char linebuf[CMD_BUF_SZ];
 static int linepos = 0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART6_UART_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
static uint8_t char_to_mask(char c)
{
    if(c == 'g' || c == 'G') return 0x1;
    if(c == 'y' || c == 'Y') return 0x2;
    if(c == 'r' || c == 'R') return 0x4;
    return 0x0;
}

/* инициализация предустановленных 1..4 */
static void init_presets_defaults(void)
{
    presets[1].len = 3; presets[1].seq[0]=0x1; presets[1].seq[1]=0x2; presets[1].seq[2]=0x4; presets[1].period_ms=300;
    presets[2].len = 2; presets[2].seq[0]=0x1; presets[2].seq[1]=0x1|0x2; presets[2].period_ms=500;
    presets[3].len = 3; presets[3].seq[0]=0x2; presets[3].seq[1]=0x4; presets[3].seq[2]=0x0; presets[3].period_ms=200;
    presets[4].len = 5; presets[4].seq[0]=0x1; presets[4].seq[1]=0x2; presets[4].seq[2]=0x1; presets[4].seq[3]=0x4; presets[4].seq[4]=0x0; presets[4].period_ms=150;
}

static void uart_println(const char *s) { USART_DRV_TxStr(s); USART_DRV_TxStr("\n"); }

/* Командный буфер */
#define CMD_BUF_SZ 64

/* обработка готовой команды (строки без \r\n) */
static void process_command(const char *cmd)
{
    if(strncmp(cmd, "new ", 4) == 0) {
        const char *p = cmd+4;
        char seqchars[8]; int slen = 0;
        while(*p && slen < 8) {
            char c = *p++;
            if(c==' ' || c=='\t') continue;
            if(c=='\r' || c=='\n') break;
            if(c=='g'||c=='r'||c=='y'||c=='n' || c=='G'||c=='R'||c=='Y'||c=='N') {
                seqchars[slen++] = c;
            } else break;
        }
        if(slen < 2) { uart_println("Sequence must be 2..8 symbols (g/r/y/n)"); return; }
        uart_println("Enter period: 1-fast(200),2-medium(500),3-slow(1000)");
        awaiting_period = true;
        memcpy(pending_seq, seqchars, slen);
        pending_len = slen;
        awaiting_period = true;
    } else if(strncmp(cmd, "set interrupts on", 17) == 0) {
        USART_DRV_EnableInterrupts(true);
        uart_println("interrupts on");
    } else if(strncmp(cmd, "set interrupts off", 18) == 0) {
        USART_DRV_EnableInterrupts(false);
        uart_println("interrupts off");
    } else if(strncmp(cmd, "set ", 4) == 0) {
        int idx = atoi(cmd+4);
        if(idx >=1 && idx <= total_presets) {
            active_seq = idx;
            uart_println("ok");
        } else {
            uart_println("invalid index");
        }
    } else {
        uart_println("unknown command");
    }
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
  MX_USART6_UART_Init();
  /* USER CODE BEGIN 2 */
  LED_Init();
  BUTTON_Init();
  USART_DRV_Init(&huart6);
  USART_DRV_EnableInterrupts(true);
  USART_DRV_SetEcho(true);
  init_presets_defaults();

  uint32_t last_step_time = HAL_GetTick();
  uart_println("welcome!");
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
	BUTTON_Process();
	if(BUTTON_WasPressed()){
	 active_seq++;
	 if(active_seq > 4){
		 if(active_seq > total_presets) active_seq = 1;
	 }
	}

	// UART прием
	uint8_t ch;
	while(USART_DRV_PollGetByte(&ch)){
	 if(ch=='\n' || ch=='\r'){
		 linebuf[linepos]=0;
		 if(linepos>0){
			 if(awaiting_period){
				 if(linebuf[0]=='1'||linebuf[0]=='2'||linebuf[0]=='3'){
					 uint32_t period=500;
					 if(linebuf[0]=='1') period=200;
					 if(linebuf[0]=='2') period=500;
					 if(linebuf[0]=='3') period=1000;
					 for(int i=0;i<pending_len;i++) presets[next_user_slot].seq[i]=char_to_mask(pending_seq[i]);
					 presets[next_user_slot].len = pending_len;
					 presets[next_user_slot].period_ms = period;
					 char tmp[32];
					 snprintf(tmp,sizeof(tmp),"saved %d", next_user_slot);
					 uart_println(tmp);

					 if(next_user_slot > total_presets) total_presets = next_user_slot;

					 next_user_slot++;
					 if(next_user_slot > 8) next_user_slot = 5;
					 awaiting_period=false;
				 } else {
					 uart_println("invalid period; enter 1/2/3");
				 }
			 } else {
				 process_command(linebuf);
			 }
		 }
		 linepos=0;
	 } else {
		 if(linepos<(int)sizeof(linebuf)-1) linebuf[linepos++]=(char)ch;
	 }
	}

	// анимация
	uint32_t now = HAL_GetTick();
	if(now - last_step_time >= presets[active_seq].period_ms){
	 last_step_time = now;
	 uint8_t mask = presets[active_seq].seq[ seq_index[active_seq]%presets[active_seq].len ];
	 LED_SetMask(mask);
	 seq_index[active_seq] = (seq_index[active_seq]+1)%presets[active_seq].len;
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
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief USART6 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART6_UART_Init(void)
{

  /* USER CODE BEGIN USART6_Init 0 */

  /* USER CODE END USART6_Init 0 */

  /* USER CODE BEGIN USART6_Init 1 */

  /* USER CODE END USART6_Init 1 */
  huart6.Instance = USART6;
  huart6.Init.BaudRate = 38400;
  huart6.Init.WordLength = UART_WORDLENGTH_8B;
  huart6.Init.StopBits = UART_STOPBITS_1;
  huart6.Init.Parity = UART_PARITY_NONE;
  huart6.Init.Mode = UART_MODE_TX_RX;
  huart6.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart6.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart6) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART6_Init 2 */

  /* USER CODE END USART6_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOD, GPIO_PIN_13|GPIO_PIN_14|GPIO_PIN_15, GPIO_PIN_RESET);

  /*Configure GPIO pin : PC15 */
  GPIO_InitStruct.Pin = GPIO_PIN_15;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : PD13 PD14 PD15 */
  GPIO_InitStruct.Pin = GPIO_PIN_13|GPIO_PIN_14|GPIO_PIN_15;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

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
