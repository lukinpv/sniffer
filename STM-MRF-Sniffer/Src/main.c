/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "WM.h"
#include "zb_mrf24j40.h"

/* Private variables ---------------------------------------------------------*/
uint8_t GUI_Initialized = 0;

/* SPI */
SPI_HandleTypeDef SpiHandle;
uint8_t aTxBuffer[BUFFERSIZE];
uint8_t aRxBuffer[BUFFERSIZE];
uint32_t SPITimeout = 5000;

char hdigits[16] = {'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};

uint8_t channel = 11;

/* Private function prototypes -----------------------------------------------*/
static void BSP_Config(void);
static void SystemClock_Config(void);

void Error_Handler(void);

void setNewChannel(void);
void byteToHex(uint8_t c, char* h);
void board_spi_test(void);

/* Private functions ---------------------------------------------------------*/

void setNewChannel(void) {
  zb_transceiver_select_channel(channel);
  
  GUI_Clear();
  GUI_DispString("Channel: ");
  GUI_DispChar('0'+channel/10);
  GUI_DispChar('0'+channel%10);
  GUI_DispString(" - 0x");
  ZB_READ_LONG_REG(ZB_LREG_RFCTRL0);
  RXPrint();
  GUI_DispNextLine();
}

void byteToHex(uint8_t c, char* h) {
  h[0]=hdigits[c >> 4];
  h[1]=hdigits[c & 0x0F];
}

/**
  * @brief EXTI line detection callbacks
  * @param GPIO_Pin: Specifies the pins connected EXTI line
  * @retval None
  */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  if(GPIO_Pin == KEY_BUTTON_PIN)
  {
    /* next channel */
    if(++channel>26)
      channel = 11; 
    setNewChannel();
    BSP_LED_Off(LED3);
  }
  else if(GPIO_Pin == OLMX_INT_PIN)
  {
    /* receiving packet */
    BSP_LED_On(LED3);
    zb_read_rx_fifo();
    ZB_READ_SHORT_REG(ZB_SREG_ISRSTS); //clearing interrapt register on mrf
    GUI_DispNextLine();
  }
}

/* print the receiver */
void RXPrint() {
   char i, hex[2];
   for(i=0;i<BUFFERSIZE;i++) {
     byteToHex(aRxBuffer[i],hex);
     GUI_DispChar(hex[0]);
     GUI_DispChar(hex[1]);
     GUI_DispChar(' ');
     if(GUI_GetDispPosX()+10 >= LCD_GetXSize())
       GUI_DispNextLine();
  }
}

void board_spi_test() {
  int8_t i;
  const char message[]="Test 1";
  for (i=0; i<6; i++) {
    aTxBuffer[0]=message[i];
    HAL_SPI_TransmitReceive(&SpiHandle, aTxBuffer, aRxBuffer, BUFFERSIZE, SPITimeout);
    GUI_DispChar(aRxBuffer[0]);
  }
  GUI_DispNextLine();
}

int main(void)
{
  /* STM32F4xx HAL library initialization:
       - Configure the Flash prefetch, instruction and Data caches
       - Configure the Systick to generate an interrupt each 1 msec
       - Set NVIC Group Priority to 4
       - Global MSP (MCU Support Package) initialization
     */
  HAL_Init();
  
  /* Initialize LCD, LEDs, Button and Olimex Pins */
  BSP_Config();
  
  /* Configure the system clock to 180 MHz */
  SystemClock_Config();
  
  /***********************************************************/
  
  /* Init the STemWin GUI Library */
  GUI_Init();
  GUI_Initialized = 1;
  GUI_SetFont(&GUI_Font16_1);
  
  /* Set the SPI parameters */
  SpiHandle.Instance               = SPIx;
  SpiHandle.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_16;
  SpiHandle.Init.Direction         = SPI_DIRECTION_2LINES;
  SpiHandle.Init.CLKPhase          = SPI_PHASE_1EDGE;
  SpiHandle.Init.CLKPolarity       = SPI_POLARITY_LOW;                          /* taken from stack*/
  SpiHandle.Init.CRCCalculation    = SPI_CRCCALCULATION_DISABLE;
  SpiHandle.Init.CRCPolynomial     = 7;
  SpiHandle.Init.DataSize          = SPI_DATASIZE_8BIT;
  SpiHandle.Init.FirstBit          = SPI_FIRSTBIT_MSB;
  SpiHandle.Init.NSS               = SPI_NSS_SOFT;
  SpiHandle.Init.TIMode            = SPI_TIMODE_DISABLE;
  SpiHandle.Init.Mode              = SPI_MODE_MASTER;
  
  if(HAL_SPI_Init(&SpiHandle) != HAL_OK)
  {
    GUI_DispString("HAL SPI INIT ERROR");
    Error_Handler();
  }

  INIT_MRF();
  
  //initially channel is switched 2 times, if it set to 11 (0x00)
  setNewChannel();
 
  /* Infinite loop */
 
  while (1)
  {
  }
}

