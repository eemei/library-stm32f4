/**
  ******************************************************************************
  * @file    DMA2D/DMA2D_MemToMemWithLCD/Src/main.c
  * @author  MCD Application Team
  * @version V1.2.1
  * @date    13-March-2015
  * @brief   This example provides a description of how to configure 
  *          DMA2D peripheral in Memory to Memory transfer mode 
  *          and display the result on LCD.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT(c) 2015 STMicroelectronics</center></h2>
  *
  * Redistribution and use in source and binary forms, with or without modification,
  * are permitted provided that the following conditions are met:
  *   1. Redistributions of source code must retain the above copyright notice,
  *      this list of conditions and the following disclaimer.
  *   2. Redistributions in binary form must reproduce the above copyright notice,
  *      this list of conditions and the following disclaimer in the documentation
  *      and/or other materials provided with the distribution.
  *   3. Neither the name of STMicroelectronics nor the names of its contributors
  *      may be used to endorse or promote products derived from this software
  *      without specific prior written permission.
  *
  * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
  * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
  * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "RGB565_150x150.h"

/** @addtogroup STM32F4xx_HAL_Examples
  * @{
  */

/** @addtogroup DMA2D_MemToMemWithLCD
  * @{
  */ 

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
DMA2D_HandleTypeDef     Dma2dHandle;

/* DMA2D output address and Input for LCD */
uint32_t aBufferResult[11250];

/* Private function prototypes -----------------------------------------------*/
static void DMA2D_Config(void);
static void LCD_Config(void);
static void TransferError(DMA2D_HandleTypeDef* dma2dHandle);
static void TransferComplete(DMA2D_HandleTypeDef* dma2dHandle);
static void SystemClock_Config(void);
static void Error_Handler(void);

/* Private functions ---------------------------------------------------------*/

/**
  * @brief   Main program
  * @param  None
  * @retval None
  */
int main(void)
{
  /* STM32F4xx HAL library initialization:
       - Configure the Flash prefetch, instruction and Data caches
       - Configure the Systick to generate an interrupt each 1 msec
       - Set NVIC Group Priority to 4
       - Global MSP (MCU Support Package) initialization
     */
  HAL_Init();
  
  /* Configure the system clock to 180 MHz */
  SystemClock_Config(); 

  /* Configure LED1, LED2 and LED3 */
  BSP_LED_Init(LED1);
  BSP_LED_Init(LED2);
  BSP_LED_Init(LED3);
  
  /*##-1- LCD Configuration ##################################################*/
  LCD_Config();  
  
  /*##-2- DMA2D configuration ################################################*/
  DMA2D_Config();
  
  /*##-3- Start DMA2D transfer ###############################################*/  
  if(HAL_DMA2D_Start_IT(&Dma2dHandle, (uint32_t)&RGB565_150x150, (uint32_t)&aBufferResult, 150, 150) != HAL_OK)
  {
    /* Initialization Error */
    Error_Handler(); 
  }

  /* Infinite loop */  
  while (1)
  {
  }
}

/**
  * @brief DMA2D configuration.
  * @note  This function Configure tha DMA2D peripheral :
  *        1) Configure the transfer mode : memory to memory
  *        2) Configure the output color mode as RGB565
  *        3) Configure the transfer from FLASH to SRAM   
  *        4) Configure the data size : 150x150 (pixels)  
  * @retval
  *  None
  */

static void DMA2D_Config(void)
{
  /* Configure the DMA2D Mode, Color Mode and output offset */  
  Dma2dHandle.Init.Mode         = DMA2D_M2M;
  Dma2dHandle.Init.ColorMode    = DMA2D_RGB565;
  Dma2dHandle.Init.OutputOffset = 0x0;     

  /* DMA2D Callbacks Configuration */
  Dma2dHandle.XferCpltCallback  = TransferComplete;
  Dma2dHandle.XferErrorCallback = TransferError;
  
  /* Foreground Configuration */
  Dma2dHandle.LayerCfg[1].AlphaMode = DMA2D_NO_MODIF_ALPHA;
  Dma2dHandle.LayerCfg[1].InputAlpha = 0xFF;
  Dma2dHandle.LayerCfg[1].InputColorMode = CM_RGB565;
  Dma2dHandle.LayerCfg[1].InputOffset = 0x0;

  Dma2dHandle.Instance          = DMA2D; 
  
  /* DMA2D Initialization */
  if(HAL_DMA2D_Init(&Dma2dHandle) != HAL_OK) 
  {
    /* Initialization Error */
    Error_Handler(); 
  }
  
    if(HAL_DMA2D_ConfigLayer(&Dma2dHandle, 1) != HAL_OK) 
  {
    /* Initialization Error */
    Error_Handler(); 
  }  
}

/**
  * @brief LCD configuration.
  * @note  This function Configure tha LTDC peripheral :
  *        1) Configure the Pixel Clock for the LCD
  *        2) Configure the LTDC Timing and Polarity
  *        3) Configure the LTDC Layer 2 :
  *           - RGB565 as pixel format  
  *           - The frame buffer is located at internal RAM : The output of DMA2D transfer
  *           - The Layer size configuration : 150x150  
  * @retval
  *  None
  */
