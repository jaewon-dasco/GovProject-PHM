/*
 * EEPROM_24LC.h
 *
 *  Microchip 24LCxx I2C Serial EEPROM 드라이버.
 *  지원: 24LC16B (16 Kbit, 16B/page), 24LC32A (32 Kbit, 32B/page)
 *  WP 핀은 옵션 (NULL 허용) — HW에 WP가 GND/VCC 직결 시 NULL 전달.
 *
 *  Created on: Dec 24, 2024
 *      Author: JONE
 *  Reference: https://github.com/nimaltd/ee24/blob/master/ee24.c
 */

#ifndef INC_EEPROM_24LC_H_
#define INC_EEPROM_24LC_H_

#define EEPROM_24LC_VERSION		0.1

#include "ONE_Common.h"
#include "ONE_Signal.h"
#include "main.h"

/* 칩 선택 — 사용 측에서 #define EEPROM_24LC16B 또는 EEPROM_24LC32A 정의 */
#if !defined(EEPROM_24LC16B) && !defined(EEPROM_24LC32A)
#define EEPROM_24LC32A  /* 기본값 */
#endif

#if defined(EEPROM_24LC32A)
#define EEPROM_24LC_PAGE_BYTESIZE	32
#define EEPROM_24LC_MAX_BYTESIZE	(EEPROM_24LC_PAGE_BYTESIZE*4095)
#define EEPROM_24LC_MEMADD_SIZE		I2C_MEMADD_SIZE_16BIT
#elif defined(EEPROM_24LC16B)
#define EEPROM_24LC_PAGE_BYTESIZE	16
#define EEPROM_24LC_MAX_BYTESIZE	(EEPROM_24LC_PAGE_BYTESIZE*255)
#define EEPROM_24LC_MEMADD_SIZE		I2C_MEMADD_SIZE_8BIT
#endif

/* ─── Device Structure ─── */
typedef struct {
	I2C_HandleTypeDef *pI2C;
	oIO_t *pWP;				/* WP 핀 — NULL 허용 (옵션) */
	uint8_t ChipAddress;	/* 24LC16B: 블록 선택 0~7 (B2/B1/B0), 24LC32A: A2/A1/A0 핀 값 0~7 */
	uint8_t IsOpen;
	uint8_t IsFault;
} EEPROM_24LC_t;

/* ─── API ─── */
extern oResult_t EEPROM_24LC_Init(EEPROM_24LC_t *pDev, I2C_HandleTypeDef *pI2C, oIO_t *pWP, uint8_t ChipAddress);
extern oResult_t EEPROM_24LC_DeInit(EEPROM_24LC_t *pDev);
extern oResult_t EEPROM_24LC_Read(EEPROM_24LC_t *pDev, uint16_t Index, uint8_t *pData, uint32_t Size);
extern oResult_t EEPROM_24LC_Write(EEPROM_24LC_t *pDev, uint16_t Index, uint8_t *pData, uint32_t Size);

#endif /* INC_EEPROM_24LC_H_ */

/* History

2026-06-26 | v0.1
	- baseline (EEPROM_24LC.h)
*/
