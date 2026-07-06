/*
 * Mi_Measurement.c
 *
 *  Version: 0.3 (2026-07-03)
 */
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
 *  CMSIS-DSP 미사용 — ONE_FFT (radix-2 DIT, float, 2048-pt)
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
#include "ONE_FFT.h"
#include "ONE_Serial.h"
#include "Mi_Main.h"
#include "Mi_IoT.h"
#include "Mi_Serial.h"
#include "Mi_Measurement.h"
#include "MEMS_ISM330DHCXTR.h"
#include "ONE_Filter.h"

/* ─── 측정 파라미터 ─── */
#define MEASUREMENT_ODR_HZ				1666
#define MEASUREMENT_WINDOW_SAMPLES		1666		/* 1 sec window @ 1666 Hz */
#define MEASUREMENT_FIFO_WATERMARK		500			/* entry — ~300 ms @ 1666 Hz (max 511) */
#define MEASUREMENT_OUTPUT_FFT_DIVIDER	1			/* N 회 FFT 마다 1회 log 출력 (전송 주기 = FFT 주기 × N) */
#define MEASUREMENT_FIFO_BURST_BYTES	(MEASUREMENT_FIFO_WATERMARK * ISM330DHCX_FIFO_ENTRY_SIZE)
#define MEASUREMENT_FALLBACK_MS			200			/* IRQ 누락 시 강제 polling 주기 */
#define MEASUREMENT_PI					3.14159265358979f
#define MEASUREMENT_DT_SEC				(1.0f / (float)MEASUREMENT_ODR_HZ)

/* ±4 g: 0.122 mg/LSB → g per LSB */
#define ACCEL_LSB_TO_G					0.122e-3f		/* g 단위 (mg/LSB ÷ 1000) */

/* 임시 게인 보정 — 추후 원인 해결 시 삭제 가능. 삭제 시 이 매크로 정의와
   Measurement_ProcessSample() 의 ACCEL_GAIN_CORRECTION 곱셈만 제거하면 됨 */
#define ACCEL_GAIN_CORRECTION			(1.0f / 1.737f)
#define G_TO_MPSS						9.80665f		/* g → m/s² 환산 (적분 시 사용) */

/* SW HPF — ONE_Filter Biquad, cutoff 1 Hz
   HW HPF 는 OFF, SW HPF 3축 적용 */
#define SW_HPF_CUTOFF_HZ				1.0f
#define SW_HPF_Q						0.707f		/* Butterworth flat */

/* ─── FFT ─── */
#define FFT_SIZE						2048		/* zero-pad to next 2^N (1660 → 2048) */
#define FFT_SIZE_LOG2					11

/* ─── FFT dominant bin 검색 대역 (brick-wall band masking) ─── */
#define PEAK_SEARCH_LOW_HZ				5.0f		/* 이전 1.0 Hz → 5.0 Hz: 저주파 1/f 속도 증폭으로 인한 PPV 과대평가 억제 */
#define PEAK_SEARCH_HIGH_HZ				800.0f		/* ODR 1666 Hz, Nyquist 833 Hz 이하 */

/* ─── 신호/노이즈 판별 임계값 (로그 통계 P95~P99 기반) ─── */
#define MEASUREMENT_PPV_THRESHOLD_MMPS	0.8f		/* 0.8 mm/s 미만 = 노이즈 floor (95 Hz spike 차단) */
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

/* ─── Peak 추적 (max |a| 의 순간 3축 동시 캡처) — HW HPF 후 가속도 기반 ─── */
static float PeakMag = 0;
static float PeakAccel[3] = {0, 0, 0};

/* ─── FFT ring buffer (3축 HPF 가속도, 윈도우 크기 = 1660 sample) ─── */
static float InputRing[3][MEASUREMENT_WINDOW_SAMPLES];
static uint32_t InputRingIdx = 0;
static uint8_t  InputRingReady = 0;

/* ─── ODR 자동 보정 — ISM330DHCX RC 오실레이터 ±10 % drift 대응
       부팅 시 16660 샘플 모이면 SysTick 으로 실측 → ActualOdrHz 갱신 (~10 sec @ 1666 Hz)
       이후 LPF(α=0.25) 로 온도 드리프트 추적
       FFT bin / V[k]=A[k]/2πf 모두 이 값을 사용 */
