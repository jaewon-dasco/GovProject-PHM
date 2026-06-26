/*
 * Mi_Measurement.h
 *
 *  PHM 측정 모듈 스켈레톤 (ISM330DHCX MEMS · 24LC16B EEPROM).
 *  실제 구현은 추후 Mi_Measurement.c 작성 시 추가.
 *
 *  Created on: 2026-06-23
 *      Author: JONE
 */

#ifndef INC_MI_MEASUREMENT_H_
#define INC_MI_MEASUREMENT_H_

#define MI_MEASUREMENT_VERSION		0.1

#include "Mi_Main.h"
#include "Mi_IoT.h"

#define MEASUREMENT_CHANNEL_MAXCOUNT		1

extern void      Measurement_Process(void);
extern oResult_t Measurement_Init(void);
extern oResult_t Measurement_Sensor(IoT_DataPacket_t *pPacket);
extern oResult_t Measurement_Supply(uint8_t Count);
extern void      Measurement_OnFifoIrq(void);	/* EXTI8 ISR 호출 */

#endif /* INC_MI_MEASUREMENT_H_ */

/* History

2026-06-26 | v0.1
	- baseline (Mi_Measurement.h)
*/
