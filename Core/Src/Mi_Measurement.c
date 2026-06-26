/*
 * Mi_Measurement.c
 *
 *  PHM 측정 — ISM330DHCX 6축 IMU FIFO + EXTI batch 아키텍처.
 *
 *  데이터 흐름 (double-buffer 자동):
 *    [센서 내부 FIFO 9 KB ← 1.66 kHz 자동 누적]
 *           ↓ watermark 256 entry → INT1 (EXTI8) → flag set
 *    [MCU SPI burst read 256 sample] ──→ HPF·적분·peak·FFT ring push
 *                                        └─→ 1초 누적 시 FFT + snapshot
 *
 *  Filter:
 *    - Accel HPF biquad (float, fc=8Hz, Q=0.707) — 중력·DC 제거
 *    - Vel   HPF biquad (float, fc=2Hz, Q=0.707) — 적분 drift 제거
 *
 *  CMSIS-DSP 미사용 — 자체 radix-2 FFT (float, 1024-pt)
 *
 *  Created on: 2026-06-23
 *      Author: JONE
 */

#include <string.h>
#include <math.h>
#include "arm_math.h"
#include "ONE_Common.h"
#include "ONE_Time.h"
#include "ONE_Math.h"
#include "ONE_Filter.h"
#include "ONE_Serial.h"
#include "Mi_Main.h"
#include "Mi_IoT.h"
#include "Mi_Serial.h"
#include "Mi_Measurement.h"
#include "MEMS_ISM330DHCXTR.h"

/* ─── 측정 파라미터 ─── */
#define MEASUREMENT_ODR_HZ				1660
#define MEASUREMENT_WINDOW_SAMPLES		1660		/* 1 sec window */
#define MEASUREMENT_FIFO_WATERMARK		256			/* entry — ~154 ms @ 1.66 kHz */
#define MEASUREMENT_FIFO_BURST_BYTES	(MEASUREMENT_FIFO_WATERMARK * ISM330DHCX_FIFO_ENTRY_SIZE)
#define MEASUREMENT_FALLBACK_MS			200			/* IRQ 누락 시 강제 polling 주기 */
#define MEASUREMENT_PI					3.14159265358979f
#define MEASUREMENT_DT_SEC				(1.0f / (float)MEASUREMENT_ODR_HZ)

/* ±8 g: 0.244 mg/LSB → g per LSB */
#define ACCEL_LSB_TO_G					0.244e-3f		/* g 단위 (mg/LSB ÷ 1000) */
#define G_TO_MPSS						9.80665f		/* g → m/s² 환산 (적분 시 사용) */

/* HPF cutoff — ONE_Filter biquad Butterworth */
#define HPF_ACCEL_FC_HZ					5.0
#define HPF_VELOCITY_FC_HZ				2.0
#define HPF_BUTTERWORTH_Q				0.707

/* ─── FFT ─── */
#define FFT_SIZE						2048		/* zero-pad to next 2^N (1660 → 2048) */
#define FFT_SIZE_LOG2					11

/* ─── FFT dominant bin 검색 대역 (brick-wall band masking) ─── */
#define PEAK_SEARCH_LOW_HZ				5.0f		/* DC 인근 잔존 노이즈 배제 */
#define PEAK_SEARCH_HIGH_HZ				500.0f		/* Nyquist 근처 노이즈·aliasing 배제 (ISO 10816-3 호환) */

/* ─── 신호/노이즈 판별 임계값 (로그 통계 P95~P99 기반) ─── */
#define MEASUREMENT_PPV_THRESHOLD_MMPS	0.5f		/* 0.5 mm/s 미만 = 노이즈 floor (Zone A 진입 직전) */
#define MEASUREMENT_PEAK_THRESHOLD_G	0.030f		/* 30 mg 미만 = 노이즈 floor (P99 위) */

/* ─── State machine ─── */
typedef enum {
	MS_INIT = 0,
	MS_RUN,
} MeasurementState_t;

