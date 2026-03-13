/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : PWM Fade + UART printf (STM32F407G-DISC1 / MB997D)
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
/* ---- printf redirect (CubeIDE / GCC) ---- */
/* Keep only ONE retarget method. We'll use _write() (most reliable). */
int _write(int file, char *ptr, int len)
{
  (void)file;
  HAL_UART_Transmit(&huart2, (uint8_t*)ptr, len, HAL_MAX_DELAY);
  return len;
}
/* USER CODE END 0 */

int main(void)
{
  HAL_Init();
  SystemClock_Config();

  MX_GPIO_Init();
  MX_TIM4_Init();
  MX_USART2_UART_Init();

  /* Start PWM on TIM4 Channel 4 (PD15 Blue LED) */
  HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_4);

  HAL_Delay(100);

  printf("\r\n========================================\r\n");
  printf("STM32F407G-DISC1 PWM Control + UART\r\n");
  printf("========================================\r\n");
  printf("PWM: TIM4 CH4 on PD15 (Blue LED)\r\n");
  printf("UART: USART2 on PA2/PA3 (ST-LINK VCP)\r\n");
  printf("PWM Frequency: ~10 kHz\r\n");
  printf("Duty Cycle: 0%% to 100%%\r\n\r\n");

  while (1)
  {
    /* Fade IN */
    for (int duty = 0; duty <= 100; duty++)
    {
      uint32_t ccr = (duty >= 100) ? 99 : (uint32_t)duty; /* ARR=99 */
      __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_4, ccr);

      if (duty % 10 == 0)
      {
        printf("LED Brightness: %3d%%  [", duty);
        int bars = duty / 10;
        for (int i = 0; i < 10; i++)
          printf(i < bars ? "#" : "-");
        printf("]\r\n");
      }

      HAL_Delay(50);
    }

    printf("\r\n--- Maximum Brightness Reached ---\r\n\r\n");
    HAL_Delay(500);

    /* Fade OUT */
    for (int duty = 100; duty >= 0; duty--)
    {
      uint32_t ccr = (duty >= 100) ? 99 : (uint32_t)duty;
      __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_4, ccr);

      if (duty % 10 == 0)
      {
        printf("LED Brightness: %3d%%  [", duty);
        int bars = duty / 10;
        for (int i = 0; i < 10; i++)
          printf(i < bars ? "#" : "-");
        printf("]\r\n");
      }

      HAL_Delay(50);
    }

    printf("\r\n--- Minimum Brightness Reached ---\r\n\r\n");
    HAL_Delay(500);
  }
}

/**
  * @brief System Clock Configuration (STM32F407G-DISC1, 168MHz using HSE+PLL)
  * NOTE: Prefer CubeMX-generated clock. This one is standard for F407 (8MHz HSE).
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;

  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;   /* 168 MHz */
  RCC_OscInitStruct.PLL.PLLQ = 7;

  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
    Error_Handler();

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK |
                                RCC_CLOCKTYPE_PCLK1  | RCC_CLOCKTYPE_PCLK2;

  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider  = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4; /* 42 MHz */
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2; /* 84 MHz */

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
    Error_Handler();
}

/**
  * @brief TIM4 Initialization Function (PWM on CH4 -> PD15)
  * ARR=99 gives 0..99 compare steps => maps nicely to 0..100% in code
  */
static void MX_TIM4_Init(void)
{
  TIM_OC_InitTypeDef sConfigOC = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  htim4.Instance = TIM4;
  htim4.Init.Prescaler = 83;              /* If TIM4 clk = 84MHz => 1MHz tick */
  htim4.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim4.Init.Period = 99;                 /* 1MHz/(99+1)=10kHz PWM */
  htim4.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim4.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;

  if (HAL_TIM_PWM_Init(&htim4) != HAL_OK)
    Error_Handler();

  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim4, &sMasterConfig) != HAL_OK)
    Error_Handler();

  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;

  if (HAL_TIM_PWM_ConfigChannel(&htim4, &sConfigOC, TIM_CHANNEL_4) != HAL_OK)
    Error_Handler();

  /* IMPORTANT:
     Do NOT define HAL_TIM_MspPostInit in main.c.
     CubeMX puts it in stm32f4xx_hal_msp.c, and it will configure PD15 as AF2. */
  HAL_TIM_MspPostInit(&htim4);
}

/**
  * @brief USART2 Initialization Function (PA2 TX, PA3 RX)
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
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;

  if (HAL_UART_Init(&huart2) != HAL_OK)
    Error_Handler();
}

/**
  * @brief GPIO Initialization Function
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();

  /* Optional: PD12/PD13/PD14 as outputs (other LEDs) */
  GPIO_InitStruct.Pin = GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_14;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);
}

/**
  * @brief Error Handler
  */
void Error_Handler(void)
{
  __disable_irq();
  while (1) {}
}

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line)
{
  (void)file;
  (void)line;
}
#endif