static volatile float ActualOdrHz   = (float)MEASUREMENT_ODR_HZ;
static uint8_t        OdrCalibrated = 0;

/* ─── SW HPF 3축 (ONE_Filter Biquad, 2 Hz cutoff) ─── */
static oFilter_t HpfX = FILTER_PASS_INITIALIZER((double)MEASUREMENT_ODR_HZ, (double)SW_HPF_CUTOFF_HZ, (double)SW_HPF_Q);
static oFilter_t HpfY = FILTER_PASS_INITIALIZER((double)MEASUREMENT_ODR_HZ, (double)SW_HPF_CUTOFF_HZ, (double)SW_HPF_Q);
static oFilter_t HpfZ = FILTER_PASS_INITIALIZER((double)MEASUREMENT_ODR_HZ, (double)SW_HPF_CUTOFF_HZ, (double)SW_HPF_Q);

/* ─── FFT (ONE_FFT — 2048-pt real, CMSIS packed 포맷 호환) ─── */
static float FftInput[FFT_SIZE];				/* 시간 도메인 실수 입력 (zero-pad 적용) */
static float FftOutput[FFT_SIZE];				/* 주파수 도메인 packed real: [Re0, Re_N/2, Re1, Im1, ..., Re_N/2-1, Im_N/2-1] */
static float FftScratch[2 * FFT_SIZE];			/* ONE_FFT 내부 complex 작업 버퍼 (Re/Im interleaved) */
static float FftTwiddles[FFT_SIZE];				/* N/2 pair of (cos, sin) — 부팅 시 precompute */
static oFFT_t Fft;

/* ─── FIFO batch buffer ─── */
static uint8_t FifoBurstBuf[MEASUREMENT_FIFO_BURST_BYTES];

/* ─── ISR flag ─── */
volatile uint8_t MiMeasurement_FifoIrqFlag = 0;

/* ─── Snapshot ─── */
typedef struct {
	float MinAccel[3];		/* g min per axis (윈도우 내 최솟값) */
	float MaxAccel[3];		/* g max per axis (윈도우 내 최댓값) */
	float RmsAccel[3];		/* g RMS — ISO 16063 / 10816 */
	float PPV[3];			/* mm/s peak — DIN 4150-3 / ISO 4866 */
	float VelRms[3];		/* mm/s RMS — ISO 10816 / 16063 (내부 유지) */
	float DominantHz[3];	/* 축별 dominant frequency (X, Y, Z) */
	float Temperature;
	float PeakAccel[3];		/* 순간 3축 스냅샷 (max |a| 시점, 내부 유지) */
} MeasurementSnapshot_t;

static MeasurementSnapshot_t Snapshot;

/* ─── Fallback timer (IRQ 누락 대비) ─── */
static uint32_t FallbackTimer = 0;


/* ============================================================
 * FFT — ONE_FFT (2048-pt real, packed 포맷)
 *   axis: 0=X, 1=Y, 2=Z
 *   결과 magnitude 최대 bin 의 Hz 반환 (DC 제외)
 *   윈도우 1666 sample → 2048 zero-padding
 * ============================================================ */
