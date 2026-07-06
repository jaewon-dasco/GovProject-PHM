/*
 * Mi_SoftwareRevision.h
 *
 *  Created on: 2026-06-23
 *      Author: JONE
 */

#ifndef INC_MI_SOFTWAREREVISION_H_
#define INC_MI_SOFTWAREREVISION_H_

#include "Mi_Main.h"

#define MI_SW_REVISION				0.12

/* History

2026-06-23 | HW 0.1 | FW 0.1
	- 초기 PHM 프로젝트 스켈레톤 (STM32U575CGT6, ISM330DHCX MEMS, 24LC16B EEPROM)
	- OneLibrary STM32U5 호환 (DMA Ring Tx, USE_HAL_UART_REGISTER_CALLBACKS)
	- EEPROM_24LC 드라이버 oIO_t 패턴 리팩토링 (WP 핀 NULL 허용)
2026-06-25 | HW 0.1 | FW 0.1 (continued)
	- ONE_Serial DMA race 수정 + 송신 효율 개선:
	  · oSerial_vPrint/PrintLine/Log: char-by-char → vsnprintf 한 번에 oSerial_Write (256B 로컬 버퍼)
	  · oSerial_Write txInProgress 판정: HW state만 → IsTxBusy(SW) || hwBusy 결합
	    (DMA 종료~콜백 race window 에서 청크 재전송되던 버그 해결)
2026-07-02 | HW 0.1 | FW 0.11
	- MiSerial_Handler·MiSerial_RxBuffer 정의를 공통 Mi_Serial.c → 모델별 Mi_Serial_PHM.c로 이동
	  · 모델별 TX/RX 버퍼 구성 분리 (SIV100 패턴 통일)
	  · PHM은 MiSerial_TxBuffer 활성화 (.pTxBuffer=MiSerial_TxBuffer, DMA 링버퍼 송신)
2026-07-03 | HW 0.1 | FW 0.12
	- MiLoRa(pUART) NULL 가드 추가: pUART==NULL 시 즉시 return (SIA100_VB 동기화)
*/

#endif /* INC_MI_SOFTWAREREVISION_H_ */
