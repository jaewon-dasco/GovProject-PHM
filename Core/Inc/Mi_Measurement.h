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

#define MI_MEASUREMENT_VERSION		0.4

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

2026-07-03 | v0.2
	- 자체 FFT 구현을 OneLibrary ONE_FFT 로 추출

2026-07-03 | v0.3
	- 가속도 게인 보정 ACCEL_GAIN_CORRECTION(1/1.7) 추가 — 추후 제거 가능

2026-07-03 | v0.4
	- MEMS 로그 포맷 변경 — X/Y/Z[min,max], RMS[x,y,z], PPV, Hz 순
*/