static void Measurement_ComputeAxisStats(int32_t axis, float *outDominantHz, float *outPpvMmps, float *outVelRmsMmps)
{
	uint32_t i, src;
	float    maxMag2 = 0;
	uint32_t maxIdx  = 0;
	float    binHzScale = ActualOdrHz / (float)FFT_SIZE;	/* 실측 ODR 사용 (RC drift 보정) */
	uint32_t bin_lo = (uint32_t)(PEAK_SEARCH_LOW_HZ  / binHzScale);
	uint32_t bin_hi = (uint32_t)(PEAK_SEARCH_HIGH_HZ / binHzScale);
	float    vMax, vMin, re, im, freq, twoPiF, mag2;

	if(bin_lo < 1)              bin_lo = 1;
	if(bin_hi > FFT_SIZE/2 - 1) bin_hi = FFT_SIZE/2 - 1;

	/* 1. Ring buffer → FftInput (oldest~newest) + zero-pad */
	src = InputRingIdx;
	for(i = 0; i < MEASUREMENT_WINDOW_SAMPLES; i++){
		FftInput[i] = InputRing[axis][src];
		src = (src + 1) % MEASUREMENT_WINDOW_SAMPLES;
	}
	for(i = MEASUREMENT_WINDOW_SAMPLES; i < FFT_SIZE; i++){
		FftInput[i] = 0;
	}

	/* 2. Forward FFT — A[k] = FFT(accel in g)
	   Packed format:
	     FftOutput[0]   = Re(DC)
	     FftOutput[1]   = Re(Nyquist N/2)
	     FftOutput[2k]  = Re(bin k)   k = 1..N/2-1
	     FftOutput[2k+1]= Im(bin k)
	*/
	oFFT_RealFast(&Fft, FftInput, FftOutput, 0);

	/* 3. DC + Nyquist brick-wall (velocity 적분에서 1/f 가 무한대 회피) */
	FftOutput[0] = 0;
	FftOutput[1] = 0;

	/* 4. Bin loop — Brick-wall band mask 복원 (V[k] = A[k]/jω 의 1/f 증폭 방지)
	      대역 밖 (특히 < 1 Hz) bin 의 미세 노이즈가 1/f 로 폭주하는 것 차단 */
	for(i = 1; i < FFT_SIZE/2; i++){
		re = FftOutput[2*i];
		im = FftOutput[2*i + 1];

		if(i < bin_lo || i > bin_hi){
			FftOutput[2*i]     = 0;	/* 대역 밖 brick-wall — V[k] 적분 제외 */
			FftOutput[2*i + 1] = 0;
			continue;
		}

		mag2 = re*re + im*im;
		if(mag2 > maxMag2){
			maxMag2 = mag2;
			maxIdx  = i;
		}

		/* V[k] = A[k] / (j·2π·f) — 대역 안 bin 만 */
		freq = (float)i * binHzScale;
		twoPiF = 2.0f * MEASUREMENT_PI * freq;
		FftOutput[2*i]     =  im / twoPiF;
		FftOutput[2*i + 1] = -re / twoPiF;
	}

	*outDominantHz = (float)maxIdx * binHzScale;

	/* 5. Inverse FFT → velocity 시간 도메인 (g·sec 단위) */
	oFFT_RealFast(&Fft, FftOutput, FftInput, 1);

	/* 6. Velocity max/min + RMS (window 영역만 — zero-pad 영역 제외) */
	vMax = vMin = FftInput[0];
	float vSumSq = 0;
	for(i = 0; i < MEASUREMENT_WINDOW_SAMPLES; i++){
		float v = FftInput[i];
		if(v > vMax) vMax = v;
		if(v < vMin) vMin = v;
		vSumSq += v * v;
	}

	/* PPV = (vMax - vMin) / 2 × g(9.80665) × 1000 (g·sec → mm/s peak) */
	*outPpvMmps = (vMax - vMin) * 0.5f * G_TO_MPSS * 1000.0f;

	/* v_RMS = √(Σv²/N) × g × 1000 (g·sec → mm/s RMS) — ISO 10816/16063 */
	*outVelRmsMmps = sqrtf(vSumSq / (float)MEASUREMENT_WINDOW_SAMPLES) * G_TO_MPSS * 1000.0f;
}

/* ============================================================
 * Init — Sensor open + FIFO Continuous + watermark IRQ + HPF + FFT twiddle
 * ============================================================ */
