/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
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
#include "stm32g4xx_hal.h"

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

void HAL_TIM_MspPostInit(TIM_HandleTypeDef *htim);

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define AI3_Pin GPIO_PIN_0
#define AI3_GPIO_Port GPIOA
#define AI2_Pin GPIO_PIN_6
#define AI2_GPIO_Port GPIOA
#define AI1_Pin GPIO_PIN_7
#define AI1_GPIO_Port GPIOA
#define COMPI1_Pin GPIO_PIN_1
#define COMPI1_GPIO_Port GPIOB
#define GSM_PWR_Pin GPIO_PIN_12
#define GSM_PWR_GPIO_Port GPIOB
#define GSM_RST_Pin GPIO_PIN_13
#define GSM_RST_GPIO_Port GPIOB
#define PWMI2_Pin GPIO_PIN_11
#define PWMI2_GPIO_Port GPIOA
#define DO2_Pin GPIO_PIN_6
#define DO2_GPIO_Port GPIOB
#define DO1_Pin GPIO_PIN_7
#define DO1_GPIO_Port GPIOB
#define DI1_Pin GPIO_PIN_8
#define DI1_GPIO_Port GPIOB
#define DI2_Pin GPIO_PIN_9
#define DI2_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