static MeasurementState_t MeasState = MS_INIT;

/* ─── Device ─── */
static ISM330DHCX_t MiMems;
extern oIO_t DO_MEMS_CS;

/* ─── HPF biquad (ONE_Filter, per axis) ─── */
static oFilter_t HpAccel[3] = {
	FILTER_PASS_INITIALIZER((double)MEASUREMENT_ODR_HZ, HPF_ACCEL_FC_HZ, HPF_BUTTERWORTH_Q),
	FILTER_PASS_INITIALIZER((double)MEASUREMENT_ODR_HZ, HPF_ACCEL_FC_HZ, HPF_BUTTERWORTH_Q),
	FILTER_PASS_INITIALIZER((double)MEASUREMENT_ODR_HZ, HPF_ACCEL_FC_HZ, HPF_BUTTERWORTH_Q),
};
static oFilter_t HpVel[3] = {
	FILTER_PASS_INITIALIZER((double)MEASUREMENT_ODR_HZ, HPF_VELOCITY_FC_HZ, HPF_BUTTERWORTH_Q),
	FILTER_PASS_INITIALIZER((double)MEASUREMENT_ODR_HZ, HPF_VELOCITY_FC_HZ, HPF_BUTTERWORTH_Q),
	FILTER_PASS_INITIALIZER((double)MEASUREMENT_ODR_HZ, HPF_VELOCITY_FC_HZ, HPF_BUTTERWORTH_Q),
};

/* ─── 적분기 ─── */
static float Velocity[3];	/* m/s */

/* ─── Peak 추적 (max |a| 의 순간 3축 동시 캡처) ─── */
static float PeakMag = 0;
static float PeakAccel[3] = {0, 0, 0};

/* ─── PPV 추적 (per axis velocity min/max) ─── */
static float VelMin[3] = { 1e30f, 1e30f, 1e30f };
static float VelMax[3] = { -1e30f, -1e30f, -1e30f };

/* ─── FFT ring buffer (3축 HPF 가속도, 윈도우 크기 = 1660 sample) ─── */
static float FftInputRing[3][MEASUREMENT_WINDOW_SAMPLES];
static uint32_t FftInputIdx = 0;
static uint8_t  FftInputReady = 0;

/* ─── FFT (CMSIS-DSP arm_rfft_fast_f32, 2048-pt with zero-padding) ─── */
static arm_rfft_fast_instance_f32 FftInst;
static float FftInput[FFT_SIZE];				/* 시간 도메인 실수 입력 (zero-pad 적용) */
static float FftOutput[FFT_SIZE];				/* 주파수 도메인 complex (real/imag interleaved) */
static float FftMagnitude[FFT_SIZE / 2];		/* magnitude */

/* ─── Window sample counter ─── */
static uint32_t SampleCount = 0;

/* ─── FIFO batch buffer ─── */
static uint8_t FifoBurstBuf[MEASUREMENT_FIFO_BURST_BYTES];

/* ─── ISR flag ─── */
volatile uint8_t MiMeasurement_FifoIrqFlag = 0;

/* ─── Snapshot ─── */
typedef struct {
	float PeakAccel[3];		/* m/s² (HPF 후) */
	float PPV[3];			/* mm/s peak-to-peak / 2 */
	float DominantHz[3];	/* 축별 dominant frequency (X, Y, Z) */
	float Temperature;
} MeasurementSnapshot_t;

static MeasurementSnapshot_t Snapshot;

/* ─── Fallback timer (IRQ 누락 대비) ─── */
static uint32_t FallbackTimer = 0;


/* ============================================================
 * FFT — CMSIS-DSP arm_rfft_fast_f32 (2048-pt, ~0.4 ms @ 160 MHz)
 *   axis: 0=X, 1=Y, 2=Z
 *   결과 magnitude 최대 bin 의 Hz 반환 (DC 제외)
 *   윈도우 1660 sample → 2048 zero-padding (분해능 0.81 Hz/bin)
 * ============================================================ */
