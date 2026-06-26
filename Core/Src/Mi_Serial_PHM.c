/*
 * Mi_Serial_PHM.c — PHM 제품 전용 오버라이드
 *
 *  공통 Mi_Serial.c 가 제공하지 않는 product-specific 함수 + RS485 log 라우팅.
 *
 *  HW:
 *    USART1 (PA9 TX / PA10 RX / PA12 DE) — RS485 외부 디버그 로그
 *    PB15 (DO_RS485_ENABLE) — 트랜시버 전원 enable
 *
 *  PHM 특성:
 *    - USB 없음 — 공통 MiSerial() 의 GPIOs.DI.UsbConnected gating 우회
 *    - 인터랙티브 명령 X — log 전용
 */
#include <string.h>
#include "Mi_Serial.h"

extern oResult_t MiSerial_DebugMessage(char *Message);

/* ============================================================
 * 공통 Mi_Serial.c 의 MiSerial() 을 __weak 로 정의해서 PHM 가 override.
 *
 * PHM 동작:
 *   첫 호출 시 핸들러 init + RS485 enable + log 라우팅 ON
 *   이후 호출은 idle (인터랙티브 명령 없음).
 * ============================================================ */
void MiSerial(UART_HandleTypeDef *pUART)
{
	char Message[MISERIAL_RX_BUFFER_SIZE];

	if(MiSerial_Handler.pUART == NULL){
		if(pUART != NULL){
			MiSerial_Handler.pUART = pUART;

			/* RS485 트랜시버 전원 ON */
			GPIOs.DO.RS485Enable = 1;
			GPIOs.DI.UsbConnected = 1;

			oSerial_LogEnagle(&MiSerial_Handler, 1);
		}
	}
	else if(MiSerial_Handler.pUART->gState == HAL_UART_STATE_RESET){
		HAL_UART_Init(MiSerial_Handler.pUART);
	}

	if(oSerial_ReadSplit(&MiSerial_Handler, Message, MISERIAL_RX_BUFFER_SIZE, ";\r\n") != RESULT_OK){
	//if(oSerial_Read(&MiSerial_Handler, Message, MISERIAL_RX_BUFFER_SIZE, 100) != RESULT_OK){
		return;
	}

	if(strlen(Message) <= 0){
		return;
	}

	if(MiSerial_DebugMessage(Message) == RESULT_OK){
		return;
	}
}

/* ============================================================
 * Product-specific 함수 (공통 Mi_Serial.c 가 extern 선언만, 본문은 product 측에서 정의)
 *
 * PHM 은 IMU 단일 센서·인터랙티브 명령 미사용 → 모두 no-op
 * ============================================================ */

void MiSerial_Process(void)
{
}

oResult_t MiSerial_Specific(char *Message)
{
	(void)Message;
	return RESULT_NULL;
}

void MiSerial_PrintChannelConfig(int32_t Channel, IoTParameter_t *pParameter)
{
	(void)Channel; (void)pParameter;
}

void MiSerial_PrintSensorData(IoT_DataPacket_t *pPacket)
{
	(void)pPacket;
}

oResult_t MiSerial_SetChannelConfig(char *Message, IoTParameter_t *pParameter, uint8_t Channel)
{
	(void)Message; (void)pParameter; (void)Channel;
	return RESULT_NULL;
}

/* Mi_Native.c HardFault 핸들러용 — 가변 인자 no-op */
void MiSerial_DebugLog(const char *fmt, ...)
{
	(void)fmt;
}

/* History

2026-06-26 | v0.1
	- baseline (Mi_Serial_PHM.c)
*/
