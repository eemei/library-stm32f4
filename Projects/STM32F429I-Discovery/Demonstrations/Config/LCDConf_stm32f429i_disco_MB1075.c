/**
  ******************************************************************************
  * @file    lcdconf.c
  * @author  MCD Application Team
  * @version V1.2.1
  * @date    13-March-2015
  * @brief   This file implements the configuration for the GUI library
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT(c) 2015 STMicroelectronics</center></h2>
  *
  * Licensed under MCD-ST Liberty SW License Agreement V2, (the "License");
  * You may not use this file except in compliance with the License.
  * You may obtain a copy of the License at:
  *
  *        http://www.st.com/software_license_agreement_liberty_v2
  *
  * Unless required by applicable law or agreed to in writing, software 
  * distributed under the License is distributed on an "AS IS" BASIS, 
  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  * See the License for the specific language governing permissions and
  * limitations under the License.
  *
  ******************************************************************************
  */ 

/* Includes ------------------------------------------------------------------*/
#include "LCDConf.h"
#include "GUI_Private.h"

/** @addtogroup STEMWIN_LIBRARY
* @{
*/

/** @defgroup STEMWIN_LIBRARY
* @brief This file contains the USB Configuration
* @{
*/ 

/** @defgroup STEMWIN_LIBRARY_Private_TypesDefinitions
* @{
*/ 
/**
* @}
*/ 

/** @defgroup STEMWIN_LIBRARY_Private_Defines
* @{
*/ 
#undef  LCD_SWAP_XY
#undef  LCD_MIRROR_Y

#define LCD_SWAP_XY  1 
#define LCD_MIRROR_Y 1

#define XSIZE_PHYS 240
#define YSIZE_PHYS 320

#define NUM_BUFFERS  3 // Number of multiple buffers to be used
#define NUM_VSCREENS 1 // Number of virtual screens to be used

#define BK_COLOR GUI_DARKBLUE

#undef  GUI_NUM_LAYERS
#define GUI_NUM_LAYERS 2

#define COLOR_CONVERSION_0 GUICC_M565
#define DISPLAY_DRIVER_0   GUIDRV_LIN_16

#if (GUI_NUM_LAYERS > 1)
  #define COLOR_CONVERSION_1 GUICC_M8888I
  #define DISPLAY_DRIVER_1   GUIDRV_LIN_32
#endif

#ifndef   XSIZE_PHYS
  #error Physical X size of display is not defined!
#endif
#ifndef   YSIZE_PHYS
  #error Physical Y size of display is not defined!
#endif
#ifndef   NUM_VSCREENS
  #define NUM_VSCREENS 1
#else
  #if (NUM_VSCREENS <= 0)
    #error At least one screeen needs to be defined!
  #endif
#endif
#if (NUM_VSCREENS > 1) && (NUM_BUFFERS > 1)
  #error Virtual screens and multiple buffers are not allowed!
#endif

#define LCD_LAYER0_FRAME_BUFFER  ((uint32_t)0xD0200000)
#define LCD_LAYER1_FRAME_BUFFER  ((uint32_t)0xD0400000)

/**
* @}
*/ 


/** @defgroup STEMWIN_LIBRARY_Private_Macros
* @{
*/ 
/* Redirect bulk conversion to DMA2D routines */
#define DEFINEDMA2D_COLORCONVERSION(PFIX, PIXELFORMAT)                                                             \
static void Color2IndexBulk_##PFIX##DMA2D(LCD_COLOR * pColor, void * pIndex, U32 NumItems, U8 SizeOfIndex) { \
  DMA2D_Color2IndexBulk(pColor, pIndex, NumItems, SizeOfIndex, PIXELFORMAT);                                         \
}                                                                                                                   \
static void Index2ColorBulk_##PFIX##DMA2D(void * pIndex, LCD_COLOR * pColor, U32 NumItems, U8 SizeOfIndex) { \
  DMA2D_Index2ColorBulk(pIndex, pColor, NumItems, SizeOfIndex, PIXELFORMAT);  \
}
  
/**
* @}
*/ 


/** @defgroup STEMWIN_LIBRARY_Private_Variables
* @{
*/
LTDC_HandleTypeDef          hltdc;  
static DMA2D_HandleTypeDef  hdma2d;
static LCD_LayerPropTypedef layer_prop[GUI_NUM_LAYERS];

static const LCD_API_COLOR_CONV * apColorConvAPI[] = 
{
  COLOR_CONVERSION_0,
#if GUI_NUM_LAYERS > 1
  COLOR_CONVERSION_1,
#endif
};

/**
* @}
*/ 


