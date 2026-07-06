/*
 * ONE_FFT.h
 *
 *  Created on: Jul 3, 2026
 *      Author: JONE
 *
 * Radix-2 DIT FFT (float, in-place)
 * Real FFT wrapper — CMSIS arm_rfft_fast_f32 packed 포맷 호환
 */

#ifndef INC_ONE_FFT_H_
#define INC_ONE_FFT_H_

#define ONE_FFT_VERSION		0.1

#include "ONE_Math.h"

/* ============================================================
 * oFFT_t — N-point 실수 FFT 컨텍스트
 * ============================================================
 * pTwiddles : N개 float 버퍼 (N/2 pair of cos,sin) — 호출자 할당, static 권장
 * pScratch  : 2*N개 float 버퍼 (복소수 작업용) — 호출자 할당, static 권장
 * N         : FFT 크기 (2의 거듭제곱)
 *
 * Usage:
 *   static float Twiddles[FFT_SIZE];
 *   static float Scratch[2 * FFT_SIZE];
 *   static oFFT_t Fft;
 *
 *   oFFT_Init(&Fft, Twiddles, Scratch, FFT_SIZE);   // 부팅 1회
 *   oFFT_RealFast(&Fft, Input, Output, 0);          // forward
 *   oFFT_RealFast(&Fft, Output, Input, 1);          // inverse
 */
typedef struct {
	float    *pTwiddles;
	float    *pScratch;
	uint32_t N;
	uint8_t  Ready;
} oFFT_t;

extern void oFFT_Init(oFFT_t *pFft, float *pTwiddles, float *pScratch, uint32_t N);
extern void oFFT_Complex(oFFT_t *pFft, float *pData, int32_t Direction);
extern void oFFT_RealFast(oFFT_t *pFft, float *pIn, float *pOut, int32_t Inverse);

#endif /* INC_ONE_FFT_H_ */

/* History

2026-07-03 | v0.1
	- baseline (ONE_FFT.h) — Mi_Measurement.c 자체 FFT 구현 추출

*/
