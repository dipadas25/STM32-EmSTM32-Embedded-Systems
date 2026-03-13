/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : PWM Fade + UART Percentage Print (STM32F407G-DISC1)
  ******************************************************************************
  */
/* USER CODE END Header */

#include "main.h"
#include <stdio.h>
#include <string.h>

/* Private variables ---------------------------------------------------------*/
TIM_HandleTypeDef htim4;
UART_HandleTypeDef huart2;

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_TIM4_Init(void);
static void MX_USART2_UART_Init(void);

/* USER CODE BEGIN 0 */
static void UART_SendString(const char *s)
{
  HAL_UART_Transmit(&huart2, (uint8_t*)s, (uint16_t)strlen(s), HAL_MAX_DELAY);
}
/* USER CODE END 0 */

int main(void)
{
  HAL_Init();
  SystemClock_Config();

  MX_GPIO_Init();
  MX_TIM4_Init();
  MX_USART2_UART_Init();

  HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_1);

  UART_SendString("\r\n=== PWM Fade Started ===\r\n");

  uint32_t period = htim4.Init.Period;
  int last_printed_percent = -1;

  while (1)
  {
    /* Fade in */
    for (uint32_t duty = 0; duty <= period; duty += 2)
    {
      __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_1, duty);

      int percent = (int)((duty * 100UL) / period);

      /* Print only each 5% change (reduce spam) */
      if (percent % 5 == 0 && percent != last_printed_percent)
      {
        char msg[64];
        snprintf(msg, sizeof(msg), "LED Brightness: %d%%\r\n", percent);
        UART_SendString(msg);
        last_printed_percent = percent;
      }

      HAL_Delay(5);
    }

    /* Fade out */
    for (int32_t duty = (int32_t)period; duty >= 0; duty -= 2)
    {
      __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_1, (uint32_t)duty);

      int percent = (int)(((uint32_t)duty * 100UL) / period);

      if (percent % 5 == 0 && percent != last_printed_percent)
      {
        char msg[64];
        snprintf(msg, sizeof(msg), "LED Brightness: %d%%\r\n", percent);
        UART_SendString(msg);
        last_printed_percent = percent;
      }

      HAL_Delay(5);
    }
  }
}