/** @defgroup STEMWIN_LIBRARY_Private_FunctionPrototypes
* @{
*/ 
static uint32_t LCD_LL_GetPixelformat(uint32_t LayerIndex);
static void     DMA2D_CopyBuffer(uint32_t LayerIndex, void * pSrc, void * pDst, uint32_t xSize, uint32_t ySize, uint32_t OffLineSrc, uint32_t OffLineDst);
static void     DMA2D_FillBuffer(uint32_t LayerIndex, void * pDst, uint32_t xSize, uint32_t ySize, uint32_t OffLine, uint32_t ColorIndex);
static void     LCD_LL_Init(void);
static void     LCD_LL_LayerInit(uint32_t LayerIndex);

static void     CUSTOM_CopyBuffer(int32_t LayerIndex, int32_t IndexSrc, int32_t IndexDst);
static void     CUSTOM_CopyRect(int32_t LayerIndex, int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t xSize, int32_t ySize);
static void     CUSTOM_FillRect(int32_t LayerIndex, int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint32_t PixelIndex);

static void BSP_LCD_DrawBitmap8bpp(int32_t LayerIndex, int32_t x, int32_t y, U8 const * p, int32_t xSize, int32_t ySize, int32_t BytesPerLine);
static void BSP_LCD_DrawBitmap16bpp(int32_t LayerIndex, int32_t x, int32_t y, U16 const * p, int32_t xSize, int32_t ySize, int32_t BytesPerLine);
static uint32_t GetBufferSize(uint32_t LayerIndex);
/**
* @}
*/ 

/** @defgroup STEMWIN_LIBRARY_Private_Functions
* @{
*/ 

/*******************************************************************************
                       LTDC and DMA2D BSP Routines
*******************************************************************************/
/**
  * @brief DMA2D MSP Initialization 
  *        This function configures the hardware resources used in this example: 
  *           - Peripheral's clock enable
  *           - Peripheral's GPIO Configuration  
  * @param hdma2d: DMA2D handle pointer
  * @retval None
  */
void HAL_DMA2D_MspInit(DMA2D_HandleTypeDef *hdma2d)
{  
  /* Enable peripheral */
  __HAL_RCC_DMA2D_CLK_ENABLE();   
}

/**
  * @brief DMA2D MSP De-Initialization 
  *        This function frees the hardware resources used in this example:
  *          - Disable the Peripheral's clock
  * @param hdma2d: DMA2D handle pointer
  * @retval None
  */
void HAL_DMA2D_MspDeInit(DMA2D_HandleTypeDef *hdma2d)
{
  /* Enable DMA2D reset state */
  __HAL_RCC_DMA2D_FORCE_RESET();
  
  /* Release DMA2D from reset state */ 
  __HAL_RCC_DMA2D_RELEASE_RESET();
}

/**
  * @brief LTDC MSP Initialization 
  *        This function configures the hardware resources used in this example: 
  *           - Peripheral's clock enable
  *           - Peripheral's GPIO Configuration  
  * @param hltdc: LTDC handle pointer
  * @retval None
  */
