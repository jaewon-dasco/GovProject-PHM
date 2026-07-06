/*
 * ONE_FFT.c
 *
 *  Version: 0.1 (2026-07-03)
 */

#include <math.h>
#include "ONE_FFT.h"

/* ============================================================
 * Twiddle table precompute (cos, sin interleaved, N/2 pair)
 * ============================================================ */
void oFFT_Init(oFFT_t *pFft, float *pTwiddles, float *pScratch, uint32_t N)
{
	uint32_t i;

	pFft->pTwiddles = pTwiddles;
	pFft->pScratch  = pScratch;
	pFft->N         = N;

	for(i = 0; i < N/2; i++){
		float angle = (float)(-2.0 * MATH_PI * (double)i / (double)N);
		pTwiddles[2*i]     = cosf(angle);
		pTwiddles[2*i + 1] = sinf(angle);
	}

	pFft->Ready = 1;
}

/* In-place radix-2 DIT complex FFT
 *   pData     : interleaved complex [Re0,Im0, Re1,Im1, ..., Re_{N-1},Im_{N-1}], length 2*N
 *   Direction : +1 forward, -1 inverse (unscaled) */
void oFFT_Complex(oFFT_t *pFft, float *pData, int32_t Direction)
{
	uint32_t i, j, k, size, half, step, tw, i1, i2, N = pFft->N;
	float wr, wi, xr, xi, tr, ti;
	float *pTw = pFft->pTwiddles;

	/* Bit-reversal permutation */
	j = 0;
	for(i = 1; i < N; i++){
		uint32_t bit = N >> 1;
		while(j & bit){ j ^= bit; bit >>= 1; }
		j ^= bit;
		if(i < j){
			tr = pData[2*i];     pData[2*i]     = pData[2*j];     pData[2*j]     = tr;
			ti = pData[2*i + 1]; pData[2*i + 1] = pData[2*j + 1]; pData[2*j + 1] = ti;
		}
	}

	/* Butterflies */
	for(size = 2; size <= N; size <<= 1){
		half = size >> 1;
		step = N / size;
		for(i = 0; i < N; i += size){
			for(k = 0; k < half; k++){
				tw = k * step;
				wr = pTw[2*tw];
				wi = pTw[2*tw + 1] * (float)Direction;	/* inverse: conjugate twiddle */
				i1 = i + k;
				i2 = i + k + half;
				xr = pData[2*i2];
				xi = pData[2*i2 + 1];
				tr = xr*wr - xi*wi;
				ti = xr*wi + xi*wr;
				pData[2*i2]     = pData[2*i1]     - tr;
				pData[2*i2 + 1] = pData[2*i1 + 1] - ti;
				pData[2*i1]     = pData[2*i1]     + tr;
				pData[2*i1 + 1] = pData[2*i1 + 1] + ti;
			}
		}
	}
}

/* Real FFT wrapper — CMSIS arm_rfft_fast_f32 API 호환 packed 포맷
 *   Inverse=0 (forward): pIn=real[N], pOut=packed[N]
 *      pOut[0]=Re(X[0]) (DC), pOut[1]=Re(X[N/2]) (Nyquist)
 *      pOut[2k]=Re(X[k]), pOut[2k+1]=Im(X[k]) for k=1..N/2-1
 *   Inverse=1 (backward): pIn=packed[N], pOut=real[N] (1/N 스케일) */
void oFFT_RealFast(oFFT_t *pFft, float *pIn, float *pOut, int32_t Inverse)
{
	uint32_t i, k, N = pFft->N;
	float *pScratch = pFft->pScratch;
	float invN;

	if(!Inverse){
		/* Forward: 실수 입력을 complex 로 (Im=0) 로드 → 전체 complex FFT → packed 로 추출 */
		for(i = 0; i < N; i++){
			pScratch[2*i]     = pIn[i];
			pScratch[2*i + 1] = 0.0f;
		}
		oFFT_Complex(pFft, pScratch, 1);

		pOut[0] = pScratch[0];			/* Re(DC) */
		pOut[1] = pScratch[2 * (N/2)];	/* Re(Nyquist) */
		for(k = 1; k < N/2; k++){
			pOut[2*k]     = pScratch[2*k];
			pOut[2*k + 1] = pScratch[2*k + 1];
		}
	}else{
		/* Inverse: packed 입력 → 전체 complex (conjugate symmetry) 재구성 → complex IFFT → real 추출 (1/N 스케일) */
		pScratch[0]             = pIn[0];		/* DC (real) */
		pScratch[1]             = 0.0f;
		pScratch[2*(N/2)]       = pIn[1];		/* Nyquist (real) */
		pScratch[2*(N/2) + 1]   = 0.0f;
		for(k = 1; k < N/2; k++){
			pScratch[2*k]           = pIn[2*k];		/* X[k]      = Re + jIm */
			pScratch[2*k + 1]       = pIn[2*k + 1];
			pScratch[2*(N-k)]       = pIn[2*k];		/* X[N-k]    = conj(X[k]) */
			pScratch[2*(N-k) + 1]   = -pIn[2*k + 1];
		}
		oFFT_Complex(pFft, pScratch, -1);

		invN = 1.0f / (float)N;
		for(i = 0; i < N; i++){
			pOut[i] = pScratch[2*i] * invN;
		}
	}
}
