/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : STM32F407G-DISC1 - Set LED brightness by UART input (0..100%)
  *                   PWM: TIM4_CH4 on PD15 (Blue LED)
  *                   UART: USART2 PA2/PA3 @115200
  *                   Clock: HSI 16MHz, PLL OFF (stable)
  *
  * HOW IT WORKS:
  * - Type a number 0..100 in serial terminal and press ENTER.
  * - LED brightness updates immediately.
  * - Works again and again.
  ******************************************************************************
  */
/* USER CODE END Header */

#include "main.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

/* Private variables ---------------------------------------------------------*/
TIM_HandleTypeDef htim4;
UART_HandleTypeDef huart2;

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_TIM4_Init(void);
static void MX_USART2_UART_Init(void);

/* USER CODE BEGIN 0 */
/* printf via UART2 */
int _write(int file, char *ptr, int len)
{
  (void)file;
  HAL_UART_Transmit(&huart2, (uint8_t*)ptr, len, HAL_MAX_DELAY);
  return len;
}

/* Read one line from UART (blocking), ends on \r or \n */
static int uart_readline(char *buf, int maxlen)
{
  int idx = 0;
  uint8_t ch;

  while (1)
  {
    if (HAL_UART_Receive(&huart2, &ch, 1, HAL_MAX_DELAY) != HAL_OK)
      return -1;

    /* Echo back (so you can see what you typed) */
    HAL_UART_Transmit(&huart2, &ch, 1, HAL_MAX_DELAY);

    if (ch == '\r' || ch == '\n')
    {
      /* finish line */
      if (idx < maxlen) buf[idx] = '\0';
      /* send newline nicely */
      const char nl[] = "\r\n";
      HAL_UART_Transmit(&huart2, (uint8_t*)nl, 2, HAL_MAX_DELAY);
      return idx;
    }

    if (idx < (maxlen - 1))
      buf[idx++] = (char)ch;
    /* else: ignore extra chars */
  }
}

/* Convert percent (0..100) -> CCR (0..ARR) */
static uint32_t percent_to_ccr(uint32_t percent, uint32_t arr)
{
  if (percent > 100) percent = 100;
  /* map 0..100 to 0..arr with rounding */
  return (percent * (arr + 1) + 50) / 100;
}

/* USER CODE END 0 */

int main(void)
{
  HAL_Init();
  SystemClock_Config();

  MX_GPIO_Init();
  MX_TIM4_Init();
  MX_USART2_UART_Init();

  /* Start PWM on TIM4 CH4 (PD15) */
  HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_4);

  setvbuf(stdout, NULL, _IONBF, 0);

  /* Turn off initially */
  __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_4, 0);

  printf("\r\n========================================\r\n");
  printf("UART Brightness Control (0-100%%)\r\n");
  printf("Board: STM32F407G-DISC1\r\n");
  printf("PWM : TIM4_CH4 PD15 (Blue LED)\r\n");
  printf("UART: USART2 115200 (ST-LINK VCP)\r\n");
  printf("Type 0..100 and press ENTER\r\n");
  printf("Example: 50\r\n");
  printf("========================================\r\n\r\n");

  char line[32];

  while (1)
  {
    printf("Enter brightness (0-100): ");

    int n = uart_readline(line, (int)sizeof(line));
    if (n <= 0)
      continue;

    /* Trim leading spaces */
    char *p = line;
    while (*p && isspace((unsigned char)*p)) p++;

    /* Validate digits only (allow spaces at end) */
    char *endp = p;
    long val = strtol(p, &endp, 10);

    while (*endp && isspace((unsigned char)*endp)) endp++;

    if (*p == '\0' || *endp != '\0')
    {
      printf("Invalid input. Type only a number 0..100.\r\n\r\n");
      continue;
    }

    if (val < 0) val = 0;
    if (val > 100) val = 100;

    uint32_t arr = __HAL_TIM_GET_AUTORELOAD(&htim4); /* should be 99 */
    uint32_t ccr = percent_to_ccr((uint32_t)val, arr);

    __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_4, ccr);

    printf("Set brightness: %ld%% (CCR=%lu / ARR=%lu)\r\n\r\n",
           val, (unsigned long)ccr, (unsigned long)arr);
  }
}

/**
  * @brief System Clock Configuration: HSI 16 MHz, PLL OFF (stable)
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_OFF;

  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
    Error_Handler();

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK |
                                RCC_CLOCKTYPE_PCLK1  | RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource   = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider  = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
    Error_Handler();
}

/**
  * @brief TIM4 Init: ~10kHz PWM on CH4 (PD15) for 16MHz clock
  *        Prescaler=15 -> 1MHz tick, Period=99 -> 10kHz
  */
static void MX_TIM4_Init(void)
{
  TIM_OC_InitTypeDef sConfigOC = {0};

  htim4.Instance = TIM4;
  htim4.Init.Prescaler = 15;
  htim4.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim4.Init.Period = 99;
  htim4.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim4.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;

  if (HAL_TIM_PWM_Init(&htim4) != HAL_OK)
    Error_Handler();

  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;

  if (HAL_TIM_PWM_ConfigChannel(&htim4, &sConfigOC, TIM_CHANNEL_4) != HAL_OK)
    Error_Handler();

  /* PD15 AF2 config must be in stm32f4xx_hal_msp.c (CubeMX) */
  HAL_TIM_MspPostInit(&htim4);
}

/**
  * @brief USART2 Init 115200 8N1 (PA2/PA3)
  */
static void MX_USART2_UART_Init(void)
{
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.Ov   erSampling = UART_OVERSAMPLING_16;

  if (HAL_UART_Init(&huart2) != HAL_OK)
    Error_Handler();
}

static void MX_GPIO_Init(void)
{
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
}

void Error_Handler(void)
{
  __disable_irq();
  while (1) {}
}