void HAL_LTDC_MspInit(LTDC_HandleTypeDef *hltdc)
{  
  GPIO_InitTypeDef GPIO_Init_Structure;
  
  /* Enable peripherals and GPIO Clocks */  
  /* Enable the LTDC Clock */
  __HAL_RCC_LTDC_CLK_ENABLE();
  
  /* Enable GPIO Clock */
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOF_CLK_ENABLE();
  __HAL_RCC_GPIOG_CLK_ENABLE();
  
  /* Configure peripheral GPIO */

  /* GPIOs Configuration */
  /*
   +------------------------+-----------------------+----------------------------+
   +                       LCD pins assignment                                   +
   +------------------------+-----------------------+----------------------------+
   |  LCD_TFT R2 <-> PC.10  |  LCD_TFT G2 <-> PA.06 |  LCD_TFT B2 <-> PD.06      |
   |  LCD_TFT R3 <-> PB.00  |  LCD_TFT G3 <-> PG.10 |  LCD_TFT B3 <-> PG.11      |
   |  LCD_TFT R4 <-> PA.11  |  LCD_TFT G4 <-> PB.10 |  LCD_TFT B4 <-> PG.12      |
   |  LCD_TFT R5 <-> PA.12  |  LCD_TFT G5 <-> PB.11 |  LCD_TFT B5 <-> PA.03      |
   |  LCD_TFT R6 <-> PB.01  |  LCD_TFT G6 <-> PC.07 |  LCD_TFT B6 <-> PB.08      |
   |  LCD_TFT R7 <-> PG.06  |  LCD_TFT G7 <-> PD.03 |  LCD_TFT B7 <-> PB.09      |
   -------------------------------------------------------------------------------
            |  LCD_TFT HSYNC <-> PC.06  | LCDTFT VSYNC <->  PA.04 |
            |  LCD_TFT CLK   <-> PG.07  | LCD_TFT DE   <->  PF.10 |
             -----------------------------------------------------
  */

  /* GPIOA configuration */
  GPIO_Init_Structure.Pin = GPIO_PIN_3 | GPIO_PIN_4 | GPIO_PIN_6 |
                           GPIO_PIN_11 | GPIO_PIN_12;
  GPIO_Init_Structure.Mode = GPIO_MODE_AF_PP;
  GPIO_Init_Structure.Pull = GPIO_NOPULL;
  GPIO_Init_Structure.Speed = GPIO_SPEED_FAST;
  GPIO_Init_Structure.Alternate= GPIO_AF14_LTDC;
  HAL_GPIO_Init(GPIOA, &GPIO_Init_Structure);

 /* GPIOB configuration */
  GPIO_Init_Structure.Pin = GPIO_PIN_8 | \
                           GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11;
  HAL_GPIO_Init(GPIOB, &GPIO_Init_Structure);

 /* GPIOC configuration */
  GPIO_Init_Structure.Pin = GPIO_PIN_6 | GPIO_PIN_7 | GPIO_PIN_10;
  HAL_GPIO_Init(GPIOC, &GPIO_Init_Structure);

 /* GPIOD configuration */
  GPIO_Init_Structure.Pin = GPIO_PIN_3 | GPIO_PIN_6;
  HAL_GPIO_Init(GPIOD, &GPIO_Init_Structure);
  
 /* GPIOF configuration */
  GPIO_Init_Structure.Pin = GPIO_PIN_10;
  HAL_GPIO_Init(GPIOF, &GPIO_Init_Structure);     

 /* GPIOG configuration */  
  GPIO_Init_Structure.Pin = GPIO_PIN_6 | GPIO_PIN_7 | \
                           GPIO_PIN_11;
  HAL_GPIO_Init(GPIOG, &GPIO_Init_Structure);
 
  /* GPIOB configuration */  
  GPIO_Init_Structure.Pin = GPIO_PIN_0 | GPIO_PIN_1;
  GPIO_Init_Structure.Alternate= GPIO_AF9_LTDC;
  HAL_GPIO_Init(GPIOB, &GPIO_Init_Structure);

  /* GPIOG configuration */  
  GPIO_Init_Structure.Pin = GPIO_PIN_10 | GPIO_PIN_12;
  HAL_GPIO_Init(GPIOG, &GPIO_Init_Structure);

  /* Set LTDC Interrupt to the lowest priority */
  HAL_NVIC_SetPriority(LTDC_IRQn, 0xF, 0);
  
  /* Enable LTDC Interrupt */
  HAL_NVIC_EnableIRQ(LTDC_IRQn);
}

/**
  * @brief LTDC MSP De-Initialization 
  *        This function frees the hardware resources used in this example:
  *          - Disable the Peripheral's clock
  * @param hltdc: LTDC handle pointer
  * @retval None
  */
void HAL_LTDC_MspDeInit(LTDC_HandleTypeDef *hltdc)
{
  /* Reset peripherals */
  /* Enable LTDC reset state */
  __HAL_RCC_LTDC_FORCE_RESET();
  
  /* Release LTDC from reset state */ 
  __HAL_RCC_LTDC_RELEASE_RESET();
}

/**
  * @brief  Line Event callback.
  * @param  hltdc: pointer to a LTDC_HandleTypeDef structure that contains
  *                the configuration information for the specified LTDC.
  * @retval None
  */
void HAL_LTDC_LineEvenCallback(LTDC_HandleTypeDef *hltdc)
{
  uint32_t Addr;
  uint32_t layer;

  for (layer = 0; layer < GUI_NUM_LAYERS; layer++) 
  {
    if (layer_prop[layer].pending_buffer >= 0) 
    {
      /* Calculate address of buffer to be used  as visible frame buffer */
      Addr = layer_prop[layer].address + layer_prop[layer].xSize * layer_prop[layer].ySize * layer_prop[layer].pending_buffer * layer_prop[layer].BytesPerPixel;
      HAL_LTDC_SetAddress(hltdc, Addr, layer);
	  
	  __HAL_LTDC_RELOAD_CONFIG(hltdc);
	  
      /* Notify STemWin that buffer is used */
      GUI_MULTIBUF_ConfirmEx(layer, layer_prop[layer].pending_buffer);

      /* Clear pending buffer flag of layer */
      layer_prop[layer].pending_buffer = -1;
    }
  }
  
  HAL_LTDC_ProgramLineEvent(hltdc, 0);
}

/*******************************************************************************
                          Display configuration
*******************************************************************************/
/**
  * @brief  Called during the initialization process in order to set up the
  *          display driver configuration
  * @param  None
  * @retval None
  */
