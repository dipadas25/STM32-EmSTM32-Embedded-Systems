/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include <string.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define LSM6DSO_WHO_AM_I      0x0F
#define LSM6DSO_CTRL1_XL      0x10
#define LSM6DSO_CTRL2_G       0x11
#define LSM6DSO_CTRL3_C       0x12
#define LSM6DSO_OUT_TEMP_L    0x20
#define LSM6DSO_OUTX_L_G      0x22
#define LSM6DSO_OUTX_L_A      0x28
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
#define CS_LOW()   HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET)
#define CS_HIGH()  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET)
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
SPI_HandleTypeDef hspi1;
UART_HandleTypeDef huart3;

/* USER CODE BEGIN PV */
char uart_buf[200];
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MPU_Config(void);
static void MX_GPIO_Init(void);
static void MX_SPI1_Init(void);
static void MX_USART3_UART_Init(void);

/* USER CODE BEGIN PFP */
void UART_Print(char *msg);
void Print_Menu(void);
void LSM6DSO_WriteReg(uint8_t reg, uint8_t data);
uint8_t LSM6DSO_ReadReg(uint8_t reg);
void LSM6DSO_ReadMulti(uint8_t reg, uint8_t *data, uint16_t len);
void LSM6DSO_Init(void);
int16_t LSM6DSO_ReadTempRaw(void);
void LSM6DSO_ReadAccelRaw(int16_t *ax, int16_t *ay, int16_t *az);
void LSM6DSO_ReadGyroRaw(int16_t *gx, int16_t *gy, int16_t *gz);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

void UART_Print(char *msg)
{
  HAL_UART_Transmit(&huart3, (uint8_t *)msg, strlen(msg), HAL_MAX_DELAY);
}

void Print_Menu(void)
{
  UART_Print("\r\n============================\r\n");
  UART_Print("What do you want to check?\r\n");
  UART_Print("A = Accelerometer\r\n");
  UART_Print("G = Gyroscope\r\n");
  UART_Print("T = Temperature\r\n");
  UART_Print("M = Show Menu Again\r\n");
  UART_Print("S = Stop Output\r\n");
  UART_Print("============================\r\n");
  UART_Print("Enter choice:\r\n");
}

void LSM6DSO_WriteReg(uint8_t reg, uint8_t data)
{
  uint8_t tx[2];

  tx[0] = reg & 0x7F;   // write
  tx[1] = data;

  CS_LOW();
  HAL_SPI_Transmit(&hspi1, tx, 2, HAL_MAX_DELAY);
  CS_HIGH();
}

uint8_t LSM6DSO_ReadReg(uint8_t reg)
{
  uint8_t tx[2];
  uint8_t rx[2];

  tx[0] = reg | 0x80;   // single-byte read
  tx[1] = 0x00;

  CS_LOW();
  HAL_SPI_TransmitReceive(&hspi1, tx, rx, 2, HAL_MAX_DELAY);
  CS_HIGH();

  return rx[1];
}

void LSM6DSO_ReadMulti(uint8_t reg, uint8_t *data, uint16_t len)
{
  uint8_t addr = reg | 0xC0;   // read + auto increment

  CS_LOW();
  HAL_SPI_Transmit(&hspi1, &addr, 1, HAL_MAX_DELAY);
  HAL_SPI_Receive(&hspi1, data, len, HAL_MAX_DELAY);
  CS_HIGH();
}

void LSM6DSO_Init(void)
{
  // CTRL3_C: BDU=1, IF_INC=1
  LSM6DSO_WriteReg(LSM6DSO_CTRL3_C, 0x44);

  // Accelerometer: 104 Hz, ±2g
  LSM6DSO_WriteReg(LSM6DSO_CTRL1_XL, 0x40);

  // Gyroscope: 104 Hz, 250 dps
  LSM6DSO_WriteReg(LSM6DSO_CTRL2_G, 0x40);
}

int16_t LSM6DSO_ReadTempRaw(void)
{
  uint8_t data[2];
  LSM6DSO_ReadMulti(LSM6DSO_OUT_TEMP_L, data, 2);
  return (int16_t)((data[1] << 8) | data[0]);
}