/**
  * @brief  Initializes the STM32F429I-DISCO's LCD and LEDs resources.
  * @param  None
  * @retval None
  */
static void BSP_Config(void)
{
  /* Initialize STM32F429I-DISCO's LEDs */
  BSP_LED_Init(LED3);
  BSP_LED_Init(LED4);
  
  BSP_PB_Init(BUTTON_KEY, BUTTON_MODE_EXTI);
  
  /* Initializes the SDRAM device */
  BSP_SDRAM_Init();
  
  /* Enable the CRC Module */
  __HAL_RCC_CRC_CLK_ENABLE();
  
  /***************************************************/
  /* Olimex  pins configuration */
  
  /* CS & RST GPIO pins configuration */
  GPIO_InitTypeDef  GPIO_InitStruct;
  
  OLIMEX_GPIO_CLK_ENABLE();
    
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull      = GPIO_PULLUP;
  GPIO_InitStruct.Pin = OLMX_CS_PIN | OLMX_RST_PIN;
  HAL_GPIO_Init(OLMX_GPIO_PORT, &GPIO_InitStruct);
  
  HAL_GPIO_WritePin(OLMX_GPIO_PORT, OLMX_CS_PIN, GPIO_PIN_SET);   //Chip Select on Device is Disabled initially
  
  /* INT GPIO pin configuration */
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Pin = OLMX_INT_PIN;
  HAL_GPIO_Init(OLMX_INT_GPIO_PORT, &GPIO_InitStruct);
  
  /* configuring interrupt on olimex int pin */
  HAL_NVIC_SetPriority(EXTI4_IRQn, 2, 0);
  HAL_NVIC_EnableIRQ(EXTI4_IRQn);
}

/**
  * @brief  System Clock Configuration
  *         The system Clock is configured as follow : 
  *            System Clock source            = PLL (HSE)
  *            SYSCLK(Hz)                     = 180000000
  *            HCLK(Hz)                       = 180000000
  *            AHB Prescaler                  = 1
  *            APB1 Prescaler                 = 4
  *            APB2 Prescaler                 = 2
  *            HSE Frequency(Hz)              = 8000000
  *            PLL_M                          = 8
  *            PLL_N                          = 360
  *            PLL_P                          = 2
  *            PLL_Q                          = 7
  *            VDD(V)                         = 3.3
  *            Main regulator output voltage  = Scale1 mode
  *            Flash Latency(WS)              = 5
  * @param  None
  * @retval None
  */
static void SystemClock_Config(void)
{
  RCC_ClkInitTypeDef RCC_ClkInitStruct;
  RCC_OscInitTypeDef RCC_OscInitStruct;
  
  /* Enable Power Control clock */
  __HAL_RCC_PWR_CLK_ENABLE();
  
  /* The voltage scaling allows optimizing the power consumption when the device is 
     clocked below the maximum system frequency, to update the voltage scaling value 
     regarding system frequency refer to product datasheet.  */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);
  
  /* Enable HSE Oscillator and activate PLL with HSE as source */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 360;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 7;
  HAL_RCC_OscConfig(&RCC_OscInitStruct);
  
  /* Activate the Over-Drive mode */
  HAL_PWREx_EnableOverDrive();
  
  /* Select PLL as system clock source and configure the HCLK, PCLK1 and PCLK2 
  clocks dividers */
  RCC_ClkInitStruct.ClockType = (RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2);
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;  
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;  
  HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5);
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @param  None
  * @retval None
  */
static void Error_Handler(void)
{
  /* Turn LED4 on */
  BSP_LED_On(LED4);
  while(1)
  {
  }
}

/**
  * @brief  SPI error callbacks
  * @param  hspi: SPI handle
  * @note   This example shows a simple way to report transfer error, and you can
  *         add your own implementation.
  * @retval None
  */
 void HAL_SPI_ErrorCallback(SPI_HandleTypeDef *hspi)
{
  GUI_DispNextLine();
  GUI_DispString("CALLBACK ERROR");
  GUI_DispNextLine();
  /* Turn LED4 on: Transfer error in reception/transmission process */
  BSP_LED_On(LED4);
}

uint8_t spi_sync_exchange(uint8_t tx_data) {
    aTxBuffer[0] = tx_data;
    switch(HAL_SPI_TransmitReceive(&SpiHandle, aTxBuffer, aRxBuffer, BUFFERSIZE, SPITimeout))
    {
    case HAL_OK:
      break;
      
    case HAL_TIMEOUT:
      GUI_DispNextLine();
      GUI_DispString("HAL TIMEOUT ERROR");
      Error_Handler();
      break;  

    case HAL_ERROR:
      GUI_DispNextLine();
      GUI_DispString("HAL ERROR");
      Error_Handler();
      break;
    
    default:
      break;
    }
    
    return aRxBuffer[0];
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *   where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t* file, uint32_t line)
{
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

  /* Infinite loop */
  while (1)
  {
  }
}
#endif