void LCD_X_Config(void) 
{
  uint32_t i;
  
  LCD_LL_Init();
  
  /* At first initialize use of multiple buffers on demand */
#if (NUM_BUFFERS > 1)
    for (i = 0; i < GUI_NUM_LAYERS; i++) 
    {
      GUI_MULTIBUF_ConfigEx(i, NUM_BUFFERS);
    }
#endif

  /* Set display driver and color conversion for 1st layer */
  GUI_DEVICE_CreateAndLink(DISPLAY_DRIVER_0, COLOR_CONVERSION_0, 0, 0);
  
  /* Set size of 1st layer */
  if (LCD_GetSwapXYEx(0)) {
    LCD_SetSizeEx (0, YSIZE_PHYS, XSIZE_PHYS);
    LCD_SetVSizeEx(0, YSIZE_PHYS * NUM_VSCREENS, XSIZE_PHYS);
  } else {
    LCD_SetSizeEx (0, XSIZE_PHYS, YSIZE_PHYS);
    LCD_SetVSizeEx(0, XSIZE_PHYS, YSIZE_PHYS * NUM_VSCREENS);
  }
  
#if (GUI_NUM_LAYERS > 1)
    /* Set display driver and color conversion for 2nd layer */
    GUI_DEVICE_CreateAndLink(DISPLAY_DRIVER_1, COLOR_CONVERSION_1, 0, 1);
    /* Set size of 2nd layer */
    if (LCD_GetSwapXYEx(1)) {
      LCD_SetSizeEx (1, YSIZE_PHYS, XSIZE_PHYS);
      LCD_SetVSizeEx(1, YSIZE_PHYS * NUM_VSCREENS, XSIZE_PHYS);
    } else {
      LCD_SetSizeEx (1, XSIZE_PHYS, YSIZE_PHYS);
      LCD_SetVSizeEx(1, XSIZE_PHYS, YSIZE_PHYS * NUM_VSCREENS);
    }
#endif
  
  /*Initialize GUI Layer structure */
  layer_prop[0].address = LCD_LAYER0_FRAME_BUFFER;
#if (GUI_NUM_LAYERS > 1)
  layer_prop[1].address = LCD_LAYER1_FRAME_BUFFER;     
#endif
    
   /* Setting up VRam address and custom functions for CopyBuffer-, CopyRect- and FillRect operations */
  for (i = 0; i < GUI_NUM_LAYERS; i++) 
  {
    layer_prop[i].pColorConvAPI = (LCD_API_COLOR_CONV *)apColorConvAPI[i];
    
    layer_prop[i].pending_buffer = -1;
    
    /* Set VRAM address */
    LCD_SetVRAMAddrEx(i, (void *)(layer_prop[i].address));
    
    /* Remember color depth for further operations */
    layer_prop[i].BytesPerPixel = LCD_GetBitsPerPixelEx(i) >> 3;
    
    /* Set custom functions for several operations */
    LCD_SetDevFunc(i, LCD_DEVFUNC_COPYBUFFER, (void(*)(void))CUSTOM_CopyBuffer);
    LCD_SetDevFunc(i, LCD_DEVFUNC_COPYRECT,   (void(*)(void))CUSTOM_CopyRect);
    
    /* Filling via DMA2D does only work with 16bpp or more */
    if (LCD_LL_GetPixelformat(i) <= LTDC_PIXEL_FORMAT_ARGB4444) 
    {
      LCD_SetDevFunc(i, LCD_DEVFUNC_FILLRECT, (void(*)(void))CUSTOM_FillRect);
      LCD_SetDevFunc(i, LCD_DEVFUNC_DRAWBMP_8BPP, (void(*)(void))BSP_LCD_DrawBitmap8bpp);
    }
    
    /* Set up drawing routine for 16bpp bitmap using DMA2D */
    if (LCD_LL_GetPixelformat(i) == LTDC_PIXEL_FORMAT_RGB565) {
      LCD_SetDevFunc(i, LCD_DEVFUNC_DRAWBMP_16BPP, (void(*)(void))BSP_LCD_DrawBitmap16bpp);     /* Set up drawing routine for 16bpp bitmap using DMA2D. Makes only sense with RGB565 */
    }
  }
}

/**
  * @brief  This function is called by the display driver for several purposes.
  *         To support the according task the routine needs to be adapted to
  *         the display controller. Please note that the commands marked with
  *         'optional' are not cogently required and should only be adapted if
  *         the display controller supports these features
  * @param  LayerIndex: Index of layer to be configured 
  * @param  Cmd       :Please refer to the details in the switch statement below
  * @param  pData     :Pointer to a LCD_X_DATA structure
  * @retval Status (-1 : Error,  0 : Ok)
  */
