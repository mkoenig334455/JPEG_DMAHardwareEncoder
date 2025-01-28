/**
  ******************************************************************************
  * @file           : JPEG/JPEG_EncodingFromXSPI_DMA/Appli/Src/encode_dma.c
  * @brief          : This file provides routines for JPEG Encoding from memory with
  *                   DMA method.
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

/* Includes ------------------------------------------------------------------*/
#include "jpeg_utils.h"
#include "encode_dma.h"
#include <string.h>
/** @addtogroup  STM32H7RSxx_HAL_Examples
  * @{
  */

/** @addtogroup JPEG_EncodingFromFLASH_DMA
  * @{
  */

/* Private typedef -----------------------------------------------------------*/
typedef struct
{
  uint8_t State;
  uint8_t *DataBuffer;
  uint32_t DataBufferSize;

}JPEG_Data_BufferTypeDef;

/* Private define ------------------------------------------------------------*/
#if (JPEG_RGB_FORMAT == JPEG_ARGB8888)
#define BYTES_PER_PIXEL    4
#elif (JPEG_RGB_FORMAT == JPEG_RGB888)
#define BYTES_PER_PIXEL    3
#elif (JPEG_RGB_FORMAT == JPEG_RGB565)
#define BYTES_PER_PIXEL    2
#endif

#define CHUNK_SIZE_IN   ((uint32_t)(MAX_INPUT_WIDTH * BYTES_PER_PIXEL * MAX_INPUT_LINES))
#define CHUNK_SIZE_OUT  ((uint32_t) (4096))

#define JPEG_BUFFER_EMPTY       0
#define JPEG_BUFFER_FULL        1

/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
JPEG_RGBToYCbCr_Convert_Function pRGBToYCbCr_Convert_Function;

uint8_t MCU_Data_IntBuffer0[CHUNK_SIZE_IN];
uint8_t MCU_Data_InBuffer1[CHUNK_SIZE_IN];

uint8_t JPEG_Data_OutBuffer0[CHUNK_SIZE_OUT];
uint8_t JPEG_Data_OutBuffer1[CHUNK_SIZE_OUT];

JPEG_Data_BufferTypeDef Jpeg_OUT_BufferTab = {JPEG_BUFFER_EMPTY , JPEG_Data_OutBuffer0 , 0};

JPEG_Data_BufferTypeDef Jpeg_IN_BufferTab = {JPEG_BUFFER_EMPTY , MCU_Data_IntBuffer0, 0};

uint32_t MCU_TotalNb                = 0;
uint32_t MCU_BlockIndex             = 0;
__IO uint32_t Jpeg_HWEncodingEnd         = 0;


__IO uint32_t Output_Is_Paused      = 0;
__IO uint32_t Input_Is_Paused       = 0;

JPEG_ConfTypeDef Conf;

uint32_t * pJpegBuffer; //FIL *pJpegFile;

uint32_t RGB_InputImageIndex;
uint32_t RGB_InputImageSize_Bytes;
uint32_t RGB_InputImageAddress;
HAL_StatusTypeDef sts1,sts2,sts3,sts4;
/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/

/**
  * @brief  Encode_DMA
  * @param hjpeg: JPEG handle pointer
  * @param  FileName    : jpg file path for decode.
  * @param  DestAddress : ARGB destination Frame Buffer Address.
  * @retval None
  */