oResult_t Measurement_Init(void)
{
	oResult_t result;

	GPIOs.DO.MEMSEnable = 1;

	result = ISM330DHCX_Open(&hspi1, DO_MEMS_CS, &MiMems);
	if(result != RESULT_OK){
		return result;
	}

	MiMems.Register.CTRL1_XL.ODR_XL = ISM330DHCX_XL_ODR_1_66KHZ;
	MiMems.Register.CTRL1_XL.FS_XL  = ISM330DHCX_XL_FS_4G;
	MiMems.Register.CTRL2_G.ODR_G   = ISM330DHCX_G_ODR_OFF;	/* gyro 미사용 — accel only */

	/* HW HPF — ODR/800 = 0.52 Hz cutoff (1 Hz 신호 ~97% 통과)
	   HP_SLOPE_XL_EN=1 → HPF 모드, HP_REF_MODE_XL=1 → 부팅 시 자동 reference 캡처 */
	MiMems.Register.CTRL8_XL.HPCF_XL          = ISM330DHCX_HPCF_XL_ODR_800;
	MiMems.Register.CTRL8_XL.HP_SLOPE_XL_EN   = 0;	/* HW HPF OFF — SW HPF (Biquad 2 Hz) 사용 */
	MiMems.Register.CTRL8_XL.HP_REF_MODE_XL   = 0;

	ISM330DHCX_SetConfig(&MiMems);

	/* FIFO Continuous + watermark + accel BDR 1.66 kHz */
	ISM330DHCX_FIFO_Setup(&MiMems, ISM330DHCX_FIFO_MODE_CONTINUOUS,
	                     MEASUREMENT_FIFO_WATERMARK,
	                     ISM330DHCX_BDR_1_66KHZ, ISM330DHCX_BDR_OFF);

	/* INT1 핀에 watermark 매핑 (PA8 EXTI8) */
	ISM330DHCX_FIFO_EnableINT1(&MiMems, 1, 0);

	memset(&Snapshot, 0, sizeof(Snapshot));

	PeakMag = 0;
	PeakAccel[0] = PeakAccel[1] = PeakAccel[2] = 0;

	memset(InputRing, 0, sizeof(InputRing));
	InputRingIdx = 0;
	InputRingReady = 0;

	MiMeasurement_FifoIrqFlag = 0;
	FallbackTimer = oTMR_GetTick(TICKBASE_SYSTICK);

	/* ONE_FFT twiddle table precompute (부팅 1회) */
	oFFT_Init(&Fft, FftTwiddles, FftScratch, FFT_SIZE);

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
 * 1 sample 처리 — HPF + FFT ring push (Peak/PPV/Hz 는 sliding 처리)
 * ============================================================ */
static inline void Measurement_ProcessSample(int16_t rawX, int16_t rawY, int16_t rawZ)
{
	/* raw → g → SW HPF (Biquad 2 Hz) → InputRing */
	float gX = (float)rawX * ACCEL_LSB_TO_G * ACCEL_GAIN_CORRECTION;
	float gY = (float)rawY * ACCEL_LSB_TO_G * ACCEL_GAIN_CORRECTION;
	float gZ = (float)rawZ * ACCEL_LSB_TO_G * ACCEL_GAIN_CORRECTION;
	InputRing[0][InputRingIdx] = (float)oFilter_HPF(&HpfX, (double)gX);
	InputRing[1][InputRingIdx] = (float)oFilter_HPF(&HpfY, (double)gY);
	InputRing[2][InputRingIdx] = (float)oFilter_HPF(&HpfZ, (double)gZ);
	InputRingIdx = (InputRingIdx + 1) % MEASUREMENT_WINDOW_SAMPLES;
	if(InputRingIdx == 0){
		InputRingReady = 1;
	}

	/* ODR 자동 보정 — 16660 샘플마다 SysTick 으로 실측 ODR 산출 (~10 sec @ 1666 Hz) */
	static uint32_t odrCnt = 0;
	static uint32_t odrTickStart = 0;
	if(odrCnt == 0){
		odrTickStart = oTMR_GetTick(TICKBASE_SYSTICK);
	}
	if(++odrCnt >= 16660){
		uint32_t dt = oTMR_GetTick(TICKBASE_SYSTICK) - odrTickStart;
		if(dt > 0){
			float measured = (float)odrCnt * 1000.0f / (float)dt;
			if(!OdrCalibrated){
				ActualOdrHz   = measured;					/* 1차 보정: 직접 대입 */
				OdrCalibrated = 1;
				oSerial_Log("ODR", "calibrated: %.2f Hz (nominal %d Hz)", (double)measured, MEASUREMENT_ODR_HZ);
			}else{
				ActualOdrHz = ActualOdrHz * 0.75f + measured * 0.25f;	/* LPF α=0.25 */
			}
		}
		odrCnt = 0;
	}
}

/* ============================================================
 * Sliding Peak + RMS 스캔 — 현재 ring (최근 1660 sample) 의 max |a| · 그 순간 3축 · 축별 RMS
 * ============================================================ */
static float RmsAccelAxis[3] = {0, 0, 0};	/* g RMS (ISO 16063/10816) */
static float MinAccelAxis[3] = {0, 0, 0};	/* g 축별 min */
static float MaxAccelAxis[3] = {0, 0, 0};	/* g 축별 max */

static void Measurement_ScanPeak(void)
{
	uint32_t i;
	float ax_v, ay_v, az_v, mag;
	float peakMag = 0;
	float peak[3] = {0, 0, 0};
	float sumSq[3] = {0, 0, 0};
	float axMin[3], axMax[3];

	axMin[0] = axMax[0] = InputRing[0][0];
	axMin[1] = axMax[1] = InputRing[1][0];
	axMin[2] = axMax[2] = InputRing[2][0];

	for(i = 0; i < MEASUREMENT_WINDOW_SAMPLES; i++){
		ax_v = InputRing[0][i];
		ay_v = InputRing[1][i];
		az_v = InputRing[2][i];

		/* 축별 min/max */
		if(ax_v < axMin[0]) axMin[0] = ax_v; else if(ax_v > axMax[0]) axMax[0] = ax_v;
		if(ay_v < axMin[1]) axMin[1] = ay_v; else if(ay_v > axMax[1]) axMax[1] = ay_v;
		if(az_v < axMin[2]) axMin[2] = az_v; else if(az_v > axMax[2]) axMax[2] = az_v;

		/* max |a| 순간 스냅샷 (내부 유지) */
		mag = fabsf(ax_v);
		if(fabsf(ay_v) > mag) mag = fabsf(ay_v);
		if(fabsf(az_v) > mag) mag = fabsf(az_v);
		if(mag > peakMag){
			peakMag = mag;
			peak[0] = ax_v;
			peak[1] = ay_v;
			peak[2] = az_v;
		}

		/* RMS 누적 */
		sumSq[0] += ax_v * ax_v;
		sumSq[1] += ay_v * ay_v;
		sumSq[2] += az_v * az_v;
	}

	PeakMag = peakMag;
	PeakAccel[0] = peak[0];
	PeakAccel[1] = peak[1];
	PeakAccel[2] = peak[2];

	MinAccelAxis[0] = axMin[0]; MinAccelAxis[1] = axMin[1]; MinAccelAxis[2] = axMin[2];
	MaxAccelAxis[0] = axMax[0]; MaxAccelAxis[1] = axMax[1]; MaxAccelAxis[2] = axMax[2];

	float invN = 1.0f / (float)MEASUREMENT_WINDOW_SAMPLES;
	RmsAccelAxis[0] = sqrtf(sumSq[0] * invN);
	RmsAccelAxis[1] = sqrtf(sumSq[1] * invN);
	RmsAccelAxis[2] = sqrtf(sumSq[2] * invN);
}

/* ============================================================
 * Snapshot 계산 — sliding window (FIFO batch 마다 호출)
 *   - Peak: 최근 1660 sample 의 max |a| 스캔
 *   - PPV/Hz: 축별 ComputeAxisStats (freq-domain 적분)
 * ============================================================ */
static void Measurement_ComputeSnapshot(void)
{
	int32_t ax;

	if(!InputRingReady){
		/* Ring 아직 가득 안 참 — 데이터 부족 */
		Snapshot.PeakAccel[0] = Snapshot.PeakAccel[1] = Snapshot.PeakAccel[2] = 0;
		Snapshot.MinAccel[0]  = Snapshot.MinAccel[1]  = Snapshot.MinAccel[2]  = 0;
		Snapshot.MaxAccel[0]  = Snapshot.MaxAccel[1]  = Snapshot.MaxAccel[2]  = 0;
		Snapshot.RmsAccel[0]  = Snapshot.RmsAccel[1]  = Snapshot.RmsAccel[2]  = 0;
		Snapshot.PPV[0]       = Snapshot.PPV[1]       = Snapshot.PPV[2]       = 0;
		Snapshot.VelRms[0]    = Snapshot.VelRms[1]    = Snapshot.VelRms[2]    = 0;
		Snapshot.DominantHz[0] = Snapshot.DominantHz[1] = Snapshot.DominantHz[2] = 0;
		return;
	}

	Measurement_ScanPeak();

	Snapshot.PeakAccel[0] = PeakAccel[0];
	Snapshot.PeakAccel[1] = PeakAccel[1];
	Snapshot.PeakAccel[2] = PeakAccel[2];
	Snapshot.MinAccel[0]  = MinAccelAxis[0];
	Snapshot.MinAccel[1]  = MinAccelAxis[1];
	Snapshot.MinAccel[2]  = MinAccelAxis[2];
	Snapshot.MaxAccel[0]  = MaxAccelAxis[0];
	Snapshot.MaxAccel[1]  = MaxAccelAxis[1];
	Snapshot.MaxAccel[2]  = MaxAccelAxis[2];
	Snapshot.RmsAccel[0]  = RmsAccelAxis[0];
	Snapshot.RmsAccel[1]  = RmsAccelAxis[1];
	Snapshot.RmsAccel[2]  = RmsAccelAxis[2];

	for(ax = 0; ax < 3; ax++){
		Measurement_ComputeAxisStats(ax, &Snapshot.DominantHz[ax], &Snapshot.PPV[ax], &Snapshot.VelRms[ax]);
	}

	/* 신호/노이즈 임계 — PPV·Peak 모두 미달 시 Hz=0 마스킹 (노이즈 floor 표시 차단) */
	for(ax = 0; ax < 3; ax++){
		if(Snapshot.PPV[ax] < MEASUREMENT_PPV_THRESHOLD_MMPS &&
		   PeakMag           < MEASUREMENT_PEAK_THRESHOLD_G){
			Snapshot.DominantHz[ax] = 0;
		}
	}

	Snapshot.Temperature = MiMems.Temperature;

	/* 출력 throttle — FFT N 회마다 1회 log 출력 (전송 주기 = FFT 주기 × DIVIDER) */
	static uint32_t fftRunCount = 0;
	if(++fftRunCount < MEASUREMENT_OUTPUT_FFT_DIVIDER){
		return;
	}
	fftRunCount = 0;

	oSerial_Log("MEMS",
	            "X[%+6.3f,%+6.3f]g Y[%+6.3f,%+6.3f]g Z[%+6.3f,%+6.3f]g "
	            "RMS[%+6.4f,%+6.4f,%+6.4f]g "
	            "PPV[%6.2f,%6.2f,%6.2f]mm/s "
	            "Hz[%3d,%3d,%3d]",
	            (double)Snapshot.MinAccel[0], (double)Snapshot.MaxAccel[0],
	            (double)Snapshot.MinAccel[1], (double)Snapshot.MaxAccel[1],
	            (double)Snapshot.MinAccel[2], (double)Snapshot.MaxAccel[2],
	            (double)Snapshot.RmsAccel[0], (double)Snapshot.RmsAccel[1], (double)Snapshot.RmsAccel[2],
	            (double)Snapshot.PPV[0],      (double)Snapshot.PPV[1],      (double)Snapshot.PPV[2],
	            (int)Snapshot.DominantHz[0],  (int)Snapshot.DominantHz[1],  (int)Snapshot.DominantHz[2]);
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
			/* IRQ 또는 fallback 트리거 — batch read 후 sliding snapshot */
			if(MiMeasurement_FifoIrqFlag || oTMR_Elapsed(&FallbackTimer, MEASUREMENT_FALLBACK_MS, TICKBASE_SYSTICK)){
				MiMeasurement_FifoIrqFlag = 0;
				FallbackTimer = oTMR_GetTick(TICKBASE_SYSTICK);
				Measurement_ProcessBatch();
				Measurement_ComputeSnapshot();	/* 매 batch 마다 sliding window 재계산·출력 */
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