int  LCD_X_DisplayDriver(unsigned LayerIndex, unsigned Cmd, void * pData)
{
  int r = 0;
  int addr;
  int xPos, yPos;
  int Color;
    
  switch (Cmd) 
  {
  case LCD_X_INITCONTROLLER: 
    LCD_LL_LayerInit(LayerIndex);
    break;

  case LCD_X_SETORG: 
    addr = layer_prop[LayerIndex].address + ((LCD_X_SETORG_INFO *)pData)->yPos * layer_prop[LayerIndex].xSize * layer_prop[LayerIndex].BytesPerPixel;
    HAL_LTDC_SetAddress(&hltdc, addr, LayerIndex);
    break;

  case LCD_X_SHOWBUFFER: 
    layer_prop[LayerIndex].pending_buffer = ((LCD_X_SHOWBUFFER_INFO *)pData)->Index;
    break;

  case LCD_X_SETLUTENTRY: 
    HAL_LTDC_ConfigCLUT(&hltdc, (uint32_t *)&(((LCD_X_SETLUTENTRY_INFO *)pData)->Color), 1, LayerIndex);
    break;

  case LCD_X_ON: 
    __HAL_LTDC_ENABLE(&hltdc);
    break;

  case LCD_X_OFF: 
    __HAL_LTDC_DISABLE(&hltdc);
    break;
    
  case LCD_X_SETVIS:
    if(((LCD_X_SETVIS_INFO *)pData)->OnOff  == ENABLE )
    {
      __HAL_LTDC_LAYER_ENABLE(&hltdc, LayerIndex); 
    }
    else
    {
      __HAL_LTDC_LAYER_DISABLE(&hltdc, LayerIndex); 
    }
    __HAL_LTDC_RELOAD_CONFIG(&hltdc); 
    break;
    
  case LCD_X_SETPOS: 
    HAL_LTDC_SetWindowPosition(&hltdc, 
                               ((LCD_X_SETPOS_INFO *)pData)->xPos, 
                               ((LCD_X_SETPOS_INFO *)pData)->yPos, 
                               LayerIndex);
    break;

  case LCD_X_SETSIZE:
    GUI_GetLayerPosEx(LayerIndex, &xPos, &yPos);
    layer_prop[LayerIndex].xSize = ((LCD_X_SETSIZE_INFO *)pData)->xSize;
    layer_prop[LayerIndex].ySize = ((LCD_X_SETSIZE_INFO *)pData)->ySize;
    HAL_LTDC_SetWindowPosition(&hltdc, xPos, yPos, LayerIndex);
    break;

  case LCD_X_SETALPHA:
    HAL_LTDC_SetAlpha(&hltdc, ((LCD_X_SETALPHA_INFO *)pData)->Alpha, LayerIndex);
    break;

  case LCD_X_SETCHROMAMODE:
    if(((LCD_X_SETCHROMAMODE_INFO *)pData)->ChromaMode != 0)
    {
      HAL_LTDC_EnableColorKeying(&hltdc, LayerIndex);
    }
    else
    {
      HAL_LTDC_DisableColorKeying(&hltdc, LayerIndex);      
    }
    break;

  case LCD_X_SETCHROMA:

    Color = ((((LCD_X_SETCHROMA_INFO *)pData)->ChromaMin & 0xFF0000) >> 16) |\
             (((LCD_X_SETCHROMA_INFO *)pData)->ChromaMin & 0x00FF00) |\
            ((((LCD_X_SETCHROMA_INFO *)pData)->ChromaMin & 0x0000FF) << 16);
    
    HAL_LTDC_ConfigColorKeying(&hltdc, Color, LayerIndex);
    break;

  default:
    r = -1;
  }
  return r;
}

/*******************************************************************************
                          Static code
*******************************************************************************/
/**
  * @brief  Initialize the LCD Controller.
  * @param  LayerIndex : layer Index.
  * @retval None
  */
static void LCD_LL_LayerInit(uint32_t LayerIndex)
{
  LTDC_LayerCfgTypeDef             layer_cfg;
  
  if (LayerIndex < GUI_NUM_LAYERS) 
  {
  /* Layer configuration */
    layer_cfg.WindowX0 = 0;
    layer_cfg.WindowX1 = XSIZE_PHYS;
    layer_cfg.WindowY0 = 0;
    layer_cfg.WindowY1 = YSIZE_PHYS; 
    layer_cfg.PixelFormat = LCD_LL_GetPixelformat(LayerIndex);
    layer_cfg.FBStartAdress = layer_prop[LayerIndex].address;
    layer_cfg.Alpha = 255;
    layer_cfg.Alpha0 = 0;
    layer_cfg.Backcolor.Blue = 0;
    layer_cfg.Backcolor.Green = 0;
    layer_cfg.Backcolor.Red = 0;
    layer_cfg.BlendingFactor1 = LTDC_BLENDING_FACTOR1_PAxCA;
    layer_cfg.BlendingFactor2 = LTDC_BLENDING_FACTOR1_PAxCA;
    layer_cfg.ImageWidth = XSIZE_PHYS;
    layer_cfg.ImageHeight = YSIZE_PHYS;
    HAL_LTDC_ConfigLayer(&hltdc, &layer_cfg, LayerIndex);  
    
    /* Enable LUT on demand */
    if (LCD_GetBitsPerPixelEx(LayerIndex) <= 8) 
    {
      /* Enable usage of LUT for all modes with <= 8bpp*/
      HAL_LTDC_EnableCLUT(&hltdc, LayerIndex);
    } 
  }
}

