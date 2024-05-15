/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
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
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32h7xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

extern void trigon();
extern void trigoff();
extern void foo();

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define CDONE_Pin GPIO_PIN_13
#define CDONE_GPIO_Port GPIOC
#define TP9_Pin GPIO_PIN_14
#define TP9_GPIO_Port GPIOC
#define TP10_Pin GPIO_PIN_15
#define TP10_GPIO_Port GPIOC
#define TP11_Pin GPIO_PIN_0
#define TP11_GPIO_Port GPIOF
#define TP12_Pin GPIO_PIN_1
#define TP12_GPIO_Port GPIOF
#define TP13_Pin GPIO_PIN_2
#define TP13_GPIO_Port GPIOF
#define TP14_Pin GPIO_PIN_3
#define TP14_GPIO_Port GPIOF
#define TP15_Pin GPIO_PIN_4
#define TP15_GPIO_Port GPIOF
#define TP16_Pin GPIO_PIN_5
#define TP16_GPIO_Port GPIOF
#define TP17_Pin GPIO_PIN_10
#define TP17_GPIO_Port GPIOF
#define TP18_Pin GPIO_PIN_1
#define TP18_GPIO_Port GPIOH
#define TP19_Pin GPIO_PIN_0
#define TP19_GPIO_Port GPIOC
#define TP20_Pin GPIO_PIN_1
#define TP20_GPIO_Port GPIOC
#define TP21_Pin GPIO_PIN_2
#define TP21_GPIO_Port GPIOC
#define TP22_Pin GPIO_PIN_3
#define TP22_GPIO_Port GPIOC
#define CRESET_N_Pin GPIO_PIN_0
#define CRESET_N_GPIO_Port GPIOA
#define TP25_Pin GPIO_PIN_1
#define TP25_GPIO_Port GPIOA
#define TP26_Pin GPIO_PIN_2
#define TP26_GPIO_Port GPIOA
#define TP28_Pin GPIO_PIN_3
#define TP28_GPIO_Port GPIOA
#define TP47_Pin GPIO_PIN_4
#define TP47_GPIO_Port GPIOA
#define TP46_Pin GPIO_PIN_5
#define TP46_GPIO_Port GPIOA
#define TP27_Pin GPIO_PIN_4
#define TP27_GPIO_Port GPIOC
#define TP29_Pin GPIO_PIN_5
#define TP29_GPIO_Port GPIOC
#define NSTATUS_Pin GPIO_PIN_11
#define NSTATUS_GPIO_Port GPIOF
#define TP30_Pin GPIO_PIN_12
#define TP30_GPIO_Port GPIOF
#define TP24_Pin GPIO_PIN_13
#define TP24_GPIO_Port GPIOF
#define TP23_Pin GPIO_PIN_14
#define TP23_GPIO_Port GPIOF
#define TP34_Pin GPIO_PIN_15
#define TP34_GPIO_Port GPIOF
#define TP33_Pin GPIO_PIN_0
#define TP33_GPIO_Port GPIOG
#define TP32_Pin GPIO_PIN_1
#define TP32_GPIO_Port GPIOG
#define TP32B11_Pin GPIO_PIN_11
#define TP32B11_GPIO_Port GPIOB
#define TP35_Pin GPIO_PIN_2
#define TP35_GPIO_Port GPIOG
#define TP36_Pin GPIO_PIN_3
#define TP36_GPIO_Port GPIOG
#define TP37_Pin GPIO_PIN_4
#define TP37_GPIO_Port GPIOG
#define TP38_Pin GPIO_PIN_5
#define TP38_GPIO_Port GPIOG
#define TP39_Pin GPIO_PIN_6
#define TP39_GPIO_Port GPIOG
#define TP40_Pin GPIO_PIN_7
#define TP40_GPIO_Port GPIOG
#define TP41_Pin GPIO_PIN_8
#define TP41_GPIO_Port GPIOG
#define TP42_Pin GPIO_PIN_8
#define TP42_GPIO_Port GPIOC
#define LED2_Pin GPIO_PIN_9
#define LED2_GPIO_Port GPIOC
#define TP43_MCO_Pin GPIO_PIN_8
#define TP43_MCO_GPIO_Port GPIOA
#define TP44_TXD_Pin GPIO_PIN_9
#define TP44_TXD_GPIO_Port GPIOA
#define TP45_RXD_Pin GPIO_PIN_10
#define TP45_RXD_GPIO_Port GPIOA
#define LED1_Pin GPIO_PIN_10
#define LED1_GPIO_Port GPIOC
#define LED0_Pin GPIO_PIN_11
#define LED0_GPIO_Port GPIOC
#define FMC_CLK_Pin GPIO_PIN_3
#define FMC_CLK_GPIO_Port GPIOD
#define PD6_SD_CLK_Pin GPIO_PIN_6
#define PD6_SD_CLK_GPIO_Port GPIOD
#define PD7_SD_CMD_SPI1_MOSI_Pin GPIO_PIN_7
#define PD7_SD_CMD_SPI1_MOSI_GPIO_Port GPIOD
#define PG9_SD_D0_SPI1_MISO_Pin GPIO_PIN_9
#define PG9_SD_D0_SPI1_MISO_GPIO_Port GPIOG
#define PG10_SD_D1_SPI1_NSS_Pin GPIO_PIN_10
#define PG10_SD_D1_SPI1_NSS_GPIO_Port GPIOG
#define PG11_SD_D2_SPI1_SCK_Pin GPIO_PIN_11
#define PG11_SD_D2_SPI1_SCK_GPIO_Port GPIOG
#define PG12_SD_D3_Pin GPIO_PIN_12
#define PG12_SD_D3_GPIO_Port GPIOG
#define WRITE_PROTECT_2_Pin GPIO_PIN_15
#define WRITE_PROTECT_2_GPIO_Port GPIOG
#define CARD_DETECT_2_Pin GPIO_PIN_6
#define CARD_DETECT_2_GPIO_Port GPIOB
#define WRITE_PROTECT_1_Pin GPIO_PIN_8
#define WRITE_PROTECT_1_GPIO_Port GPIOB
#define CARD_DETECT_1_Pin GPIO_PIN_9
#define CARD_DETECT_1_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

#define GPIONAME(X) X##_GPIO_Port, X##_Pin

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
