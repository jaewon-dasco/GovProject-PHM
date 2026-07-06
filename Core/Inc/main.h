/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
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
#include "stm32u5xx_hal.h"

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
#define DO_POWER_LED_Pin GPIO_PIN_13
#define DO_POWER_LED_GPIO_Port GPIOC
#define UART3_LORARX_Pin GPIO_PIN_5
#define UART3_LORARX_GPIO_Port GPIOA
#define UART3_LORATX_Pin GPIO_PIN_7
#define UART3_LORATX_GPIO_Port GPIOA
#define DO_EEPROM_ENABLE_Pin GPIO_PIN_12
#define DO_EEPROM_ENABLE_GPIO_Port GPIOB
#define DO_MEMS_ENABLE_Pin GPIO_PIN_13
#define DO_MEMS_ENABLE_GPIO_Port GPIOB
#define DO_LORA_ENABLE_Pin GPIO_PIN_14
#define DO_LORA_ENABLE_GPIO_Port GPIOB
#define DO_RS485_ENABLE_Pin GPIO_PIN_15
#define DO_RS485_ENABLE_GPIO_Port GPIOB
#define DI_MEMS_INT1_Pin GPIO_PIN_8
#define DI_MEMS_INT1_GPIO_Port GPIOA
#define UART1_RS485RX_Pin GPIO_PIN_9
#define UART1_RS485RX_GPIO_Port GPIOA
#define UART1_RS485TX_Pin GPIO_PIN_10
#define UART1_RS485TX_GPIO_Port GPIOA
#define DI_MEMS_INT2_Pin GPIO_PIN_11
#define DI_MEMS_INT2_GPIO_Port GPIOA
#define UART1_RS485DE_Pin GPIO_PIN_12
#define UART1_RS485DE_GPIO_Port GPIOA
#define SPI1_CS_Pin GPIO_PIN_15
#define SPI1_CS_GPIO_Port GPIOA
#define DO_EEPROM_WP_Pin GPIO_PIN_8
#define DO_EEPROM_WP_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