/**
  * @brief  Initialize the LCD Controller.
  * @param  LayerIndex : layer Index.
  * @retval None
  */
static void LCD_LL_Init(void) 
{
  static RCC_PeriphCLKInitTypeDef  PeriphClkInitStruct;
      /* DeInit */
      HAL_LTDC_DeInit(&hltdc);
      
      /* Initializaton of ILI9341 component*/
      ili9341_Init();
      
      /* Set LCD Timings */
      hltdc.Init.HorizontalSync = 9;
      hltdc.Init.VerticalSync = 1;
      hltdc.Init.AccumulatedHBP = 29;
      hltdc.Init.AccumulatedVBP = 3;  
      hltdc.Init.AccumulatedActiveH = 323;
      hltdc.Init.AccumulatedActiveW = 269;
      hltdc.Init.TotalHeigh = 327;
      hltdc.Init.TotalWidth = 279;
      
      /* background value */
      hltdc.Init.Backcolor.Blue = 0;
      hltdc.Init.Backcolor.Green = 0;
      hltdc.Init.Backcolor.Red = 0;
      
      /* LCD clock configuration */
      /* PLLSAI_VCO Input = HSE_VALUE/PLL_M = 1 MHz */
      /* PLLSAI_VCO Output = PLLSAI_VCO Input * PLLSAIN = 192 MHz */
      /* PLLLCDCLK = PLLSAI_VCO Output/PLLSAIR = 192/4 = 48 MHz */
      /* LTDC clock frequency = PLLLCDCLK / LTDC_PLLSAI_DIVR_8 = 48/8 = 6 MHz */
      PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_LTDC;
      PeriphClkInitStruct.PLLSAI.PLLSAIN = 192;
      PeriphClkInitStruct.PLLSAI.PLLSAIR = 4;
      PeriphClkInitStruct.PLLSAIDivR = RCC_PLLSAIDIVR_8;
      HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct);
      
      /* Polarity */
      hltdc.Init.HSPolarity = LTDC_HSPOLARITY_AL;
      hltdc.Init.VSPolarity = LTDC_VSPOLARITY_AL; 
      hltdc.Init.DEPolarity = LTDC_DEPOLARITY_AL;  
      hltdc.Init.PCPolarity = LTDC_PCPOLARITY_IPC;
      hltdc.Instance = LTDC;
      
      HAL_LTDC_Init(&hltdc);
      HAL_LTDC_ProgramLineEvent(&hltdc, 0);
      
	  /* Configure the DMA2D  default mode */ 
  hdma2d.Init.Mode         = DMA2D_R2M;
  hdma2d.Init.ColorMode    = DMA2D_RGB565;
  hdma2d.Init.OutputOffset = 0x0;     

  hdma2d.Instance          = DMA2D; 

  if(HAL_DMA2D_Init(&hdma2d) != HAL_OK)
  {
    while (1);
  }
}   

/**
  * @brief  Return Pixel format for a given layer
  * @param  LayerIndex : Layer Index 
  * @retval Status ( 0 : 0k , 1: error)
  */
static uint32_t LCD_LL_GetPixelformat(uint32_t LayerIndex)
{
  const LCD_API_COLOR_CONV * pColorConvAPI;

  if (LayerIndex >= GUI_NUM_LAYERS) 
  {
    return 0;
  }
  pColorConvAPI = layer_prop[LayerIndex].pColorConvAPI;
  
  if (pColorConvAPI == GUICC_M8888I) 
  {
    return LTDC_PIXEL_FORMAT_ARGB8888;
  } 
  else if (pColorConvAPI == GUICC_M888) 
  {
    return LTDC_PIXEL_FORMAT_RGB888;
  } 
  else if (pColorConvAPI == GUICC_M565) 
  {
    return LTDC_PIXEL_FORMAT_RGB565;
  } 
  else if (pColorConvAPI == GUICC_M1555I) 
  {
    return LTDC_PIXEL_FORMAT_ARGB1555;
  } 
  else if (pColorConvAPI == GUICC_M4444I) 
  {
    return LTDC_PIXEL_FORMAT_ARGB4444;
  } 
  else if (pColorConvAPI == GUICC_8666) 
  {
    return LTDC_PIXEL_FORMAT_L8;
  } 
  else if (pColorConvAPI == GUICC_1616I) 
  {
    return LTDC_PIXEL_FORMAT_AL44;
  } 
  else if (pColorConvAPI == GUICC_88666I) 
  {
    return LTDC_PIXEL_FORMAT_AL88;
  }
  while (1);
}

