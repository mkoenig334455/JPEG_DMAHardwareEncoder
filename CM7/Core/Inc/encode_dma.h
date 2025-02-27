/**
  ******************************************************************************
  * @file    JPEG/JPEG_EncodingFromXSPI_DMA/Appli/Inc/encode_dma.h
  * @author  MCD Application Team
  * @brief   Header for encode_dma.c module
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2019 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __ENCODE_DMA_H
#define __ENCODE_DMA_H

/* Includes ------------------------------------------------------------------*/
#include "main.h"
uint32_t JPEG_Encode_DMA(JPEG_HandleTypeDef *hjpeg, uint32_t RGBImageBufferAddress, uint32_t RGBImageSize_Bytes, uint32_t *jpgBufferAddress);
uint32_t JPEG_EncodeOutputHandler(JPEG_HandleTypeDef *hjpeg);
void JPEG_EncodeInputHandler(JPEG_HandleTypeDef *hjpeg);

#endif /* __ENCODE_DMA_H */

