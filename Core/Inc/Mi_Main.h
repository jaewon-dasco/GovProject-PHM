#ifndef INC_MI_MAIN_H_
#define INC_MI_MAIN_H_

#define MI_MAIN_VERSION		0.1

#include "ONE_Math.h"
#include "ONE_Signal.h"
#include "ONE_Time.h"
#include "ONE_Serial.h"
#include "main.h"

//#define DEBUGMODE
#define SLEEPMODE_ENABLE	1
#define DAS_SERIALNUMBER(year,week,number)	((MATH_MAX((uint32_t)(year),2022)-2000) * 1000000) + ((MATH_MIN((uint32_t)(week),52)) * 10000) + (uint32_t)(MATH_MIN(number,9999))

#pragma pack(1)
#pragma pack()


/* PHM HW0.1 GPIO 매핑 (.ioc 기준)
 *   PA8  DI_MEMS_INT1     | PA11 DI_MEMS_INT2  | PA15 SPI1_CS
 *   PB12 DO_EEPROM_ENABLE | PB13 DO_MEMS_ENABLE
 *   PB14 DO_LORA_ENABLE   | PB15 DO_RS485_ENABLE
 */
typedef struct{
	struct{
		uint8_t LoRaEnable;
		uint8_t MEMSEnable;
		uint8_t EEPROMEnable;
		uint8_t RS485Enable;
	}DO;

	struct{
		uint8_t UsbConnected;	/* PHM HW0.1에는 USB 없음 — 항상 0, 99_Common Mi_IoT.c 호환용 */
		uint8_t MemsEvent1;
		uint8_t MemsEvent2;
	}DI;

	/* TODO: PHM HW에 ADC가 없음 — 전원 모니터링 필요 시 .ioc 에 ADC 추가 후 활성화 */
	union{
		struct{
			uint16_t SystemSupply;
			uint16_t InternalBAT;
		};

		uint16_t Buffer[2];
	}ADC;
}GPIOs_t;

extern GPIOs_t GPIOs;

#if defined(IWDG_PRESCALER_4)
extern IWDG_HandleTypeDef hiwdg;
#endif

extern I2C_HandleTypeDef hi2c1;
extern SPI_HandleTypeDef hspi1;

extern UART_HandleTypeDef huart1;
extern UART_HandleTypeDef huart3;

extern DMA_HandleTypeDef handle_GPDMA1_Channel0;

extern void MiMain(void);

#endif /* INC_MI_MAIN_H_ */

/* History

2026-06-26 | v0.1
	- baseline (Mi_Main.h)
*/