/*********************************************************************
*
*       CopyBuffer
*/
static void DMA2D_CopyBuffer(uint32_t LayerIndex, void * pSrc, void * pDst, uint32_t xSize, uint32_t ySize, uint32_t OffLineSrc, uint32_t OffLineDst)
{
  uint32_t PixelFormat;

  PixelFormat = LCD_LL_GetPixelformat(LayerIndex);
  DMA2D->CR      = 0x00000000UL | (1 << 9);  
  
  /* Set up pointers */
  DMA2D->FGMAR   = (uint32_t)pSrc;                       
  DMA2D->OMAR    = (uint32_t)pDst;                       
  DMA2D->FGOR    = OffLineSrc;                      
  DMA2D->OOR     = OffLineDst; 
  
  /* Set up pixel format */  
  DMA2D->FGPFCCR = PixelFormat;  
  
  /*  Set up size */
  DMA2D->NLR     = (uint32_t)(xSize << 16) | (U16)ySize; 
  
  DMA2D->CR     |= DMA2D_CR_START;   
 
  /* Wait until transfer is done */
  while (DMA2D->CR & DMA2D_CR_START)
  {
  }
}

/*********************************************************************
*
*       FillBuffer
*/
static void DMA2D_FillBuffer(uint32_t LayerIndex, void * pDst, uint32_t xSize, uint32_t ySize, uint32_t OffLine, uint32_t ColorIndex) 
{
 uint32_t PixelFormat;

  PixelFormat = LCD_LL_GetPixelformat(LayerIndex);

  /* Set up mode */
  DMA2D->CR      = 0x00030000UL | (1 << 9);        
  DMA2D->OCOLR   = ColorIndex;                     

  /* Set up pointers */
  DMA2D->OMAR    = (uint32_t)pDst;                      

  /* Set up offsets */
  DMA2D->OOR     = OffLine;                        

  /* Set up pixel format */
  DMA2D->OPFCCR  = PixelFormat;                    

  /*  Set up size */
  DMA2D->NLR     = (uint32_t)(xSize << 16) | (U16)ySize;
    
  DMA2D->CR     |= DMA2D_CR_START; 
  
  /* Wait until transfer is done */
  while (DMA2D->CR & DMA2D_CR_START) 
  {
  }
}


/*********************************************************************
*
*       GetBufferSize
*/
static uint32_t GetBufferSize(uint32_t LayerIndex) 
{
  uint32_t BufferSize;

  BufferSize = layer_prop[LayerIndex].xSize * layer_prop[LayerIndex].ySize * layer_prop[LayerIndex].BytesPerPixel;
  return BufferSize;
}

/*********************************************************************
*
*       CUSTOM_CopyBuffer
*/
static void CUSTOM_CopyBuffer(int32_t LayerIndex, int32_t IndexSrc, int32_t IndexDst) {
  uint32_t BufferSize, AddrSrc, AddrDst;

  BufferSize = GetBufferSize(LayerIndex);
  AddrSrc    = layer_prop[LayerIndex].address + BufferSize * IndexSrc;
  AddrDst    = layer_prop[LayerIndex].address + BufferSize * IndexDst;
  DMA2D_CopyBuffer(LayerIndex, (void *)AddrSrc, (void *)AddrDst, layer_prop[LayerIndex].xSize, layer_prop[LayerIndex].ySize, 0, 0);
  layer_prop[LayerIndex].buffer_index = IndexDst;
}

/*********************************************************************
*
*       CUSTOM_CopyRect
*/
static void CUSTOM_CopyRect(int32_t LayerIndex, int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t xSize, int32_t ySize) 
{
  int32_t BufferSize, AddrSrc, AddrDst;

  BufferSize = GetBufferSize(LayerIndex);
  AddrSrc = layer_prop[LayerIndex].address + BufferSize * layer_prop[LayerIndex].pending_buffer + (y0 * layer_prop[LayerIndex].xSize + x0) * layer_prop[LayerIndex].BytesPerPixel;
  AddrDst = layer_prop[LayerIndex].address + BufferSize * layer_prop[LayerIndex].pending_buffer + (y1 * layer_prop[LayerIndex].xSize + x1) * layer_prop[LayerIndex].BytesPerPixel;
  DMA2D_CopyBuffer(LayerIndex, (void *)AddrSrc, (void *)AddrDst, xSize, ySize, layer_prop[LayerIndex].xSize - xSize, 0);
}

/*********************************************************************
*
*       CUSTOM_FillRect
*/
static void CUSTOM_FillRect(int32_t LayerIndex, int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint32_t PixelIndex) 
{
  uint32_t BufferSize, AddrDst;
  int32_t xSize, ySize;

  if (GUI_GetDrawMode() == GUI_DM_XOR) 
  {
    LCD_SetDevFunc(LayerIndex, LCD_DEVFUNC_FILLRECT, NULL);
    LCD_FillRect(x0, y0, x1, y1);
    LCD_SetDevFunc(LayerIndex, LCD_DEVFUNC_FILLRECT, (void(*)(void))CUSTOM_FillRect);
  } 
  else 
  {
    xSize = x1 - x0 + 1;
    ySize = y1 - y0 + 1;
    BufferSize = GetBufferSize(LayerIndex);
    AddrDst = layer_prop[LayerIndex].address + BufferSize * layer_prop[LayerIndex].buffer_index + (y0 * layer_prop[LayerIndex].xSize + x0) * layer_prop[LayerIndex].BytesPerPixel;
    DMA2D_FillBuffer(LayerIndex, (void *)AddrDst, xSize, ySize, layer_prop[LayerIndex].xSize - xSize, PixelIndex);
  }
}

