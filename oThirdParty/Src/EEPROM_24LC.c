/*
 * EEPROM_24LC.c
 *
 *  Microchip 24LCxx I2C Serial EEPROM 드라이버.
 *
 *  Created on: Dec 24, 2024
 *      Author: JONE
 *  Reference: https://github.com/nimaltd/ee24/blob/master/ee24.c
 */

#include "ONE_Common.h"
#include "ONE_Time.h"
#include "EEPROM_24LC.h"

/* I2C 슬레이브 주소 — 상위 4비트 1010 고정 + 칩/블록 선택 3비트 + R/W 1비트 */
#define	EEPROM_24LC_CHIP_READ(chip)		(((chip) << 1 & 0x0E) | 0xA1)
#define	EEPROM_24LC_CHIP_WRITE(chip)	(((chip) << 1 & 0x0E) | 0xA0)

#define EEPROM_24LC_I2C_TIMEOUT			1000	/* ms — bounded timeout */

/* WP 핀 제어 — NULL 안전.
 * State=1: 핀을 ActiveLevel 로 driving (write 허용/금지 — ActiveLevel 의미는 사용 측 정의)
 * State=0: 핀을 !ActiveLevel 로 driving (반대 상태) */
static void EEPROM_24LC_WP_Set(EEPROM_24LC_t *pDev, uint8_t State)
{
	if(pDev != NULL && pDev->pWP != NULL){
		HAL_GPIO_WritePin(pDev->pWP->Port, pDev->pWP->Pin,
		                  State ? pDev->pWP->ActiveLevel : !pDev->pWP->ActiveLevel);
	}
}

oResult_t EEPROM_24LC_Init(EEPROM_24LC_t *pDev, I2C_HandleTypeDef *pI2C, oIO_t *pWP, uint8_t ChipAddress)
{
	if(pDev == NULL || pI2C == NULL){
		return RESULT_ERROR;
	}

	pDev->pI2C = pI2C;
	pDev->pWP = pWP;					/* NULL 허용 */
	pDev->ChipAddress = ChipAddress & 0x07;
	pDev->IsFault = 0;
	pDev->IsOpen = 1;

	EEPROM_24LC_WP_Set(pDev, 0);		/* 초기: WP idle */

	return RESULT_OK;
}

oResult_t EEPROM_24LC_DeInit(EEPROM_24LC_t *pDev)
{
	if(pDev == NULL){
		return RESULT_ERROR;
	}

	EEPROM_24LC_WP_Set(pDev, 0);
	pDev->IsOpen = 0;
	pDev->IsFault = 0;

	return RESULT_OK;
}

oResult_t EEPROM_24LC_Read(EEPROM_24LC_t *pDev, uint16_t Index, uint8_t *pData, uint32_t Size)
{
	static uint8_t MemReadSqStep = 0;
	static uint32_t MemReadSqTimer = 0;
	uint8_t DevAddress;
	oResult_t result = RESULT_RUN;

	if(pDev == NULL || pDev->pI2C == NULL || !pDev->IsOpen){
		return RESULT_ERROR;
	}

	DevAddress = EEPROM_24LC_CHIP_READ(pDev->ChipAddress);

	switch(MemReadSqStep)
	{
		default:
			MemReadSqStep = 0;
			/* fallthrough */
		case 0:
			if(Index+Size <= EEPROM_24LC_MAX_BYTESIZE && Size > 0 && pData != NULL){
				MemReadSqTimer = oTMR_GetTick(TICKBASE_SYSTICK);
				MemReadSqStep++;
			}
			else{
				result = RESULT_ERROR;
				break;
			}
			/* fallthrough */
		case 1:
			if(oTMR_Elapsed(&MemReadSqTimer, 10, TICKBASE_SYSTICK)){
				MemReadSqStep++;
			}
			break;
		case 2:
			/* 호출자 buffer 직접 사용 — malloc 제거 (heap fragmentation 회피) */
			switch(HAL_I2C_Mem_Read(pDev->pI2C, DevAddress, Index, EEPROM_24LC_MEMADD_SIZE, pData, Size, EEPROM_24LC_I2C_TIMEOUT))
			{
				case HAL_OK:
					result = RESULT_OK;
					break;
				default:
				case HAL_ERROR:
					result = RESULT_ERROR;
					break;
			}
			break;
	}

	if(result != RESULT_RUN){
		MemReadSqStep = 0;
	}

	return result;
}

oResult_t EEPROM_24LC_Write(EEPROM_24LC_t *pDev, uint16_t Index, uint8_t *pData, uint32_t Size)
{
	static uint8_t MemWriteSqStep = 0;
	static uint32_t MemWriteSqTimer = 0;
	static uint32_t WriteCount = 0;

	uint16_t MemAddress;
	uint32_t RemainingSize = 0;
	uint16_t WriteSize = 0;
	uint16_t PageRemaining = 0;
	uint8_t DevAddress;
	oResult_t result = RESULT_RUN;

	if(pDev == NULL || pDev->pI2C == NULL || !pDev->IsOpen){
		return RESULT_ERROR;
	}

	DevAddress = EEPROM_24LC_CHIP_WRITE(pDev->ChipAddress);

	switch(MemWriteSqStep)
	{
		default:
			MemWriteSqStep = 0;
			/* fallthrough */
		case 0:
			if(Index+Size <= EEPROM_24LC_MAX_BYTESIZE && Size > 0 && pData != NULL){
				WriteCount = 0;
				EEPROM_24LC_WP_Set(pDev, 1);		/* WP active — write 진입 */
				MemWriteSqTimer = oTMR_GetTick(TICKBASE_SYSTICK);
				MemWriteSqStep++;
			}
			else{
				result = RESULT_ERROR;
				break;
			}
			/* fallthrough */
		case 1:
			if(oTMR_Elapsed(&MemWriteSqTimer, 10, TICKBASE_SYSTICK)){
				MemWriteSqStep++;
			}
			break;
		case 2:
			MemAddress = Index + WriteCount;
			RemainingSize = Size - WriteCount;
			PageRemaining = EEPROM_24LC_PAGE_BYTESIZE - (MemAddress % EEPROM_24LC_PAGE_BYTESIZE);
			WriteSize = (uint16_t)(RemainingSize > PageRemaining ? PageRemaining : RemainingSize);

			switch(HAL_I2C_Mem_Write(pDev->pI2C, DevAddress, (uint16_t)(MemAddress), EEPROM_24LC_MEMADD_SIZE,
			                         (uint8_t *)(pData + WriteCount), WriteSize, EEPROM_24LC_I2C_TIMEOUT))
			{
				case HAL_OK:
					MemWriteSqStep++;
					WriteCount += WriteSize;
					MemWriteSqTimer = oTMR_GetTick(TICKBASE_SYSTICK);
					break;
				default:
				case HAL_ERROR:
					result = RESULT_ERROR;
					break;
			}
			break;
		case 3:
			if(oTMR_Elapsed(&MemWriteSqTimer, 10, TICKBASE_SYSTICK)){
				if(WriteCount < Size){
					MemWriteSqStep--;
				}
				else{
					result = RESULT_OK;
				}
			}
			break;
	}

	if(result != RESULT_RUN){
		EEPROM_24LC_WP_Set(pDev, 0);		/* WP idle 복귀 */
		MemWriteSqStep = 0;
	}

	return result;
}

/* History

2026-06-26 | v0.1
	- baseline (EEPROM_24LC.c)
*/
