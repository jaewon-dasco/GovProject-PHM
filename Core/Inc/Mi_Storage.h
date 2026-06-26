/*
 * Mi_Storage.h — PHM EEPROM 기반 파라미터 저장
 *
 *  PHM HW0.1 은 NAND flash 미탑재.
 *  대신 24LC16B I2C EEPROM (2 KB) 에 IoTParameter_t 단일 저장.
 *
 *  공통 코드 (Mi_IoT.c·Mi_LoRa.c) 가 참조하는 NAND 심볼은 더미 stub 유지.
 *
 *  Created on: 2026-06-23
 */

#ifndef INC_MI_STORAGE_H_
#define INC_MI_STORAGE_H_

#define MI_STORAGE_VERSION		0.1

#include "ONE_Time.h"
#include "Mi_Main.h"
#include "Mi_IoT.h"

/* NAND 미사용 — 주소·크기는 0 더미 (Get/StoredData 명령이 항상 InvalidAddress 반환) */
#define MISTORAGE_DATA_STARTADR			0
#define MISTORAGE_DATA_ENDADR			0

/* EEPROM 주소 레이아웃 (24LC16B 2048 byte) */
#define MISTORAGE_EEPROM_SIGN			0xA5
#define MISTORAGE_EEPROM_HDR_ADDR		0
#define MISTORAGE_EEPROM_DATA_ADDR		16		/* 헤더 16 byte 정렬 */

#pragma pack(1)
typedef struct{
	uint32_t 			CRCValue;
	uint8_t				Sign;
	uint32_t			UnixTime;
	uint32_t			DLC;
} __attribute__((aligned(8))) MiStorage_PageHeader_t;
#pragma pack()

typedef struct{
	struct{
		int32_t OlderAddress;
		oDateAndTime_t OlderDateTime;

		int32_t NewerAddress;
		oDateAndTime_t NewerDateTime;

		int32_t CountOfData;

		int32_t SyncAddress;
	} SensorData;

	struct {
		uint8_t CountOfError;
		uint8_t IsInitialized;
		uint8_t IsFault;
		uint8_t IsHardFault;
		uint8_t IsError;
		uint8_t IsOkay;
	}Status;
}MiStorage_NandHeader_t;

extern uint8_t MiStorage_IsOpen;
extern uint8_t MiStorage_IsBusy;
extern int8_t MiStorage_ParameterSaveCmd;
extern MiStorage_NandHeader_t MiStorage_NandHeader;

extern oResult_t MiStorage_ReadIoTParameter(IoTParameter_t* pParameter);
extern oResult_t MiStorage_WriteIoTParameter(IoTParameter_t* pParameter);
extern oResult_t MiStorage_ReadIoTData(uint32_t Address, IoT_DataPacket_t* pPayload, MiStorage_PageHeader_t *pHeader);
extern oResult_t MiStorage_WriteIoTData(IoT_DataPacket_t* pPayload);
extern oResult_t MiStorage_Open(void);
extern oResult_t MiStorage_Close(void);
extern void MiStorage(void);

#endif /* INC_MI_STORAGE_H_ */

/* History

2026-06-26 | v0.1
	- baseline (Mi_Storage.h)
*/
