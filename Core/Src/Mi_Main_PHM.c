/*
 * Mi_Main_PHM.c
 *
 *  Version: 0.1 (2026-06-29)
 */
#include "ONE_Signal.h"
#include "ONE_Time.h"
#include "ONE_Memory.h"
#include "Mi_Native.h"
#include "Mi_LoRa.h"
#include "Mi_IoT.h"
#include "Mi_SoftwareRevision.h"
#include "Mi_Measurement.h"
#include "Mi_Serial.h"
#include "Mi_Storage.h"

#define SYSTEM_SUPPLY_LOW_LIMIT		3200
#define RTC_SUPPLY_LOW_LIMIT		2100

/* PHM HW0.1 핀 인스턴스 (.ioc 라벨 기준) */
oIO_t DO_EEPROM_ENABLE	= {DO_EEPROM_ENABLE_GPIO_Port,	DO_EEPROM_ENABLE_Pin,	IO_LOW};
oIO_t DO_MEMS_ENABLE	= {DO_MEMS_ENABLE_GPIO_Port,	DO_MEMS_ENABLE_Pin,		IO_LOW};
oIO_t DO_LORA_ENABLE	= {DO_LORA_ENABLE_GPIO_Port,	DO_LORA_ENABLE_Pin,		IO_LOW};
oIO_t DO_RS485_ENABLE	= {DO_RS485_ENABLE_GPIO_Port,	DO_RS485_ENABLE_Pin,	IO_LOW};
oIO_t DO_MEMS_CS 		= {SPI1_CS_GPIO_Port, 			SPI1_CS_Pin, 			IO_LOW };	/* active LOW */
oIO_t DO_EEPROM_WP 		= {DO_EEPROM_WP_GPIO_Port, 		DO_EEPROM_WP_Pin, 		IO_HIGH};
oIO_t DO_POWER_LED 		= {DO_POWER_LED_GPIO_Port, 		DO_POWER_LED_Pin, 		IO_LOW};
oIO_t DI_MEMS_INT1		= {DI_MEMS_INT1_GPIO_Port,		DI_MEMS_INT1_Pin,		IO_HIGH};
oIO_t DI_MEMS_INT2		= {DI_MEMS_INT2_GPIO_Port,		DI_MEMS_INT2_Pin,		IO_HIGH};

GPIOs_t GPIOs;
oDebounce_t DB_DataReady	= DEBOUNCE_INITIALIZER(100,100);
oDebounce_t DebounceError	= DEBOUNCE_INITIALIZER(1000,300);

oResult_t MiMain_GPIOControl()
{
	//GPIO output
	IO_WRITE(DO_LORA_ENABLE,	GPIOs.DO.LoRaEnable);
	IO_WRITE(DO_MEMS_ENABLE,	GPIOs.DO.MEMSEnable);
	IO_WRITE(DO_EEPROM_ENABLE,	GPIOs.DO.EEPROMEnable);
	//IO_WRITE(DO_EEPROM_WP,		GPIOs.DO.EEPROM_WP);
	IO_WRITE(DO_RS485_ENABLE,	GPIOs.DO.RS485Enable);
	IO_WRITE(DO_POWER_LED,		GPIOs.DO.PowerLED);

	//GPIO input
	GPIOs.DI.MemsEvent1 = IO_READ(DI_MEMS_INT1);
	GPIOs.DI.MemsEvent2 = IO_READ(DI_MEMS_INT2);

	uint32_t IORun = 0;
	IORun += GPIOs.DO.LoRaEnable && !MiLoRa_IsSleep;
	IORun += GPIOs.DO.MEMSEnable;
	IORun += GPIOs.DO.EEPROMEnable;
	IORun += GPIOs.DO.RS485Enable;
	IORun += GPIOs.DO.PowerLED;

	return IORun ? RESULT_RUN : RESULT_ERROR;
}

void MiMain_GPIODeInit(void)
{
	GPIO_InitTypeDef cfg = {.Mode = GPIO_MODE_ANALOG, .Pull = GPIO_NOPULL};

	cfg.Pin = DO_MEMS_ENABLE.Pin;
	HAL_GPIO_Init(DO_MEMS_ENABLE.Port, &cfg);
	cfg.Pin = DO_EEPROM_ENABLE.Pin;
	HAL_GPIO_Init(DO_EEPROM_ENABLE.Port, &cfg);
	cfg.Pin = DO_EEPROM_WP.Pin;
	HAL_GPIO_Init(DO_EEPROM_WP.Port, &cfg);
	cfg.Pin = DO_RS485_ENABLE.Pin;
	HAL_GPIO_Init(DO_RS485_ENABLE.Port, &cfg);
	cfg.Pin = DO_POWER_LED.Pin;
	HAL_GPIO_Init(DO_POWER_LED.Port, &cfg);
	cfg.Pin = DO_MEMS_CS.Pin;
	HAL_GPIO_Init(DO_MEMS_CS.Port, &cfg);

	if(!MiLoRa_IsSleep){
		cfg.Pin = DO_LORA_ENABLE.Pin;
		HAL_GPIO_Init(DO_LORA_ENABLE.Port, &cfg);
	}
}

