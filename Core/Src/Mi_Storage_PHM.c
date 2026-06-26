/*
 * Mi_Storage_PHM.c — PHM 24LC16B EEPROM 기반 파라미터 저장
 *
 *  레이아웃 (2048 byte EEPROM):
 *    addr 0x000 ~ 0x00F : MiStorage_PageHeader_t (CRC32, Sign, UnixTime, DLC)
 *    addr 0x010 ~ 0x7FF : IoTParameter_t (max 2032 byte)
 *
 *  Read:  Header CRC 검증 → 실패 시 DefaultParameter
 *  Write: CRC 계산 후 Header + Data 일괄 기록
 *
 *  EEPROM driver 는 동기 blocking (페이지당 ~5ms write delay 포함).
 *  IoT 데이터 히스토리 (NAND) 미지원 — Read/WriteIoTData 는 ERROR.
 */
#define EEPROM_24LC16B

#include <string.h>
#include "ONE_Common.h"
#include "ONE_Memory.h"
#include "ONE_Serial.h"
#include "ONE_Time.h"
#include "Mi_IoT.h"
#include "Mi_Serial.h"
#include "Mi_Storage.h"
#include "EEPROM_24LC.h"

/* ─── Public state ─── */
uint8_t MiStorage_IsOpen = 0;
uint8_t MiStorage_IsBusy = 0;
int8_t MiStorage_ParameterSaveCmd = 0;
MiStorage_NandHeader_t MiStorage_NandHeader;

/* ─── Device ─── */
static EEPROM_24LC_t MiEEPROM;

/* IoTParameter 가 EEPROM 용량 내 들어가는지 컴파일타임 검증 */
_Static_assert(sizeof(IoTParameter_t) <= (EEPROM_24LC_MAX_BYTESIZE - MISTORAGE_EEPROM_DATA_ADDR),
               "IoTParameter_t too large for 24LC16B EEPROM");

/* ============================================================
 * Open — EEPROM 전원 ON + driver init
 * ============================================================ */
oResult_t MiStorage_Open(void)
{
	if(MiStorage_IsOpen){
		return RESULT_OK;
	}

	GPIOs.DO.EEPROMEnable = 1;
	HAL_Delay(5);	/* VDD 안정화 (24LC16B Tpur < 1ms) */

	if(EEPROM_24LC_Init(&MiEEPROM, &hi2c1, NULL, 0) != RESULT_OK){
		oSerial_Log("MiStorage", "EEPROM init FAIL");
		MiStorage_NandHeader.Status.IsFault = 1;
		return RESULT_ERROR;
	}

	MiStorage_IsOpen = 1;
	oSerial_Log("MiStorage", "EEPROM open OK");
	return RESULT_OK;
}

/* ============================================================
 * Close — driver deinit + 전원 OFF
 * ============================================================ */
oResult_t MiStorage_Close(void)
{
	if(!MiStorage_IsOpen){
		return RESULT_OK;
	}

	EEPROM_24LC_DeInit(&MiEEPROM);
	GPIOs.DO.EEPROMEnable = 0;
	MiStorage_IsOpen = 0;
	return RESULT_OK;
}

/* ============================================================
 * ReadIoTParameter — Header CRC 검증 후 데이터 로드
 * ============================================================ */
oResult_t MiStorage_ReadIoTParameter(IoTParameter_t *pParameter)
{
	MiStorage_PageHeader_t Header;
	uint32_t CalcCRC;
	uint32_t Size;

	if(pParameter == NULL){
		return RESULT_NULL;
	}

	if(!MiStorage_IsOpen){
		if(MiStorage_Open() != RESULT_OK){
			return RESULT_ERROR;
		}
	}

	/* 헤더 읽기 */
	if(EEPROM_24LC_Read(&MiEEPROM, MISTORAGE_EEPROM_HDR_ADDR, (uint8_t *)&Header, sizeof(Header)) != RESULT_OK){
		oSerial_Log("MiStorage", "ReadParam header I2C FAIL");
		MiStorage_NandHeader.Status.IsError = 1;
		return RESULT_ERROR;
	}

	if(Header.Sign != MISTORAGE_EEPROM_SIGN){
		oSerial_Log("MiStorage", "ReadParam sign=0x%02X (uninit)", Header.Sign);
		return RESULT_ERROR;
	}

	Size = Header.DLC;
	if(Size == 0 || Size > sizeof(IoTParameter_t)){
		oSerial_Log("MiStorage", "ReadParam DLC=%u invalid", (unsigned int)Size);
		return RESULT_ERROR;
	}

	/* 데이터 읽기 */
	memset(pParameter, 0, sizeof(IoTParameter_t));
	if(EEPROM_24LC_Read(&MiEEPROM, MISTORAGE_EEPROM_DATA_ADDR, (uint8_t *)pParameter, Size) != RESULT_OK){
		oSerial_Log("MiStorage", "ReadParam data I2C FAIL");
		MiStorage_NandHeader.Status.IsError = 1;
		return RESULT_ERROR;
	}

	/* CRC 검증 */
	CalcCRC = oMEM_CRC32((uint8_t *)pParameter, Size, 0);
	if(CalcCRC != Header.CRCValue){
		oSerial_Log("MiStorage", "ReadParam CRC mismatch (calc=0x%X hdr=0x%X)",
		            (unsigned int)CalcCRC, (unsigned int)Header.CRCValue);
		return RESULT_ERROR;
	}

	MiStorage_NandHeader.Status.IsError = 0;
	oSerial_Log("MiStorage", "ReadParam OK (%u byte)", (unsigned int)Size);
	return RESULT_OK;
}