/**
  * @brief  
  * @param  pSrc
  * @param  pDst
  * @param  OffSrc
  * @param  OffDst
  * @param  PixelFormatDst
  * @param  xSize
  * @param  ySize
  * @retval None
  */
static void DMA2D_DrawBitmapL8(void * pSrc, void * pDst,  uint32_t OffSrc, uint32_t OffDst, uint32_t PixelFormatDst, uint32_t xSize, uint32_t ySize)
{
  /* Set up mode */
  DMA2D->CR      = 0x00010000UL | (1 << 9);         /* Control Register (Memory to memory with pixel format conversion and TCIE) */
  
  /* Set up pointers */
  DMA2D->FGMAR   = (uint32_t)pSrc;                       /* Foreground Memory Address Register (Source address) */
  DMA2D->OMAR    = (uint32_t)pDst;                       /* Output Memory Address Register (Destination address) */
  
  /* Set up offsets */
  DMA2D->FGOR    = OffSrc;                          /* Foreground Offset Register (Source line offset) */
  DMA2D->OOR     = OffDst;                          /* Output Offset Register (Destination line offset) */
  
  /* Set up pixel format */
  DMA2D->FGPFCCR = LTDC_PIXEL_FORMAT_L8;             /* Foreground PFC Control Register (Defines the input pixel format) */
  DMA2D->OPFCCR  = PixelFormatDst;                  /* Output PFC Control Register (Defines the output pixel format) */
  
  /* Set up size */
  DMA2D->NLR     = (uint32_t)(xSize << 16) | ySize;      /* Number of Line Register (Size configuration of area to be transfered) */
  
  /* Execute operation */
  DMA2D->CR     |= DMA2D_CR_START;                               /* Start operation */
  
  /* Wait until transfer is done */
  while (DMA2D->CR & DMA2D_CR_START)
  {
  }
}

/**
  * @brief  
  * @param  LayerIndex
  * @param  x
  * @param  y
  * @param  p
  * @param  xSize
  * @param  ySize
  * @param  BytesPerLine
  * @retval None
  */
static void BSP_LCD_DrawBitmap16bpp(int32_t LayerIndex, int32_t x, int32_t y, U16 const * p, int32_t xSize, int32_t ySize, int32_t BytesPerLine)
{
  uint32_t BufferSize, AddrDst;
  int32_t OffLineSrc, OffLineDst;

  BufferSize = GetBufferSize(LayerIndex);
  AddrDst = layer_prop[LayerIndex].address + BufferSize * layer_prop[LayerIndex].buffer_index + (y * layer_prop[LayerIndex].xSize + x) * layer_prop[LayerIndex].BytesPerPixel;
  OffLineSrc = (BytesPerLine / 2) - xSize;
  OffLineDst = layer_prop[LayerIndex].xSize - xSize;
  DMA2D_CopyBuffer(LayerIndex, (void *)p, (void *)AddrDst, xSize, ySize, OffLineSrc, OffLineDst);
}

/**
  * @brief  
  * @param  LayerIndex
  * @param  x
  * @param  y
  * @param  p
  * @param  xSize
  * @param  ySize
  * @param  BytesPerLine
  * @retval None
  */
static void BSP_LCD_DrawBitmap8bpp(int32_t LayerIndex, int32_t x, int32_t y, U8 const * p, int32_t xSize, int32_t ySize, int32_t BytesPerLine)
{
  uint32_t BufferSize, AddrDst;
  int32_t OffLineSrc, OffLineDst;
  uint32_t PixelFormat;

  BufferSize = GetBufferSize(LayerIndex);
  AddrDst = layer_prop[LayerIndex].address + BufferSize * layer_prop[LayerIndex].buffer_index + (y * layer_prop[LayerIndex].xSize + x) * layer_prop[LayerIndex].BytesPerPixel;
  OffLineSrc = BytesPerLine - xSize;
  OffLineDst = layer_prop[LayerIndex].xSize - xSize;
  PixelFormat = LCD_LL_GetPixelformat(LayerIndex);
  DMA2D_DrawBitmapL8((void *)p, (void *)AddrDst, OffLineSrc, OffLineDst, PixelFormat, xSize, ySize);
}

/*********************************************************************
*
*       CUSTOM_FillRect
*/
void RestoreDefaultFillProcess(void) 
{
  LCD_SetDevFunc(1, LCD_DEVFUNC_FILLRECT, (void(*)(void))CUSTOM_FillRect);
}
/*************************** End of file ****************************/