void MiMain_GPIOInit(void)
{
	GPIO_InitTypeDef cfg = {.Speed = GPIO_SPEED_FREQ_LOW};

	__HAL_RCC_GPIOA_CLK_ENABLE();
	__HAL_RCC_GPIOB_CLK_ENABLE();
	__HAL_RCC_GPIOH_CLK_ENABLE();

	/* 1. OD outputs 모드 (.ioc 와 일치: PB12-15 = OD, NOPULL, init HIGH) */
	cfg.Mode = GPIO_MODE_OUTPUT_OD;
	cfg.Pull = GPIO_NOPULL;

	cfg.Pin = DO_LORA_ENABLE.Pin;
	HAL_GPIO_Init(DO_LORA_ENABLE.Port, &cfg);
	cfg.Pin = DO_MEMS_ENABLE.Pin;
	HAL_GPIO_Init(DO_MEMS_ENABLE.Port, &cfg);
	cfg.Pin = DO_EEPROM_ENABLE.Pin;
	HAL_GPIO_Init(DO_EEPROM_ENABLE.Port, &cfg);
	cfg.Pin = DO_RS485_ENABLE.Pin;
	HAL_GPIO_Init(DO_RS485_ENABLE.Port, &cfg);

	/* 2. PP outputs — Power LED, EEPROM WP, SPI1 CS (.ioc 와 일치) */
	cfg.Mode = GPIO_MODE_OUTPUT_PP;
	cfg.Pull = GPIO_NOPULL;

	cfg.Pin = DO_POWER_LED.Pin;
	HAL_GPIO_Init(DO_POWER_LED.Port, &cfg);
	cfg.Pin = DO_EEPROM_WP.Pin;
	HAL_GPIO_Init(DO_EEPROM_WP.Port, &cfg);
	cfg.Pin = DO_MEMS_CS.Pin;
	HAL_GPIO_Init(DO_MEMS_CS.Port, &cfg);

	/* 3. EXTI Rising inputs — MEMS INT1(PA8), INT2(PA11) */
	cfg.Mode = GPIO_MODE_IT_RISING;
	cfg.Pull = GPIO_NOPULL;

	cfg.Pin  = DI_MEMS_INT1.Pin;
	HAL_GPIO_Init(DI_MEMS_INT1.Port, &cfg);
	cfg.Pin  = DI_MEMS_INT2.Pin;
	HAL_GPIO_Init(DI_MEMS_INT2.Port, &cfg);

	HAL_NVIC_SetPriority(EXTI8_IRQn, 0, 0);
	HAL_NVIC_EnableIRQ(EXTI8_IRQn);
	HAL_NVIC_SetPriority(EXTI11_IRQn, 0, 0);
	HAL_NVIC_EnableIRQ(EXTI11_IRQn);

	/* 4. 모드 설정 후 마지막에 IO 값 적용 */
	MiMain_GPIOControl();
}

oResult_t MiMain_UpdateMeasure(IoT_DataPacket_t **ppPacket)
{
	static uint8_t UpdateSamplingStep = 0;
	static IoT_DataPacket_t DataPacket;
	oResult_t result = RESULT_RUN;

	switch(UpdateSamplingStep)
	{
		case 0:
			oSerial_Log("UpdateMeasure", "start\r\n");
			UpdateSamplingStep++;
			break;
		case 1:
			if(MiIoT_Parameter.ChannelConfig[0].TypeOfSensor != IoTSensorType_Tilt){
				oSerial_Log("UpdateMeasure", "Channel type error\r\n");
				result = RESULT_ERROR;
			}
			else{
				switch(result = Measurement_Sensor(&DataPacket))
				{
					case RESULT_RUN:
						break;
					case RESULT_OK:
						*ppPacket = &DataPacket;
						UpdateSamplingStep++;
						break;
					default:
						oSerial_Log("UpdateMeasure", "sampling error\r\n");
						result = RESULT_ERROR;
						break;
				}
			}
			break;
		case 2:
			result = RESULT_DONE;
			break;
	}

	if(result != RESULT_RUN && result != RESULT_OK){
		UpdateSamplingStep = 0;
		oSerial_Log("UpdateMeasure", "finish\r\n");
	}

	return result;
}

oResult_t MiMain_UpdateStatus()
{
	oResult_t result = RESULT_OK;

	MiIoT_Status.SoftwareVersion = (uint16_t)(MI_SW_REVISION*100);

	/* TODO: PHM HW에 ADC가 없음 — 추가되면 Measurement_Supply() 활성화 */
	MiIoT_Status.UpdateTimestmap = oTMR_GetTick(TICKBASE_SYSTICK);
	MiIoT_Status.StatusBits.Okay = (MiIoT_Status.StatusBits.Bits & 0xFFFFFFFE) == 0 ? 1 : 0;

	return result;
}

void MiMain (void)
{
	static uint8_t MiMainStep = 0;

	Native_WatchDog(MIIOT_SLEEP_TIME*2);
	MiSerial(&huart1);   /* RS485 log */

	switch(MiMainStep)
	{
		case 0: //Init
			MiIoT_SamplingCallback   = MiMain_UpdateMeasure;
			MiIoT_StatusCallback     = MiMain_UpdateStatus;
			MiIoT_IOControlCallback  = MiMain_GPIOControl;

			MiIoT_GPIOInitCallback   = MiMain_GPIOInit;
			MiIoT_GPIODeInitCallback = MiMain_GPIODeInit;

			MiMainStep++;
			break;
		default:
			Measurement_Process();
			MiStorage();
			MiIoT(&huart3);
			break;
	}
}
