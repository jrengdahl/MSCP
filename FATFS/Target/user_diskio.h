/* USER CODE BEGIN Header */
/**
 ******************************************************************************
  * @file    user_diskio.h
  * @brief   This file contains the common defines and functions prototypes for
  *          the user_diskio driver.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
 /* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __USER_DISKIO_H
#define __USER_DISKIO_H

#ifdef __cplusplus
 extern "C" {
#endif

/* USER CODE BEGIN 0 */

/* Includes ------------------------------------------------------------------*/

#include "ffconf.h"
#include "diskio.h"

/* Exported types ------------------------------------------------------------*/

/**
  * @brief  Disk IO Driver structure definition
  */

typedef struct
  {
  DSTATUS (*disk_initialize) (BYTE);                            /*!< Initialize Disk Drive                     */
  DSTATUS (*disk_status)     (BYTE);                            /*!< Get Disk Status                           */
  DRESULT (*disk_read)       (BYTE, BYTE*, DWORD, UINT);        /*!< Read Sector(s)                            */
  DRESULT (*disk_write)      (BYTE, const BYTE*, DWORD, UINT);  /*!< Write Sector(s) when _USE_WRITE = 0       */
  DRESULT (*disk_ioctl)      (BYTE, BYTE, void*);               /*!< I/O control operation when _USE_IOCTL = 1 */
  }Diskio_drvTypeDef;

/**
  * @brief  Global Disk IO Drivers structure definition
  */

typedef struct
  {
  uint8_t                 is_initialized[_VOLUMES];
  const Diskio_drvTypeDef *drv[_VOLUMES];
  volatile uint8_t        nbr;
  }Disk_drvTypeDef;


/* Exported constants --------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */

extern Diskio_drvTypeDef  USER_Driver;

/* USER CODE END 0 */

#ifdef __cplusplus
}
#endif

#endif /* __USER_DISKIO_H */
