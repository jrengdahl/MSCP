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

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define CDONE_Pin GPIO_PIN_13
#define CDONE_GPIO_Port GPIOC
#define SPI5_SCK_Pin GPIO_PIN_7
#define SPI5_SCK_GPIO_Port GPIOF
#define SPI5_MISO_Pin GPIO_PIN_8
#define SPI5_MISO_GPIO_Port GPIOF
#define SPI5_MOSI_Pin GPIO_PIN_9
#define SPI5_MOSI_GPIO_Port GPIOF
#define CRESET_N_Pin GPIO_PIN_0
#define CRESET_N_GPIO_Port GPIOA
#define PA3_Pin GPIO_PIN_3
#define PA3_GPIO_Port GPIOA
#define PA4_Pin GPIO_PIN_4
#define PA4_GPIO_Port GPIOA
#define NSTATUS_Pin GPIO_PIN_11
#define NSTATUS_GPIO_Port GPIOF
#define LED2_Pin GPIO_PIN_9
#define LED2_GPIO_Port GPIOC
#define SPI3_NSS_Pin GPIO_PIN_15
#define SPI3_NSS_GPIO_Port GPIOA
#define LED1_Pin GPIO_PIN_10
#define LED1_GPIO_Port GPIOC
#define LED0_Pin GPIO_PIN_11
#define LED0_GPIO_Port GPIOC
#define FPGA_IRQ_Pin GPIO_PIN_3
#define FPGA_IRQ_GPIO_Port GPIOD
#define SPI1_NSS_Pin GPIO_PIN_10
#define SPI1_NSS_GPIO_Port GPIOG

/* USER CODE BEGIN Private defines */

#define GPIONAME(X) X##_GPIO_Port, X##_Pin
#define trigon(x)  HAL_GPIO_WritePin(LED##x##_GPIO_Port, LED##x##_Pin, GPIO_PIN_SET)
#define trigoff(x) HAL_GPIO_WritePin(LED##x##_GPIO_Port, LED##x##_Pin, GPIO_PIN_RESET);

extern void LED_on();
extern void LED_off();



/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