static float Measurement_RunFFT(int32_t axis)
{
	uint32_t i, src;
	float maxVal;
	uint32_t maxIdx = 0;

	/* Ring buffer → FFT 입력 (oldest~newest 순) */
	src = FftInputIdx;
	for(i = 0; i < MEASUREMENT_WINDOW_SAMPLES; i++){
		FftInput[i] = FftInputRing[axis][src];
		src = (src + 1) % MEASUREMENT_WINDOW_SAMPLES;
	}
	/* Zero-padding (1660 ~ 2047) */
	for(i = MEASUREMENT_WINDOW_SAMPLES; i < FFT_SIZE; i++){
		FftInput[i] = 0;
	}

	/* Forward real FFT */
	arm_rfft_fast_f32(&FftInst, FftInput, FftOutput, 0);

	/* Magnitude (complex → real magnitude, N/2 bins) */
	arm_cmplx_mag_f32(FftOutput, FftMagnitude, FFT_SIZE / 2);

	/* Brick-wall band mask — [PEAK_SEARCH_LOW_HZ, PEAK_SEARCH_HIGH_HZ] 안에서만 검색 */
	uint32_t bin_lo = (uint32_t)(PEAK_SEARCH_LOW_HZ  * (float)FFT_SIZE / (float)MEASUREMENT_ODR_HZ);
	uint32_t bin_hi = (uint32_t)(PEAK_SEARCH_HIGH_HZ * (float)FFT_SIZE / (float)MEASUREMENT_ODR_HZ);
	if(bin_lo < 1)            bin_lo = 1;
	if(bin_hi > FFT_SIZE / 2) bin_hi = FFT_SIZE / 2;

	FftMagnitude[0] = 0;	/* DC bin 명시 제거 (안전망) */
	arm_max_f32(&FftMagnitude[bin_lo], bin_hi - bin_lo, &maxVal, &maxIdx);
	maxIdx += bin_lo;	/* 원래 bin 인덱스 복원 */

	return (float)maxIdx * (float)MEASUREMENT_ODR_HZ / (float)FFT_SIZE;
}

/* ============================================================
 * Init — Sensor open + FIFO Continuous + watermark IRQ + HPF + FFT twiddle
 * ============================================================ */
oResult_t Measurement_Init(void)
{
	oResult_t result;
	int32_t ax;

	GPIOs.DO.MEMSEnable = 1;

	result = ISM330DHCX_Open(&hspi1, DO_MEMS_CS, &MiMems);
	if(result != RESULT_OK){
		return result;
	}

	MiMems.Register.CTRL1_XL.ODR_XL = ISM330DHCX_XL_ODR_1_66KHZ;
	MiMems.Register.CTRL1_XL.FS_XL  = ISM330DHCX_XL_FS_8G;
	MiMems.Register.CTRL2_G.ODR_G   = ISM330DHCX_G_ODR_OFF;	/* gyro 미사용 — accel only */
	ISM330DHCX_SetConfig(&MiMems);

	/* FIFO Continuous + watermark + accel BDR 1.66 kHz */
	ISM330DHCX_FIFO_Setup(&MiMems, ISM330DHCX_FIFO_MODE_CONTINUOUS,
	                     MEASUREMENT_FIFO_WATERMARK,
	                     ISM330DHCX_BDR_1_66KHZ, ISM330DHCX_BDR_OFF);

	/* INT1 핀에 watermark 매핑 (PA8 EXTI8) */
	ISM330DHCX_FIFO_EnableINT1(&MiMems, 1, 0);

	/* HPF biquad Reset (oFilter_HPF 첫 호출 시 계수 자동 계산) */
	for(ax = 0; ax < 3; ax++){
		HpAccel[ax].Reset = 1;
		HpVel[ax].Reset = 1;
	}

	memset(Velocity, 0, sizeof(Velocity));
	memset(&Snapshot, 0, sizeof(Snapshot));

	PeakMag = 0;
	PeakAccel[0] = PeakAccel[1] = PeakAccel[2] = 0;
	VelMin[0] = VelMin[1] = VelMin[2] = 1e30f;
	VelMax[0] = VelMax[1] = VelMax[2] = -1e30f;

	memset(FftInputRing, 0, sizeof(FftInputRing));
	FftInputIdx = 0;
	FftInputReady = 0;
	SampleCount = 0;

	MiMeasurement_FifoIrqFlag = 0;
	FallbackTimer = oTMR_GetTick(TICKBASE_SYSTICK);

	/* CMSIS-DSP FFT instance init (1024-pt) */
	if(arm_rfft_fast_init_f32(&FftInst, FFT_SIZE) != ARM_MATH_SUCCESS){
		return RESULT_ERROR;
	}

	return RESULT_OK;
}