extern void RGB_GetInfo(JPEG_ConfTypeDef *pInfo);
//https://www.st.com/resource/en/application_note/dm00356635-hardware-jpeg-codec-peripheral-in-stm32f7677xxx-and-stm32h743534555475750a3b3b0xx-microcontrollers-stmicroelectronics.pdf
uint32_t JPEG_Encode_DMA(JPEG_HandleTypeDef *hjpeg, uint32_t RGBImageBufferAddress, uint32_t RGBImageSize_Bytes, uint32_t *jpgBufferAddress )
{
  pJpegBuffer = jpgBufferAddress;
  uint32_t DataBufferSize = 0;

  /* Reset all Global variables */
  MCU_TotalNb                = 0;
  MCU_BlockIndex             = 0;
  Jpeg_HWEncodingEnd         = 0;
  Output_Is_Paused           = 0;
  Input_Is_Paused            = 0;

  /* Get RGB Info */
  //X3_Pin_On;
  RGB_GetInfo(&Conf);

//  The next step is to select the RGB to YCbCr conversion function according to the color space and chroma
//  sampling. This is done by calling the function JPEG_GetEncodeColorConvertFunc. This function also
//  initializes necessary internals variable for the RGB to YCbCr MCU conversion according to the image
//  settings dimensions color space and chroma sampling). The parameters of this function ares as follows:
//  – JPEG_ConfTypeDef *pJpegInfo: pointer to a JPEG_ConfTypeDef structure that contains the image
//  information (color space, chroma sub-sampling, image height and width). These info must be filled by
//  the user for encoding.
//  – PEG_RGBToYCbCr_Convert_Function *pFunction: this parameter returns the pointer to the function
//  that is used to convert the RGB pixels to MCUs.
//  – uint32_t *ImageNbMCUs: this parameter is used to return to the user the total number of MCUs
//  according to image dimensions, color space and chroma sampling.
  sts1 = JPEG_GetEncodeColorConvertFunc(&Conf, &pRGBToYCbCr_Convert_Function, &MCU_TotalNb);

  /* Clear Output Buffer */
  //X4_Pin_On;
  Jpeg_OUT_BufferTab.DataBufferSize = 0;
  Jpeg_OUT_BufferTab.State = JPEG_BUFFER_EMPTY;

  /* Fill input Buffers */
  //X5_Pin_On;
  RGB_InputImageIndex = 0;
  RGB_InputImageAddress = RGBImageBufferAddress;
  RGB_InputImageSize_Bytes = RGBImageSize_Bytes;
  DataBufferSize= Conf.ImageWidth * MAX_INPUT_LINES * BYTES_PER_PIXEL;



//The conversion function can then be called to convert input image RGB pixel to YCbCr MCUs. The
//conversion function parameters are as follows:
//– uint8_t *pInBuffer: a buffer containing RGB pixels to be converted to MCUs. Due to the fact that
//MCUs correspond to 8x8 blocks of the original images, the input buffer must correspond to a multiple
//of:
//◦ 8 lines of the input RGB image in case of YCBCR 4:4:4, YCbCr 4:2:2, grayscale or CMYK.
//◦ 16 lines of the input RGB image in case of YCbCr 4:2:0.
//– uint8_t *pOutBuffer: the MCUs destination buffer. This buffer can then be used to feed the hardware
//JPEG codec.
//– uint32_t BlockIndex: the index of the first MCU in the current input buffer (pInBuffer) versus the total
//number of MCUs.
//– uint32_t DataCount: the input buffer (pInBuffer) size in bytes.
//– uint32_t *ConvertedDataCount: returns the number of converted bytes from input buffer.
//The conversion function returns the number of converted MCUs from the input buffer to the output MCUs buffer
//so it can be used for the parameter BlockIndex in the next call of this function if the conversion is done by chunks
//(not in one shot).
  if(RGB_InputImageIndex < RGB_InputImageSize_Bytes)
  {
    /* Pre-Processing */
    MCU_BlockIndex += pRGBToYCbCr_Convert_Function((uint8_t *)(RGB_InputImageAddress + RGB_InputImageIndex), Jpeg_IN_BufferTab.DataBuffer, 0, DataBufferSize,(uint32_t*)(&Jpeg_IN_BufferTab.DataBufferSize));
    Jpeg_IN_BufferTab.State = JPEG_BUFFER_FULL;

    RGB_InputImageIndex += DataBufferSize;
  }

  /* Fill Encoding Params */
  sts2 = HAL_JPEG_ConfigEncoding(hjpeg, &Conf);

  //wird vielleicht nicht gebraucht
  SCB_CleanInvalidateDCache();
  SCB_InvalidateICache();

  /* Start JPEG encoding with DMA method */
  //X6_Pin_On;
  sts3 = HAL_JPEG_Encode_DMA(hjpeg ,Jpeg_IN_BufferTab.DataBuffer ,Jpeg_IN_BufferTab.DataBufferSize ,Jpeg_OUT_BufferTab.DataBuffer ,CHUNK_SIZE_OUT);

  return 0;
}

/**
  * @brief JPEG Output Data BackGround processing .
  * @param hjpeg: JPEG handle pointer
  * @retval 1 : if JPEG processing has finiched, 0 : if JPEG processing still ongoing
  */
uint32_t cntJPEG_EncodeOutputHandler;
uint32_t JPEG_EncodeOutputHandler(JPEG_HandleTypeDef *hjpeg)
{
	cntJPEG_EncodeOutputHandler++;
	X3_Pin_Toggle;

  if(Jpeg_OUT_BufferTab.State == JPEG_BUFFER_FULL)
  {
    /* Copy encoded shunk from Jpeg_OUT_BufferTab to JpegBuffer */
    memcpy(pJpegBuffer, Jpeg_OUT_BufferTab.DataBuffer ,Jpeg_OUT_BufferTab.DataBufferSize);
    pJpegBuffer+= Jpeg_OUT_BufferTab.DataBufferSize/4;
    Jpeg_OUT_BufferTab.State = JPEG_BUFFER_EMPTY;
    Jpeg_OUT_BufferTab.DataBufferSize = 0;

    if(Jpeg_HWEncodingEnd != 0)
    {
      return 1;
    }
    else if((Output_Is_Paused == 1) && (Jpeg_OUT_BufferTab.State == JPEG_BUFFER_EMPTY))
    {
      Output_Is_Paused = 0;
      HAL_JPEG_Resume(hjpeg, JPEG_PAUSE_RESUME_OUTPUT);
    }
  }


  return 0;
}