void LSM6DSO_ReadAccelRaw(int16_t *ax, int16_t *ay, int16_t *az)
{
  uint8_t data[6];
  LSM6DSO_ReadMulti(LSM6DSO_OUTX_L_A, data, 6);

  *ax = (int16_t)((data[1] << 8) | data[0]);
  *ay = (int16_t)((data[3] << 8) | data[2]);
  *az = (int16_t)((data[5] << 8) | data[4]);
}

void LSM6DSO_ReadGyroRaw(int16_t *gx, int16_t *gy, int16_t *gz)
{
  uint8_t data[6];
  LSM6DSO_ReadMulti(LSM6DSO_OUTX_L_G, data, 6);

  *gx = (int16_t)((data[1] << 8) | data[0]);
  *gy = (int16_t)((data[3] << 8) | data[2]);
  *gz = (int16_t)((data[5] << 8) | data[4]);
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */
  uint8_t who_am_i;
  int16_t ax, ay, az;
  int16_t gx, gy, gz;
  int16_t temp_raw;
  float temp_c;
  uint8_t rx_char;
  char mode = 'S';
  /* USER CODE END 1 */

  MPU_Config();
  HAL_Init();
  SystemClock_Config();

  MX_GPIO_Init();
  MX_SPI1_Init();
  MX_USART3_UART_Init();

  /* USER CODE BEGIN 2 */
  HAL_Delay(100);

  UART_Print("LSM6DSO SPI Test Start\r\n");

  who_am_i = LSM6DSO_ReadReg(LSM6DSO_WHO_AM_I);

  sprintf(uart_buf, "WHO_AM_I = 0x%02X\r\n", who_am_i);
  UART_Print(uart_buf);

  if (who_am_i == 0x6C)
  {
    UART_Print("LSM6DSO detected successfully\r\n");
    LSM6DSO_Init();
    UART_Print("LSM6DSO init done\r\n");
    Print_Menu();
  }
  else
  {
    UART_Print("LSM6DSO not detected! Check wiring and SPI settings.\r\n");
  }
  /* USER CODE END 2 */

  while (1)
  {
    /* USER CODE BEGIN 3 */

    if (HAL_UART_Receive(&huart3, &rx_char, 1, 10) == HAL_OK)
    {
      if (rx_char == 'A' || rx_char == 'a')
      {
        mode = 'A';
        UART_Print("\r\nAccelerometer selected\r\n");
      }
      else if (rx_char == 'G' || rx_char == 'g')
      {
        mode = 'G';
        UART_Print("\r\nGyroscope selected\r\n");
      }
      else if (rx_char == 'T' || rx_char == 't')
      {
        mode = 'T';
        UART_Print("\r\nTemperature selected\r\n");
      }
      else if (rx_char == 'M' || rx_char == 'm')
      {
        Print_Menu();
      }
      else if (rx_char == 'S' || rx_char == 's')
      {
        mode = 'S';
        UART_Print("\r\nOutput stopped\r\n");
        Print_Menu();
      }
    }

    if (mode == 'A')
    {
      LSM6DSO_ReadAccelRaw(&ax, &ay, &az);
      sprintf(uart_buf, "ACC X:%6d  Y:%6d  Z:%6d\r\n", ax, ay, az);
      UART_Print(uart_buf);
      HAL_Delay(500);
    }
    else if (mode == 'G')
    {
      LSM6DSO_ReadGyroRaw(&gx, &gy, &gz);
      sprintf(uart_buf, "GYRO X:%6d  Y:%6d  Z:%6d\r\n", gx, gy, gz);
      UART_Print(uart_buf);
      HAL_Delay(500);
    }
    else if (mode == 'T')
    {
      temp_raw = LSM6DSO_ReadTempRaw();
      temp_c = 25.0f + (temp_raw / 256.0f);
      sprintf(uart_buf, "TEMP: %.2f C\r\n", temp_c);
      UART_Print(uart_buf);
      HAL_Delay(500);
    }
    else
    {
      HAL_Delay(100);
    }

    /* USER CODE END 3 */
  }
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  HAL_PWREx_ConfigSupply(PWR_LDO_SUPPLY);
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);
  while(!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {}

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_DIV1;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 4;
  RCC_OscInitStruct.PLL.PLLN = 9;
  RCC_OscInitStruct.PLL.PLLP = 2;
  RCC_OscInitStruct.PLL.PLLQ = 2;
  RCC_OscInitStruct.PLL.PLLR = 2;
  RCC_OscInitStruct.PLL.PLLRGE = RCC_PLL1VCIRANGE_3;
  RCC_OscInitStruct.PLL.PLLVCOSEL = RCC_PLL1VCOMEDIUM;
  RCC_OscInitStruct.PLL.PLLFRACN = 3072;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                              | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2
                              | RCC_CLOCKTYPE_D3PCLK1 | RCC_CLOCKTYPE_D1PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.SYSCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB3CLKDivider = RCC_APB3_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_APB1_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_APB2_DIV1;
  RCC_ClkInitStruct.APB4CLKDivider = RCC_APB4_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief SPI1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI1_Init(void)
{
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi1.Init.CLKPhase = SPI_PHASE_2EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_64;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 0x0;
  hspi1.Init.NSSPMode = SPI_NSS_PULSE_DISABLE;
  hspi1.Init.NSSPolarity = SPI_NSS_POLARITY_LOW;
  hspi1.Init.FifoThreshold = SPI_FIFO_THRESHOLD_01DATA;
  hspi1.Init.TxCRCInitializationPattern = SPI_CRC_INITIALIZATION_ALL_ZERO_PATTERN;
  hspi1.Init.RxCRCInitializationPattern = SPI_CRC_INITIALIZATION_ALL_ZERO_PATTERN;
  hspi1.Init.MasterSSIdleness = SPI_MASTER_SS_IDLENESS_00CYCLE;
  hspi1.Init.MasterInterDataIdleness = SPI_MASTER_INTERDATA_IDLENESS_00CYCLE;
  hspi1.Init.MasterReceiverAutoSusp = SPI_MASTER_RX_AUTOSUSP_DISABLE;
  hspi1.Init.MasterKeepIOState = SPI_MASTER_KEEP_IO_STATE_DISABLE;
  hspi1.Init.IOSwap = SPI_IO_SWAP_DISABLE;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief USART3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART3_UART_Init(void)
{
  huart3.Instance = USART3;
  huart3.Init.BaudRate = 115200;
  huart3.Init.WordLength = UART_WORDLENGTH_8B;
  huart3.Init.StopBits = UART_STOPBITS_1;
  huart3.Init.Parity = UART_PARITY_NONE;
  huart3.Init.Mode = UART_MODE_TX_RX;
  huart3.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart3.Init.OverSampling = UART_OVERSAMPLING_16;
  huart3.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart3.Init.ClockPrescaler = UART_PRESCALER_DIV1;
  huart3.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart3) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetTxFifoThreshold(&huart3, UART_TXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetRxFifoThreshold(&huart3, UART_RXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_DisableFifoMode(&huart3) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();

  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET);

  GPIO_InitStruct.Pin = GPIO_PIN_4;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
}

/* MPU Configuration */
void MPU_Config(void)
{
  MPU_Region_InitTypeDef MPU_InitStruct = {0};

  HAL_MPU_Disable();

  MPU_InitStruct.Enable = MPU_REGION_ENABLE;
  MPU_InitStruct.Number = MPU_REGION_NUMBER0;
  MPU_InitStruct.BaseAddress = 0x0;
  MPU_InitStruct.Size = MPU_REGION_SIZE_4GB;
  MPU_InitStruct.SubRegionDisable = 0x87;
  MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL0;
  MPU_InitStruct.AccessPermission = MPU_REGION_NO_ACCESS;
  MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_DISABLE;
  MPU_InitStruct.IsShareable = MPU_ACCESS_SHAREABLE;
  MPU_InitStruct.IsCacheable = MPU_ACCESS_NOT_CACHEABLE;
  MPU_InitStruct.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;

  HAL_MPU_ConfigRegion(&MPU_InitStruct);
  HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);
}

void Error_Handler(void)
{
  __disable_irq();
  while (1)
  {
  }
}

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line)
{
}
#endif