/* ============================================================
 * OnFifoIrq — EXTI8 ISR 에서 호출 (ISR context: flag set만)
 * ============================================================ */
void Measurement_OnFifoIrq(void)
{
	MiMeasurement_FifoIrqFlag = 1;
}

/* ============================================================
 * 1 sample 처리 — HPF·적분·peak·FFT ring push
 * ============================================================ */
static inline void Measurement_ProcessSample(int16_t rawX, int16_t rawY, int16_t rawZ)
{
	float a_g[3];		/* 가속도 (g 단위) */
	float ha_g[3];		/* HPF 후 가속도 (g 단위) — FFT·peak 용 */
	float a_mpss;		/* 임시 m/s² (적분용) */
	float hv[3];		/* HPF velocity (m/s) */
	float mag;
	int32_t ax;

	/* raw → g */
	a_g[0] = (float)rawX * ACCEL_LSB_TO_G;
	a_g[1] = (float)rawY * ACCEL_LSB_TO_G;
	a_g[2] = (float)rawZ * ACCEL_LSB_TO_G;

	/* HPF (g 단위 그대로) — gravity·DC 제거 */
	for(ax = 0; ax < 3; ax++){
		ha_g[ax] = (float)oFilter_HPF(&HpAccel[ax], (double)a_g[ax]);
	}

	/* 적분 → velocity (m/s)
	   적분만 SI 단위 필요 → g → m/s² 변환 */
	for(ax = 0; ax < 3; ax++){
		a_mpss = ha_g[ax] * G_TO_MPSS;
		Velocity[ax] += a_mpss * MEASUREMENT_DT_SEC;
	}

	/* velocity HPF (적분 drift 제거) + min/max */
	for(ax = 0; ax < 3; ax++){
		hv[ax] = (float)oFilter_HPF(&HpVel[ax], (double)Velocity[ax]);
		if(hv[ax] < VelMin[ax]) VelMin[ax] = hv[ax];
		if(hv[ax] > VelMax[ax]) VelMax[ax] = hv[ax];
	}

	/* Peak 추적: max |a| 의 순간 3축 동시 캡처 (g 단위) */
	mag = fabsf(ha_g[0]);
	if(fabsf(ha_g[1]) > mag) mag = fabsf(ha_g[1]);
	if(fabsf(ha_g[2]) > mag) mag = fabsf(ha_g[2]);

	if(mag > PeakMag){
		PeakMag = mag;
		PeakAccel[0] = ha_g[0];
		PeakAccel[1] = ha_g[1];
		PeakAccel[2] = ha_g[2];
	}

	/* FFT ring push (3축, g 단위) */
	FftInputRing[0][FftInputIdx] = ha_g[0];
	FftInputRing[1][FftInputIdx] = ha_g[1];
	FftInputRing[2][FftInputIdx] = ha_g[2];
	FftInputIdx = (FftInputIdx + 1) % MEASUREMENT_WINDOW_SAMPLES;
	if(FftInputIdx == 0){
		FftInputReady = 1;
	}

	SampleCount++;
}