/**
  * @brief JPEG Input Data BackGround Preprocessing .
  * @param hjpeg: JPEG handle pointer
  * @retval None
  */
uint32_t cntJPEG_EncodeInputHandler;
void JPEG_EncodeInputHandler(JPEG_HandleTypeDef *hjpeg)
{
  cntJPEG_EncodeInputHandler++;
  X4_Pin_Toggle;

  uint32_t DataBufferSize = Conf.ImageWidth * MAX_INPUT_LINES * BYTES_PER_PIXEL;

  if((Jpeg_IN_BufferTab.State == JPEG_BUFFER_EMPTY) && (MCU_BlockIndex <= MCU_TotalNb))
  {
    /* Read and reorder lines from RGB input and fill data buffer */
    if(RGB_InputImageIndex < RGB_InputImageSize_Bytes)
    {
      /* Pre-Processing */
      MCU_BlockIndex += pRGBToYCbCr_Convert_Function((uint8_t *)(RGB_InputImageAddress + RGB_InputImageIndex), Jpeg_IN_BufferTab.DataBuffer, 0, DataBufferSize, (uint32_t*)(&Jpeg_IN_BufferTab.DataBufferSize));
      Jpeg_IN_BufferTab.State = JPEG_BUFFER_FULL;
      RGB_InputImageIndex += DataBufferSize;

      if(Input_Is_Paused == 1)
      {
        Input_Is_Paused = 0;
        HAL_JPEG_ConfigInputBuffer(hjpeg,Jpeg_IN_BufferTab.DataBuffer, Jpeg_IN_BufferTab.DataBufferSize);

        HAL_JPEG_Resume(hjpeg, JPEG_PAUSE_RESUME_INPUT);
      }
    }
    else
    {
      MCU_BlockIndex++;
    }
  }
}

/**
  * @brief JPEG Get Data callback
  * @param hjpeg: JPEG handle pointer
  * @param NbEncodedData: Number of encoded (consumed) bytes from input buffer
  * @retval None
  */
uint32_t cntHAL_JPEG_GetDataCallback;
void HAL_JPEG_GetDataCallback(JPEG_HandleTypeDef *hjpeg, uint32_t NbEncodedData)
{
	cntHAL_JPEG_GetDataCallback++;
	  X5_Pin_Toggle;

  if(NbEncodedData == Jpeg_IN_BufferTab.DataBufferSize)
  {
    Jpeg_IN_BufferTab.State = JPEG_BUFFER_EMPTY;
    Jpeg_IN_BufferTab.DataBufferSize = 0;

    HAL_JPEG_Pause(hjpeg, JPEG_PAUSE_RESUME_INPUT);
    Input_Is_Paused = 1;
  }
  else
  {
    HAL_JPEG_ConfigInputBuffer(hjpeg,Jpeg_IN_BufferTab.DataBuffer + NbEncodedData, Jpeg_IN_BufferTab.DataBufferSize - NbEncodedData);
  }
}

/**
  * @brief JPEG Data Ready callback
  * @param hjpeg: JPEG handle pointer
  * @param pDataOut: pointer to the output data buffer
  * @param OutDataLength: length of output buffer in bytes
  * @retval None
  */
uint32_t cntHAL_JPEG_DataReadyCallback;
void HAL_JPEG_DataReadyCallback (JPEG_HandleTypeDef *hjpeg, uint8_t *pDataOut, uint32_t OutDataLength)
{
	cntHAL_JPEG_DataReadyCallback++;
	  X6_Pin_Toggle;

  Jpeg_OUT_BufferTab.State = JPEG_BUFFER_FULL;
  Jpeg_OUT_BufferTab.DataBufferSize = OutDataLength;

  HAL_JPEG_Pause(hjpeg, JPEG_PAUSE_RESUME_OUTPUT);
  Output_Is_Paused = 1;

  HAL_JPEG_ConfigOutputBuffer(hjpeg, Jpeg_OUT_BufferTab.DataBuffer, CHUNK_SIZE_OUT);
}

/**
  * @brief  JPEG Error callback
  * @param hjpeg: JPEG handle pointer
  * @retval None
  */
void HAL_JPEG_ErrorCallback(JPEG_HandleTypeDef *hjpeg)
{
  Error_Handler();
}

/*
  * @brief JPEG Decode complete callback
  * @param hjpeg: JPEG handle pointer
  * @retval None
  */
void HAL_JPEG_EncodeCpltCallback(JPEG_HandleTypeDef *hjpeg)
{
  Jpeg_HWEncodingEnd = 1;
}

/**
  * @}
  */