/* ============================================================
 * WriteIoTParameter — Header + Data 일괄 기록
 * ============================================================ */
oResult_t MiStorage_WriteIoTParameter(IoTParameter_t *pParameter)
{
	MiStorage_PageHeader_t Header;
	uint32_t Size;

	if(pParameter == NULL){
		return RESULT_NULL;
	}

	if(!MiStorage_IsOpen){
		if(MiStorage_Open() != RESULT_OK){
			return RESULT_ERROR;
		}
	}

	Size = sizeof(IoTParameter_t);

	/* 데이터 기록 (페이지 단위 driver 내부 처리) */
	if(EEPROM_24LC_Write(&MiEEPROM, MISTORAGE_EEPROM_DATA_ADDR, (uint8_t *)pParameter, Size) != RESULT_OK){
		oSerial_Log("MiStorage", "WriteParam data FAIL");
		MiStorage_NandHeader.Status.IsError = 1;
		return RESULT_ERROR;
	}

	/* 헤더 작성 */
	memset(&Header, 0, sizeof(Header));
	Header.Sign = MISTORAGE_EEPROM_SIGN;
	Header.UnixTime = oDT_ToUnixTime(oDT_UpdateNow());
	Header.DLC = Size;
	Header.CRCValue = oMEM_CRC32((uint8_t *)pParameter, Size, 0);

	if(EEPROM_24LC_Write(&MiEEPROM, MISTORAGE_EEPROM_HDR_ADDR, (uint8_t *)&Header, sizeof(Header)) != RESULT_OK){
		oSerial_Log("MiStorage", "WriteParam header FAIL");
		MiStorage_NandHeader.Status.IsError = 1;
		return RESULT_ERROR;
	}

	MiStorage_NandHeader.Status.IsError = 0;
	oSerial_Log("MiStorage", "WriteParam OK (%u byte, CRC=0x%X)",
	            (unsigned int)Size, (unsigned int)Header.CRCValue);
	return RESULT_OK;
}

/* ============================================================
 * IoTData Read/Write — 미지원 (NAND 없음)
 * ============================================================ */
oResult_t MiStorage_ReadIoTData(uint32_t Address, IoT_DataPacket_t* pPayload, MiStorage_PageHeader_t *pHeader)
{
	(void)Address; (void)pPayload; (void)pHeader;
	return RESULT_ERROR;
}

oResult_t MiStorage_WriteIoTData(IoT_DataPacket_t* pPayload)
{
	(void)pPayload;
	return RESULT_ERROR;
}

/* ============================================================
 * MiStorage — main loop
 *   step0: 부팅 시 Parameter 로드 (실패 → DefaultParameter)
 *   step1: 첫 저장 (DefaultParameter 적용된 경우)
 *   step2: 저장 명령 (MiSerial 의 "Set/Save") 대기
 * ============================================================ */
void MiStorage(void)
{
	static uint8_t Step = 0;

	MiStorage_IsBusy = MiStorage_IsOpen || GPIOs.DO.EEPROMEnable;

	switch(Step)
	{
		default:
			Step = 0;
			break;

		case 0:	/* 부팅 시 1회 로드 */
			if(MiStorage_ReadIoTParameter(&MiIoT_Parameter) == RESULT_OK){
				if(MiIoT_DefaultParameter.Information.ProductCode != MiIoT_Parameter.Information.ProductCode){
					oSerial_Log("MiStorage", "Boot: product mismatch → DefaultParam");
					MiIoT_Parameter = MiIoT_DefaultParameter;
					Step = 1;	/* 새 default 저장 */
				}
				else{
					MiStorage_NandHeader.Status.IsInitialized = 1;
					MiStorage_NandHeader.Status.IsOkay = 1;
					Step = 2;
				}
			}
			else{
				oSerial_Log("MiStorage", "Boot: EEPROM read FAIL → DefaultParam");
				MiIoT_Parameter = MiIoT_DefaultParameter;
				Step = 1;
			}
			break;

		case 1:	/* DefaultParameter 신규 저장 */
			if(MiStorage_WriteIoTParameter(&MiIoT_Parameter) == RESULT_OK){
				MiStorage_NandHeader.Status.IsInitialized = 1;
				MiStorage_NandHeader.Status.IsOkay = 1;
				Step = 2;
			}
			else{
				MiStorage_NandHeader.Status.CountOfError++;
				if(MiStorage_NandHeader.Status.CountOfError > 3){
					MiStorage_NandHeader.Status.IsFault = 1;
					Step = 2;	/* 더 시도하지 않음 */
				}
			}
			break;

		case 2:	/* Save 명령 대기 */
			if(MiStorage_ParameterSaveCmd == 1){
				if(MiStorage_WriteIoTParameter(&MiIoT_Parameter) == RESULT_OK){
					MiStorage_ParameterSaveCmd = 2;
				}
				else{
					MiStorage_ParameterSaveCmd = -1;
				}
			}
			break;
	}
}

/* History

2026-06-26 | v0.1
	- baseline (Mi_Storage_PHM.c)
*/