/* ============================================================
 * Reset window
 * ============================================================ */
static void Measurement_ResetWindow(void)
{
	PeakMag = 0;
	PeakAccel[0] = PeakAccel[1] = PeakAccel[2] = 0;
	VelMin[0] = VelMin[1] = VelMin[2] = 1e30f;
	VelMax[0] = VelMax[1] = VelMax[2] = -1e30f;
	SampleCount = 0;
}

/* ============================================================
 * Snapshot 계산 — FFT + 출력
 * ============================================================ */
static void Measurement_ComputeSnapshot(void)
{
	Snapshot.PeakAccel[0] = PeakAccel[0];
	Snapshot.PeakAccel[1] = PeakAccel[1];
	Snapshot.PeakAccel[2] = PeakAccel[2];

	Snapshot.PPV[0] = (VelMax[0] - VelMin[0]) * 0.5f * 1000.0f;
	Snapshot.PPV[1] = (VelMax[1] - VelMin[1]) * 0.5f * 1000.0f;
	Snapshot.PPV[2] = (VelMax[2] - VelMin[2]) * 0.5f * 1000.0f;

	if(FftInputReady){
		/* 축별 임계값 검사 — PPV ≥ 0.5 mm/s 또는 Peak ≥ 30 mg 일 때만 FFT 의미
		   (정지 시 노이즈 floor 의 무작위 dominant Hz 출력 방지) */
		int32_t ax;
		for(ax = 0; ax < 3; ax++){
			if(Snapshot.PPV[ax] >= MEASUREMENT_PPV_THRESHOLD_MMPS ||
			   PeakMag           >= MEASUREMENT_PEAK_THRESHOLD_G){
				Snapshot.DominantHz[ax] = Measurement_RunFFT(ax);
			}
			else{
				Snapshot.DominantHz[ax] = 0;	/* 노이즈 floor — Hz 의미 없음 */
			}
		}
	}
	else{
		Snapshot.DominantHz[0] = 0;
		Snapshot.DominantHz[1] = 0;
		Snapshot.DominantHz[2] = 0;
	}

	Snapshot.Temperature = MiMems.Temperature;

	oSerial_Log("MEMS", "ACC[%7.4f,%7.4f,%7.4f]g PPV[%6.3f,%6.3f,%6.3f]mm/s Hz[%3d,%3d,%3d] T=%5.2f",
	            (double)Snapshot.PeakAccel[0],
	            (double)Snapshot.PeakAccel[1],
	            (double)Snapshot.PeakAccel[2],
	            (double)Snapshot.PPV[0],
	            (double)Snapshot.PPV[1],
	            (double)Snapshot.PPV[2],
	            (int)Snapshot.DominantHz[0],
	            (int)Snapshot.DominantHz[1],
	            (int)Snapshot.DominantHz[2],
	            (double)Snapshot.Temperature);
}

/* ============================================================
 * ProcessBatch — FIFO unread count 기반 burst read + sample loop
 * ============================================================ */
static void Measurement_ProcessBatch(void)
{
	uint16_t unread = 0;
	uint16_t toRead;
	uint16_t i;
	uint8_t  *pEntry;
	uint8_t  tag;
	int16_t  raw_x, raw_y, raw_z;

	/* FIFO 가 비워질 때까지 반복 — INT1 재트리거 조건 (unread < watermark) 보장 */
	for(;;){
		if(ISM330DHCX_FIFO_GetUnreadCount(&MiMems, &unread) != RESULT_OK){
			return;
		}
		if(unread == 0){
			return;
		}

		toRead = (unread > MEASUREMENT_FIFO_WATERMARK) ? MEASUREMENT_FIFO_WATERMARK : unread;

		if(ISM330DHCX_FIFO_ReadBurst(&MiMems, FifoBurstBuf, toRead) != RESULT_OK){
			return;
		}

		for(i = 0; i < toRead; i++){
			pEntry = &FifoBurstBuf[i * ISM330DHCX_FIFO_ENTRY_SIZE];
			tag = ISM330DHCX_FIFO_GET_TAG(pEntry[0]);

			if(tag != ISM330DHCX_TAG_ACCEL_NC){
				continue;
			}

			raw_x = (int16_t)(((uint16_t)pEntry[2] << 8) | pEntry[1]);
			raw_y = (int16_t)(((uint16_t)pEntry[4] << 8) | pEntry[3]);
			raw_z = (int16_t)(((uint16_t)pEntry[6] << 8) | pEntry[5]);

			Measurement_ProcessSample(raw_x, raw_y, raw_z);
		}

		/* 잔여가 watermark 이하면 종료 — 다음 IRQ 가 정상 발생 */
		if(unread <= MEASUREMENT_FIFO_WATERMARK){
			return;
		}
	}
}