static void LCD_Config(void)
{
  static RCC_PeriphCLKInitTypeDef  PeriphClkInitStruct;  
  static LTDC_HandleTypeDef        hltdc_F;
         LTDC_LayerCfgTypeDef      pLayerCfg;
  
/* LTDC Initialization -------------------------------------------------------*/
  
  /* Polarity configuration */
  /* Initialize the horizontal synchronization polarity as active low */
  hltdc_F.Init.HSPolarity = LTDC_HSPOLARITY_AL;
  /* Initialize the vertical synchronization polarity as active low */ 
  hltdc_F.Init.VSPolarity = LTDC_VSPOLARITY_AL; 
  /* Initialize the data enable polarity as active low */ 
  hltdc_F.Init.DEPolarity = LTDC_DEPOLARITY_AL; 
  /* Initialize the pixel clock polarity as input pixel clock */  
  hltdc_F.Init.PCPolarity = LTDC_PCPOLARITY_IPC;
  
  /* Timing configuration */
  /* Horizontal synchronization width = Hsync - 1 */  
  hltdc_F.Init.HorizontalSync = 40;
  /* Vertical synchronization height = Vsync - 1 */
  hltdc_F.Init.VerticalSync = 9;
  /* Accumulated horizontal back porch = Hsync + HBP - 1 */
  hltdc_F.Init.AccumulatedHBP = 42;
  /* Accumulated vertical back porch = Vsync + VBP - 1 */
  hltdc_F.Init.AccumulatedVBP = 11; 
  /* Accumulated active width = Hsync + HBP + Active Width - 1 */ 
  hltdc_F.Init.AccumulatedActiveH = 283;
  /* Accumulated active height = Vsync + VBP + Active Heigh - 1 */
  hltdc_F.Init.AccumulatedActiveW = 522;
  /* Total height = Vsync + VBP + Active Heigh + VFP - 1 */
  hltdc_F.Init.TotalHeigh = 285;
  /* Total width = Hsync + HBP + Active Width + HFP - 1 */
  hltdc_F.Init.TotalWidth = 524;

  /* LCD clock configuration */
  /* PLLSAI_VCO Input = HSE_VALUE/PLL_M = 1 MHz */
  /* PLLSAI_VCO Output = PLLSAI_VCO Input * PLLSAIN = 192 MHz */
  /* PLLLCDCLK = PLLSAI_VCO Output/PLLSAIR = 192/5 = 38.4 MHz */
  /* LTDC clock frequency = PLLLCDCLK / LTDC_PLLSAI_DIVR_4 = 38.4/4 = 9.6MHz */
  PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_LTDC;
  PeriphClkInitStruct.PLLSAI.PLLSAIN = 192;
  PeriphClkInitStruct.PLLSAI.PLLSAIR = 5;
  PeriphClkInitStruct.PLLSAIDivR = RCC_PLLSAIDIVR_4;
  HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct); 
  
  /* Configure R,G,B component values for LCD background color */
  hltdc_F.Init.Backcolor.Blue = 0;
  hltdc_F.Init.Backcolor.Green = 0;
  hltdc_F.Init.Backcolor.Red = 0;

  hltdc_F.Instance = LTDC;
  
/* Layer1 Configuration ------------------------------------------------------*/
  
  /* Windowing configuration */ 
  /*
     WindowX0 = Horizontal start
     WindowX1 = Horizontal stop      
     WindowY0 = Vertical start 
     WindowY1 = Vertical stop      
  */
  pLayerCfg.WindowX0 = 165;
  pLayerCfg.WindowX1 = 315;
  pLayerCfg.WindowY0 = 61;
  pLayerCfg.WindowY1 = 211;
  
  /* Pixel Format configuration*/ 
  pLayerCfg.PixelFormat = LTDC_PIXEL_FORMAT_RGB565;
  
  /* Start Address configuration : frame buffer is located at FLASH memory */
  pLayerCfg.FBStartAdress = (uint32_t)&aBufferResult;
  
  /* Alpha constant (255 totally opaque) */
  pLayerCfg.Alpha = 255;
  
  /* Default Color configuration (configure A,R,G,B component values) */
  pLayerCfg.Alpha0 = 0;
  pLayerCfg.Backcolor.Blue = 0;
  pLayerCfg.Backcolor.Green = 0;
  pLayerCfg.Backcolor.Red = 0;
  
  /* Configure blending factors */
  pLayerCfg.BlendingFactor1 = LTDC_BLENDING_FACTOR1_CA;
  pLayerCfg.BlendingFactor2 = LTDC_BLENDING_FACTOR2_CA;
  
  /* Configure the number of lines and number of pixels per line */
  pLayerCfg.ImageWidth = 150;
  pLayerCfg.ImageHeight = 150;  
  
  /* Configure the LTDC */  
  if(HAL_LTDC_Init(&hltdc_F) != HAL_OK)
  {
    /* Initialization Error */
    Error_Handler(); 
  }
  
  /* Configure the Layer */
  if(HAL_LTDC_ConfigLayer(&hltdc_F, &pLayerCfg, 1) != HAL_OK)
  {
    /* Initialization Error */
    Error_Handler(); 
  }  
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
  *            HSE Frequency(Hz)              = 25000000
  *            PLL_M                          = 25
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
  RCC_OscInitStruct.PLL.PLLM = 25;
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
  /* Turn LED3 on */
  BSP_LED_On(LED3);
  while(1)
  {
  }
}

/**
  * @brief  DMA2D Transfer completed callback
  * @param  hdma2d: DMA2D handle. 
  * @note   This example shows a simple way to report end of DMA2D transfer, and 
  *         you can add your own implementation. 
  * @retval None
  */
static void TransferComplete(DMA2D_HandleTypeDef *hdma2d)
{
  /* Turn LED1 on */
  BSP_LED_On(LED1);
}

/**
  * @brief  DMA2D error callbacks
  * @param  hdma2d: DMA2D handle
  * @note   This example shows a simple way to report DMA2D transfer error, and you can
  *         add your own implementation.
  * @retval None
  */
static void TransferError(DMA2D_HandleTypeDef *hdma2d)
{
  /* Turn LED2 on */
  BSP_LED_On(LED2);
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
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

/**
  * @}
  */

/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