/* ============================================================
 * Process — main loop 진입점
 *   IRQ flag 또는 fallback 타임아웃 시 batch 수행
 * ============================================================ */
void Measurement_Process(void)
{
	switch(MeasState)
	{
		case MS_INIT:
			if(Measurement_Init() == RESULT_OK){
				MeasState = MS_RUN;
				oSerial_Log("MEMS", "Init OK — FIFO WM=%d FFT=%dpt @ %dHz",
				            MEASUREMENT_FIFO_WATERMARK, FFT_SIZE, MEASUREMENT_ODR_HZ);
			}
			break;

		case MS_RUN:
			/* IRQ 또는 fallback 트리거 */
			if(MiMeasurement_FifoIrqFlag || oTMR_Elapsed(&FallbackTimer, MEASUREMENT_FALLBACK_MS, TICKBASE_SYSTICK)){
				MiMeasurement_FifoIrqFlag = 0;
				FallbackTimer = oTMR_GetTick(TICKBASE_SYSTICK);
				Measurement_ProcessBatch();

				if(SampleCount >= MEASUREMENT_WINDOW_SAMPLES){
					Measurement_ComputeSnapshot();
					Measurement_ResetWindow();
				}
			}
			break;
	}
}

/* ============================================================
 * Sensor — IoT 패킷 출력
 * ============================================================ */
oResult_t Measurement_Sensor(IoT_DataPacket_t *pPacket)
{
	IoTDataVibration_t *pFrame;

	if(pPacket == NULL || MeasState != MS_RUN){
		return RESULT_ERROR;
	}

	pPacket->TypeOfData = IoTDataType_Vibration;
	pPacket->DLC = sizeof(IoTDataVibration_t);

	pFrame = (IoTDataVibration_t *)&pPacket->Frame;
	memset(pFrame, 0, sizeof(IoTDataVibration_t));

	pFrame->PPV_X = (uint16_t)MATH_MIN(Snapshot.PPV[0], 65535);
	pFrame->PPV_Y = (uint16_t)MATH_MIN(Snapshot.PPV[1], 65535);
	pFrame->PPV_Z = (uint16_t)MATH_MIN(Snapshot.PPV[2], 65535);

	pFrame->Freq_X = (uint16_t)MATH_MIN(Snapshot.DominantHz[0], 65535);
	pFrame->Freq_Y = (uint16_t)MATH_MIN(Snapshot.DominantHz[1], 65535);
	pFrame->Freq_Z = (uint16_t)MATH_MIN(Snapshot.DominantHz[2], 65535);

	return RESULT_OK;
}

/* ============================================================
 * Supply — PHM HW0.1 ADC 없음 → 고정값
 * ============================================================ */
oResult_t Measurement_Supply(uint8_t Count)
{
	(void)Count;
	GPIOs.ADC.SystemSupply = 3300;
	GPIOs.ADC.InternalBAT  = 3300;
	return RESULT_OK;
}

/* History

2026-06-26 | v0.1
	- baseline (Mi_Measurement.c)
*/
