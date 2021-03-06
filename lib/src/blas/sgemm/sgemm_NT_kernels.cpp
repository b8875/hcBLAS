/*
Copyright (c) 2015-2016 Advanced Micro Devices, Inc. All rights reserved.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

#include "./sgemm_array_kernels.h"
#include <cmath>
#include <hc_math.hpp>

hcblasStatus gemm_NoTransA_MICRO_NBK_Mini_Batch_M128_N128_K16_TS16XMTS2_MB2(
    hc::accelerator_view accl_view, const float *A, __int64_t aOffset,
    const float *B, __int64_t bOffset, float *C, __int64_t cOffset, int M,
    int N, int K, int lda, int ldb, int ldc, float alpha, float beta) {
  int M_ = (M - 1) / 4 + 1;
  int N_ = (N - 1) / 4 + 1;
  int N_R = (N_ + 15) & ~15;
  int M_R = (M_ + 15) & ~15;
  int K_R = (K + 15) & ~15;
  hc::extent<2> grdExt(N_R, M_R);
  hc::tiled_extent<2> t_ext = grdExt.tile(16, 16);
  hc::parallel_for_each(accl_view, t_ext, [=](hc::tiled_index<2> tidx)[[hc]] {
    float rC[4][4] = {{static_cast<float>(0)}};
    float rA[1][4];
    float rB[1][4];
    tile_static float lA[16 * 32 * 2 + 16];
    tile_static float lB[16 * 32 * 2 + 16];
    int gidx = tidx.tile[1];
    int gidy = tidx.tile[0];
    int idx = tidx.local[1];
    int idy = tidx.local[0];
    int block_k = 0;
    int lIndex = (idy * (32 * 2 + 1)) + idx * 2;
    int AIndex = aOffset + (gidx * 32 * 2) + idx * 2 + (idy * lda);
    int BIndex = bOffset + (gidy * 32 * 2) + idx * 2 + (idy * ldb);
    __int64_t CIndex = cOffset + (gidx * 32 * 2) + idx * 2 +
                       (((gidy * 32 * 2) + idy * 2) * ldc);
    __int64_t CinitOffset = 0;
    do {
      tidx.barrier.wait();

      lB[lIndex + 0 * 16 * 2 + 0] =
          B[BIndex + block_k * 16 * ldb + 0 * 16 * 2 + 0];
      lB[lIndex + 0 * 16 * 2 + 1] =
          B[BIndex + block_k * 16 * ldb + 0 * 16 * 2 + 1];
      lB[lIndex + 1 * 16 * 2 + 0] =
          B[BIndex + block_k * 16 * ldb + 1 * 16 * 2 + 0];
      lB[lIndex + 1 * 16 * 2 + 1] =
          B[BIndex + block_k * 16 * ldb + 1 * 16 * 2 + 1];
      lA[lIndex + 0 * 16 * 2 + 0] =
          A[AIndex + block_k * 16 * lda + 0 * 16 * 2 + 0];
      lA[lIndex + 0 * 16 * 2 + 1] =
          A[AIndex + block_k * 16 * lda + 0 * 16 * 2 + 1];
      lA[lIndex + 1 * 16 * 2 + 0] =
          A[AIndex + block_k * 16 * lda + 1 * 16 * 2 + 0];
      lA[lIndex + 1 * 16 * 2 + 1] =
          A[AIndex + block_k * 16 * lda + 1 * 16 * 2 + 1];

      tidx.barrier.wait();

      int offA = idx * 2;
      int offB = idy * 2;

      for (int iter = 0; iter < 16; iter++) {
        M2x2_MB;
      }
    } while (++block_k < (K_R >> 4));

    tidx.barrier.wait();
    C[CIndex + CinitOffset + (0 * 2 + 0) * ldc + 0] =
        alpha * rC[0][0] +
        beta * C[CIndex + CinitOffset + (0 * 2 + 0) * ldc + 0];
    C[CIndex + CinitOffset + (0 * 2 + 1) * ldc + 0] =
        alpha * rC[0][1] +
        beta * C[CIndex + CinitOffset + (0 * 2 + 1) * ldc + 0];
    C[CIndex + CinitOffset + (16 * 2 + 0) * ldc + 0] =
        alpha * rC[0][2] +
        beta * C[CIndex + CinitOffset + (16 * 2 + 0) * ldc + 0];
    C[CIndex + CinitOffset + (16 * 2 + 1) * ldc + 0] =
        alpha * rC[0][3] +
        beta * C[CIndex + CinitOffset + (16 * 2 + 1) * ldc + 0];
    C[CIndex + CinitOffset + (0 * 2 + 0) * ldc + 1] =
        alpha * rC[1][0] +
        beta * C[CIndex + CinitOffset + (0 * 2 + 0) * ldc + 1];
    C[CIndex + CinitOffset + (0 * 2 + 1) * ldc + 1] =
        alpha * rC[1][1] +
        beta * C[CIndex + CinitOffset + (0 * 2 + 1) * ldc + 1];
    C[CIndex + CinitOffset + (16 * 2 + 0) * ldc + 1] =
        alpha * rC[1][2] +
        beta * C[CIndex + CinitOffset + (16 * 2 + 0) * ldc + 1];
    C[CIndex + CinitOffset + (16 * 2 + 1) * ldc + 1] =
        alpha * rC[1][3] +
        beta * C[CIndex + CinitOffset + (16 * 2 + 1) * ldc + 1];
    CinitOffset += 16 * 2;
    C[CIndex + CinitOffset + (0 * 2 + 0) * ldc + 0] =
        alpha * rC[2][0] +
        beta * C[CIndex + CinitOffset + (0 * 2 + 0) * ldc + 0];
    C[CIndex + CinitOffset + (0 * 2 + 1) * ldc + 0] =
        alpha * rC[2][1] +
        beta * C[CIndex + CinitOffset + (0 * 2 + 1) * ldc + 0];
    C[CIndex + CinitOffset + (16 * 2 + 0) * ldc + 0] =
        alpha * rC[2][2] +
        beta * C[CIndex + CinitOffset + (16 * 2 + 0) * ldc + 0];
    C[CIndex + CinitOffset + (16 * 2 + 1) * ldc + 0] =
        alpha * rC[2][3] +
        beta * C[CIndex + CinitOffset + (16 * 2 + 1) * ldc + 0];
    C[CIndex + CinitOffset + (0 * 2 + 0) * ldc + 1] =
        alpha * rC[3][0] +
        beta * C[CIndex + CinitOffset + (0 * 2 + 0) * ldc + 1];
    C[CIndex + CinitOffset + (0 * 2 + 1) * ldc + 1] =
        alpha * rC[3][1] +
        beta * C[CIndex + CinitOffset + (0 * 2 + 1) * ldc + 1];
    C[CIndex + CinitOffset + (16 * 2 + 0) * ldc + 1] =
        alpha * rC[3][2] +
        beta * C[CIndex + CinitOffset + (16 * 2 + 0) * ldc + 1];
    C[CIndex + CinitOffset + (16 * 2 + 1) * ldc + 1] =
        alpha * rC[3][3] +
        beta * C[CIndex + CinitOffset + (16 * 2 + 1) * ldc + 1];
  }) ;
  return HCBLAS_SUCCEEDS;
}

hcblasStatus gemm_NoTransA_MICRO_NBK_Mini_Batch_M128_N128_K16_TS16XMTS4_MB2(
    hc::accelerator_view accl_view, const float *A, __int64_t aOffset,
    const float *B, __int64_t bOffset, float *C, __int64_t cOffset, int M,
    int N, int K, int lda, int ldb, int ldc, float alpha, float beta) {
  int M_ = M >> 3;
  int N_ = N >> 3;
  int N_R = (N_ + 15) & ~15;
  int M_R = (M_ + 15) & ~15;
  int K_R = (K + 15) & ~15;
  hc::extent<2> grdExt(N_R, M_R);
  hc::tiled_extent<2> t_ext = grdExt.tile(16, 16);
  hc::parallel_for_each(accl_view, t_ext, [=](hc::tiled_index<2> tidx)[[hc]] {
    float rC[8][8] = {{static_cast<float>(0)}};
    float rA[1][8];
    float rB[1][8];
    tile_static float lA[16 * 64 * 2 + 16];
    tile_static float lB[16 * 64 * 2 + 16];
    int gidx = tidx.tile[1];
    int gidy = tidx.tile[0];
    int idx = tidx.local[1];
    int idy = tidx.local[0];
    int block_k = 0;
    int lIndex = (idy * (64 * 2 + 1)) + idx * 2;
    int AIndex = aOffset + (gidx * 64 * 2) + idx * 2 + (idy * lda);
    int BIndex = bOffset + (gidy * 64 * 2) + idx * 2 + (idy * ldb);
    __int64_t CIndex = cOffset + (gidx * 64 * 2) + idx * 2 +
                       (((gidy * 64 * 2) + idy * 2) * ldc);
    __int64_t CinitOffset = 0;

    do {
      tidx.barrier.wait();

      lB[lIndex + 0 * 16 * 2 + 0] =
          B[BIndex + block_k * 16 * ldb + 0 * 16 * 2 + 0];
      lB[lIndex + 0 * 16 * 2 + 1] =
          B[BIndex + block_k * 16 * ldb + 0 * 16 * 2 + 1];
      lB[lIndex + 1 * 16 * 2 + 0] =
          B[BIndex + block_k * 16 * ldb + 1 * 16 * 2 + 0];
      lB[lIndex + 1 * 16 * 2 + 1] =
          B[BIndex + block_k * 16 * ldb + 1 * 16 * 2 + 1];
      lB[lIndex + 2 * 16 * 2 + 0] =
          B[BIndex + block_k * 16 * ldb + 2 * 16 * 2 + 0];
      lB[lIndex + 2 * 16 * 2 + 1] =
          B[BIndex + block_k * 16 * ldb + 2 * 16 * 2 + 1];
      lB[lIndex + 3 * 16 * 2 + 0] =
          B[BIndex + block_k * 16 * ldb + 3 * 16 * 2 + 0];
      lB[lIndex + 3 * 16 * 2 + 1] =
          B[BIndex + block_k * 16 * ldb + 3 * 16 * 2 + 1];
      lA[lIndex + 0 * 16 * 2 + 0] =
          A[AIndex + block_k * 16 * lda + 0 * 16 * 2 + 0];
      lA[lIndex + 0 * 16 * 2 + 1] =
          A[AIndex + block_k * 16 * lda + 0 * 16 * 2 + 1];
      lA[lIndex + 1 * 16 * 2 + 0] =
          A[AIndex + block_k * 16 * lda + 1 * 16 * 2 + 0];
      lA[lIndex + 1 * 16 * 2 + 1] =
          A[AIndex + block_k * 16 * lda + 1 * 16 * 2 + 1];
      lA[lIndex + 2 * 16 * 2 + 0] =
          A[AIndex + block_k * 16 * lda + 2 * 16 * 2 + 0];
      lA[lIndex + 2 * 16 * 2 + 1] =
          A[AIndex + block_k * 16 * lda + 2 * 16 * 2 + 1];
      lA[lIndex + 3 * 16 * 2 + 0] =
          A[AIndex + block_k * 16 * lda + 3 * 16 * 2 + 0];
      lA[lIndex + 3 * 16 * 2 + 1] =
          A[AIndex + block_k * 16 * lda + 3 * 16 * 2 + 1];

      tidx.barrier.wait();
      int offA = idx * 2;
      int offB = idy * 2;

      for (int iter = 0; iter < 16; iter++) {
        M4x4_MB
      }

      tidx.barrier.wait();
    } while (++block_k < (K_R / 16));

    C[CIndex + CinitOffset + (0 * 2 + 0) * ldc + 0] =
        alpha * rC[0][0] +
        beta * C[CIndex + CinitOffset + (0 * 2 + 0) * ldc + 0];
    C[CIndex + CinitOffset + (0 * 2 + 1) * ldc + 0] =
        alpha * rC[0][1] +
        beta * C[CIndex + CinitOffset + (0 * 2 + 1) * ldc + 0];
    C[CIndex + CinitOffset + (16 * 2 + 0) * ldc + 0] =
        alpha * rC[0][2] +
        beta * C[CIndex + CinitOffset + (16 * 2 + 0) * ldc + 0];
    C[CIndex + CinitOffset + (16 * 2 + 1) * ldc + 0] =
        alpha * rC[0][3] +
        beta * C[CIndex + CinitOffset + (16 * 2 + 1) * ldc + 0];
    C[CIndex + CinitOffset + (32 * 2 + 0) * ldc + 0] =
        alpha * rC[0][4] +
        beta * C[CIndex + CinitOffset + (32 * 2 + 0) * ldc + 0];
    C[CIndex + CinitOffset + (32 * 2 + 1) * ldc + 0] =
        alpha * rC[0][5] +
        beta * C[CIndex + CinitOffset + (32 * 2 + 1) * ldc + 0];
    C[CIndex + CinitOffset + (48 * 2 + 0) * ldc + 0] =
        alpha * rC[0][6] +
        beta * C[CIndex + CinitOffset + (48 * 2 + 0) * ldc + 0];
    C[CIndex + CinitOffset + (48 * 2 + 1) * ldc + 0] =
        alpha * rC[0][7] +
        beta * C[CIndex + CinitOffset + (48 * 2 + 1) * ldc + 0];
    C[CIndex + CinitOffset + (0 * 2 + 0) * ldc + 1] =
        alpha * rC[1][0] +
        beta * C[CIndex + CinitOffset + (0 * 2 + 0) * ldc + 1];
    C[CIndex + CinitOffset + (0 * 2 + 1) * ldc + 1] =
        alpha * rC[1][1] +
        beta * C[CIndex + CinitOffset + (0 * 2 + 1) * ldc + 1];
    C[CIndex + CinitOffset + (16 * 2 + 0) * ldc + 1] =
        alpha * rC[1][2] +
        beta * C[CIndex + CinitOffset + (16 * 2 + 0) * ldc + 1];
    C[CIndex + CinitOffset + (16 * 2 + 1) * ldc + 1] =
        alpha * rC[1][3] +
        beta * C[CIndex + CinitOffset + (16 * 2 + 1) * ldc + 1];
    C[CIndex + CinitOffset + (32 * 2 + 0) * ldc + 1] =
        alpha * rC[1][4] +
        beta * C[CIndex + CinitOffset + (32 * 2 + 0) * ldc + 1];
    C[CIndex + CinitOffset + (32 * 2 + 1) * ldc + 1] =
        alpha * rC[1][5] +
        beta * C[CIndex + CinitOffset + (32 * 2 + 1) * ldc + 1];
    C[CIndex + CinitOffset + (48 * 2 + 0) * ldc + 1] =
        alpha * rC[1][6] +
        beta * C[CIndex + CinitOffset + (48 * 2 + 0) * ldc + 1];
    C[CIndex + CinitOffset + (48 * 2 + 1) * ldc + 1] =
        alpha * rC[1][7] +
        beta * C[CIndex + CinitOffset + (48 * 2 + 1) * ldc + 1];
    CinitOffset += 16 * 2;
    C[CIndex + CinitOffset + (0 * 2 + 0) * ldc + 0] =
        alpha * rC[2][0] +
        beta * C[CIndex + CinitOffset + (0 * 2 + 0) * ldc + 0];
    C[CIndex + CinitOffset + (0 * 2 + 1) * ldc + 0] =
        alpha * rC[2][1] +
        beta * C[CIndex + CinitOffset + (0 * 2 + 1) * ldc + 0];
    C[CIndex + CinitOffset + (16 * 2 + 0) * ldc + 0] =
        alpha * rC[2][2] +
        beta * C[CIndex + CinitOffset + (16 * 2 + 0) * ldc + 0];
    C[CIndex + CinitOffset + (16 * 2 + 1) * ldc + 0] =
        alpha * rC[2][3] +
        beta * C[CIndex + CinitOffset + (16 * 2 + 1) * ldc + 0];
    C[CIndex + CinitOffset + (32 * 2 + 0) * ldc + 0] =
        alpha * rC[2][4] +
        beta * C[CIndex + CinitOffset + (32 * 2 + 0) * ldc + 0];
    C[CIndex + CinitOffset + (32 * 2 + 1) * ldc + 0] =
        alpha * rC[2][5] +
        beta * C[CIndex + CinitOffset + (32 * 2 + 1) * ldc + 0];
    C[CIndex + CinitOffset + (48 * 2 + 0) * ldc + 0] =
        alpha * rC[2][6] +
        beta * C[CIndex + CinitOffset + (48 * 2 + 0) * ldc + 0];
    C[CIndex + CinitOffset + (48 * 2 + 1) * ldc + 0] =
        alpha * rC[2][7] +
        beta * C[CIndex + CinitOffset + (48 * 2 + 1) * ldc + 0];
    C[CIndex + CinitOffset + (0 * 2 + 0) * ldc + 1] =
        alpha * rC[3][0] +
        beta * C[CIndex + CinitOffset + (0 * 2 + 0) * ldc + 1];
    C[CIndex + CinitOffset + (0 * 2 + 1) * ldc + 1] =
        alpha * rC[3][1] +
        beta * C[CIndex + CinitOffset + (0 * 2 + 1) * ldc + 1];
    C[CIndex + CinitOffset + (16 * 2 + 0) * ldc + 1] =
        alpha * rC[3][2] +
        beta * C[CIndex + CinitOffset + (16 * 2 + 0) * ldc + 1];
    C[CIndex + CinitOffset + (16 * 2 + 1) * ldc + 1] =
        alpha * rC[3][3] +
        beta * C[CIndex + CinitOffset + (16 * 2 + 1) * ldc + 1];
    C[CIndex + CinitOffset + (32 * 2 + 0) * ldc + 1] =
        alpha * rC[3][4] +
        beta * C[CIndex + CinitOffset + (32 * 2 + 0) * ldc + 1];
    C[CIndex + CinitOffset + (32 * 2 + 1) * ldc + 1] =
        alpha * rC[3][5] +
        beta * C[CIndex + CinitOffset + (32 * 2 + 1) * ldc + 1];
    C[CIndex + CinitOffset + (48 * 2 + 0) * ldc + 1] =
        alpha * rC[3][6] +
        beta * C[CIndex + CinitOffset + (48 * 2 + 0) * ldc + 1];
    C[CIndex + CinitOffset + (48 * 2 + 1) * ldc + 1] =
        alpha * rC[3][7] +
        beta * C[CIndex + CinitOffset + (48 * 2 + 1) * ldc + 1];
    CinitOffset += 16 * 2;
    C[CIndex + CinitOffset + (0 * 2 + 0) * ldc + 0] =
        alpha * rC[4][0] +
        beta * C[CIndex + CinitOffset + (0 * 2 + 0) * ldc + 0];
    C[CIndex + CinitOffset + (0 * 2 + 1) * ldc + 0] =
        alpha * rC[4][1] +
        beta * C[CIndex + CinitOffset + (0 * 2 + 1) * ldc + 0];
    C[CIndex + CinitOffset + (16 * 2 + 0) * ldc + 0] =
        alpha * rC[4][2] +
        beta * C[CIndex + CinitOffset + (16 * 2 + 0) * ldc + 0];
    C[CIndex + CinitOffset + (16 * 2 + 1) * ldc + 0] =
        alpha * rC[4][3] +
        beta * C[CIndex + CinitOffset + (16 * 2 + 1) * ldc + 0];
    C[CIndex + CinitOffset + (32 * 2 + 0) * ldc + 0] =
        alpha * rC[4][4] +
        beta * C[CIndex + CinitOffset + (32 * 2 + 0) * ldc + 0];
    C[CIndex + CinitOffset + (32 * 2 + 1) * ldc + 0] =
        alpha * rC[4][5] +
        beta * C[CIndex + CinitOffset + (32 * 2 + 1) * ldc + 0];
    C[CIndex + CinitOffset + (48 * 2 + 0) * ldc + 0] =
        alpha * rC[4][6] +
        beta * C[CIndex + CinitOffset + (48 * 2 + 0) * ldc + 0];
    C[CIndex + CinitOffset + (48 * 2 + 1) * ldc + 0] =
        alpha * rC[4][7] +
        beta * C[CIndex + CinitOffset + (48 * 2 + 1) * ldc + 0];
    C[CIndex + CinitOffset + (0 * 2 + 0) * ldc + 1] =
        alpha * rC[5][0] +
        beta * C[CIndex + CinitOffset + (0 * 2 + 0) * ldc + 1];
    C[CIndex + CinitOffset + (0 * 2 + 1) * ldc + 1] =
        alpha * rC[5][1] +
        beta * C[CIndex + CinitOffset + (0 * 2 + 1) * ldc + 1];
    C[CIndex + CinitOffset + (16 * 2 + 0) * ldc + 1] =
        alpha * rC[5][2] +
        beta * C[CIndex + CinitOffset + (16 * 2 + 0) * ldc + 1];
    C[CIndex + CinitOffset + (16 * 2 + 1) * ldc + 1] =
        alpha * rC[5][3] +
        beta * C[CIndex + CinitOffset + (16 * 2 + 1) * ldc + 1];
    C[CIndex + CinitOffset + (32 * 2 + 0) * ldc + 1] =
        alpha * rC[5][4] +
        beta * C[CIndex + CinitOffset + (32 * 2 + 0) * ldc + 1];
    C[CIndex + CinitOffset + (32 * 2 + 1) * ldc + 1] =
        alpha * rC[5][5] +
        beta * C[CIndex + CinitOffset + (32 * 2 + 1) * ldc + 1];
    C[CIndex + CinitOffset + (48 * 2 + 0) * ldc + 1] =
        alpha * rC[5][6] +
        beta * C[CIndex + CinitOffset + (48 * 2 + 0) * ldc + 1];
    C[CIndex + CinitOffset + (48 * 2 + 1) * ldc + 1] =
        alpha * rC[5][7] +
        beta * C[CIndex + CinitOffset + (48 * 2 + 1) * ldc + 1];
    CinitOffset += 16 * 2;
    C[CIndex + CinitOffset + (0 * 2 + 0) * ldc + 0] =
        alpha * rC[6][0] +
        beta * C[CIndex + CinitOffset + (0 * 2 + 0) * ldc + 0];
    C[CIndex + CinitOffset + (0 * 2 + 1) * ldc + 0] =
        alpha * rC[6][1] +
        beta * C[CIndex + CinitOffset + (0 * 2 + 1) * ldc + 0];
    C[CIndex + CinitOffset + (16 * 2 + 0) * ldc + 0] =
        alpha * rC[6][2] +
        beta * C[CIndex + CinitOffset + (16 * 2 + 0) * ldc + 0];
    C[CIndex + CinitOffset + (16 * 2 + 1) * ldc + 0] =
        alpha * rC[6][3] +
        beta * C[CIndex + CinitOffset + (16 * 2 + 1) * ldc + 0];
    C[CIndex + CinitOffset + (32 * 2 + 0) * ldc + 0] =
        alpha * rC[6][4] +
        beta * C[CIndex + CinitOffset + (32 * 2 + 0) * ldc + 0];
    C[CIndex + CinitOffset + (32 * 2 + 1) * ldc + 0] =
        alpha * rC[6][5] +
        beta * C[CIndex + CinitOffset + (32 * 2 + 1) * ldc + 0];
    C[CIndex + CinitOffset + (48 * 2 + 0) * ldc + 0] =
        alpha * rC[6][6] +
        beta * C[CIndex + CinitOffset + (48 * 2 + 0) * ldc + 0];
    C[CIndex + CinitOffset + (48 * 2 + 1) * ldc + 0] =
        alpha * rC[6][7] +
        beta * C[CIndex + CinitOffset + (48 * 2 + 1) * ldc + 0];
    C[CIndex + CinitOffset + (0 * 2 + 0) * ldc + 1] =
        alpha * rC[7][0] +
        beta * C[CIndex + CinitOffset + (0 * 2 + 0) * ldc + 1];
    C[CIndex + CinitOffset + (0 * 2 + 1) * ldc + 1] =
        alpha * rC[7][1] +
        beta * C[CIndex + CinitOffset + (0 * 2 + 1) * ldc + 1];
    C[CIndex + CinitOffset + (16 * 2 + 0) * ldc + 1] =
        alpha * rC[7][2] +
        beta * C[CIndex + CinitOffset + (16 * 2 + 0) * ldc + 1];
    C[CIndex + CinitOffset + (16 * 2 + 1) * ldc + 1] =
        alpha * rC[7][3] +
        beta * C[CIndex + CinitOffset + (16 * 2 + 1) * ldc + 1];
    C[CIndex + CinitOffset + (32 * 2 + 0) * ldc + 1] =
        alpha * rC[7][4] +
        beta * C[CIndex + CinitOffset + (32 * 2 + 0) * ldc + 1];
    C[CIndex + CinitOffset + (32 * 2 + 1) * ldc + 1] =
        alpha * rC[7][5] +
        beta * C[CIndex + CinitOffset + (32 * 2 + 1) * ldc + 1];
    C[CIndex + CinitOffset + (48 * 2 + 0) * ldc + 1] =
        alpha * rC[7][6] +
        beta * C[CIndex + CinitOffset + (48 * 2 + 0) * ldc + 1];
    C[CIndex + CinitOffset + (48 * 2 + 1) * ldc + 1] =
        alpha * rC[7][7] +
        beta * C[CIndex + CinitOffset + (48 * 2 + 1) * ldc + 1];
  }) ;
#undef TILESIZE
#undef MICROTILESIZE
  return HCBLAS_SUCCEEDS;
}

hcblasStatus gemm_NoTransA_MICRO_NBK_Mini_Batch_M_N_K_TS16XMTS2_MB2(
    hc::accelerator_view accl_view, const float *A, __int64_t aOffset,
    const float *B, __int64_t bOffset, float *C, __int64_t cOffset, int M,
    int N, int K, int lda, int ldb, int ldc, float alpha, float beta) {
  int M_ = (M - 1) / 4 + 1;
  int N_ = (N - 1) / 4 + 1;
  int N_R = (N_ + 15) & ~15;
  int M_R = (M_ + 15) & ~15;
  int K_R = (K + 15) & ~15;
  hc::extent<2> grdExt(N_R, M_R);
  hc::tiled_extent<2> t_ext = grdExt.tile(16, 16);
  hc::parallel_for_each(accl_view, t_ext, [=](hc::tiled_index<2> tidx)[[hc]] {
    float rC[4][4] = {{static_cast<float>(0)}};
    float rA[1][4];
    float rB[1][4];
    tile_static float lA[16 * 32 * 2 + 16];
    tile_static float lB[16 * 32 * 2 + 16];
    int gidx = tidx.tile[1];
    int gidy = tidx.tile[0];
    int idx = tidx.local[1];
    int idy = tidx.local[0];
    int block_k = 0;
    int alIndex = (idy * (32 * 2 + 1)) + idx * 2;
    int blIndex = (idy * (32 * 2 + 1)) + idx * 2;
    int AIndex = aOffset + (gidx * 32 * 2) + idx * 2 + (idy * lda);
    int BIndex = bOffset + (gidy * 32 * 2) + idx * 2 + (idy * ldb);
    __int64_t CIndex = cOffset + (gidx * 32 * 2) + idx * 2 +
                       (((gidy * 32 * 2) + idy * 2) * ldc);
    __int64_t AinitOffset = 0;
    __int64_t BinitOffset = 0;
    __int64_t CinitOffset = 0;
    int N_block = N_R >> 4;
    int M_block = M_R >> 4;
    int K_block = K_R >> 4;
    do {
      tidx.barrier.wait();

      if (gidx == M_block - 1 || gidy == N_block - 1 ||
          block_k == K_block - 1) {
        for (int sec = 0; sec < 2; ++sec) {
          int secVal = sec << 4;

          if ((gidy * 32 * 2 + idx * 2 + secVal * 2 + 0) < N &&
              (block_k * 16 + idy) < K) {
            lB[blIndex + secVal * 2 + 0] =
                B[BIndex + BinitOffset + secVal * 2 + 0];
          } else {
            lB[blIndex + secVal * 2 + 0] = 0;
          }
          if ((gidy * 32 * 2 + idx * 2 + secVal * 2 + 1) < N &&
              (block_k * 16 + idy) < K) {
            lB[blIndex + secVal * 2 + 1] =
                B[BIndex + BinitOffset + secVal * 2 + 1];
          } else {
            lB[blIndex + secVal * 2 + 1] = 0;
          }

          if ((gidx * 32 * 2 + idx * 2 + secVal * 2 + 0) < M &&
              (block_k * 16 + idy) < K) {
            lA[alIndex + secVal * 2 + 0] =
                A[AIndex + secVal * 2 + 0 + AinitOffset];
          } else {
            lA[alIndex + secVal * 2 + 0] = 0;
          }
          if ((gidx * 32 * 2 + idx * 2 + secVal * 2 + 1) < M &&
              (block_k * 16 + idy) < K) {
            lA[alIndex + secVal * 2 + 1] =
                A[AIndex + secVal * 2 + 1 + AinitOffset];
          } else {
            lA[alIndex + secVal * 2 + 1] = 0;
          }
        }
      } else {
        lB[blIndex + 0 * 2 + 0] = B[BIndex + BinitOffset + 0 * 2 + 0];
        lB[blIndex + 0 * 2 + 1] = B[BIndex + BinitOffset + 0 * 2 + 1];
        lB[blIndex + 16 * 2 + 0] = B[BIndex + BinitOffset + 16 * 2 + 0];
        lB[blIndex + 16 * 2 + 1] = B[BIndex + BinitOffset + 16 * 2 + 1];
        lA[alIndex + 0 * 2 + 0] = A[AIndex + 0 * 2 + 0 + AinitOffset];
        lA[alIndex + 0 * 2 + 1] = A[AIndex + 0 * 2 + 1 + AinitOffset];
        lA[alIndex + 16 * 2 + 0] = A[AIndex + 16 * 2 + 0 + AinitOffset];
        lA[alIndex + 16 * 2 + 1] = A[AIndex + 16 * 2 + 1 + AinitOffset];
      }

      tidx.barrier.wait();

      int offA = idx * 2;
      int offB = idy * 2;

      for (int iter = 0; iter < 16; iter++) {
        M2x2_MB;
      }

      AinitOffset += lda << 4;
      BinitOffset += ldb << 4;
    } while (++block_k < (K_R >> 4));

    tidx.barrier.wait();

    if (gidx == M_block - 1 || gidy == N_block - 1) {
      if ((gidx * 32 * 2 + idx * 2 + CinitOffset) < M &&
          (gidy * 32 * 2 + idy * 2 + (0 * 16 * 2 + 0)) < N)
        C[CIndex + CinitOffset + (0 * 2 + 0) * ldc + 0] =
            alpha * rC[0][0] +
            beta * C[CIndex + CinitOffset + (0 * 2 + 0) * ldc + 0];
      if ((gidx * 32 * 2 + idx * 2 + CinitOffset) < M &&
          (gidy * 32 * 2 + idy * 2 + (0 * 16 * 2 + 1)) < N)
        C[CIndex + CinitOffset + (0 * 2 + 1) * ldc + 0] =
            alpha * rC[0][1] +
            beta * C[CIndex + CinitOffset + (0 * 2 + 1) * ldc + 0];
      if ((gidx * 32 * 2 + idx * 2 + CinitOffset) < M &&
          (gidy * 32 * 2 + idy * 2 + (1 * 16 * 2 + 0)) < N)
        C[CIndex + CinitOffset + (16 * 2 + 0) * ldc + 0] =
            alpha * rC[0][2] +
            beta * C[CIndex + CinitOffset + (16 * 2 + 0) * ldc + 0];
      if ((gidx * 32 * 2 + idx * 2 + CinitOffset) < M &&
          (gidy * 32 * 2 + idy * 2 + (1 * 16 * 2 + 1)) < N)
        C[CIndex + CinitOffset + (16 * 2 + 1) * ldc + 0] =
            alpha * rC[0][3] +
            beta * C[CIndex + CinitOffset + (16 * 2 + 1) * ldc + 0];
      if ((gidx * 32 * 2 + idx * 2 + CinitOffset + 1) < M &&
          (gidy * 32 * 2 + idy * 2 + (0 * 16 * 2 + 0)) < N)
        C[CIndex + CinitOffset + (0 * 2 + 0) * ldc + 1] =
            alpha * rC[1][0] +
            beta * C[CIndex + CinitOffset + (0 * 2 + 0) * ldc + 1];
      if ((gidx * 32 * 2 + idx * 2 + CinitOffset + 1) < M &&
          (gidy * 32 * 2 + idy * 2 + (0 * 16 * 2 + 1)) < N)
        C[CIndex + CinitOffset + (0 * 2 + 1) * ldc + 1] =
            alpha * rC[1][1] +
            beta * C[CIndex + CinitOffset + (0 * 2 + 1) * ldc + 1];
      if ((gidx * 32 * 2 + idx * 2 + CinitOffset + 1) < M &&
          (gidy * 32 * 2 + idy * 2 + (1 * 16 * 2 + 0)) < N)
        C[CIndex + CinitOffset + (16 * 2 + 0) * ldc + 1] =
            alpha * rC[1][2] +
            beta * C[CIndex + CinitOffset + (16 * 2 + 0) * ldc + 1];
      if ((gidx * 32 * 2 + idx * 2 + CinitOffset + 1) < M &&
          (gidy * 32 * 2 + idy * 2 + (1 * 16 * 2 + 1)) < N)
        C[CIndex + CinitOffset + (16 * 2 + 1) * ldc + 1] =
            alpha * rC[1][3] +
            beta * C[CIndex + CinitOffset + (16 * 2 + 1) * ldc + 1];
      CinitOffset += 16 * 2;
      if ((gidx * 32 * 2 + idx * 2 + CinitOffset) < M &&
          (gidy * 32 * 2 + idy * 2 + (0 * 16 * 2 + 0)) < N)
        C[CIndex + CinitOffset + (0 * 2 + 0) * ldc + 0] =
            alpha * rC[2][0] +
            beta * C[CIndex + CinitOffset + (0 * 2 + 0) * ldc + 0];
      if ((gidx * 32 * 2 + idx * 2 + CinitOffset) < M &&
          (gidy * 32 * 2 + idy * 2 + (0 * 16 * 2 + 1)) < N)
        C[CIndex + CinitOffset + (0 * 2 + 1) * ldc + 0] =
            alpha * rC[2][1] +
            beta * C[CIndex + CinitOffset + (0 * 2 + 1) * ldc + 0];
      if ((gidx * 32 * 2 + idx * 2 + CinitOffset) < M &&
          (gidy * 32 * 2 + idy * 2 + (1 * 16 * 2 + 0)) < N)
        C[CIndex + CinitOffset + (16 * 2 + 0) * ldc + 0] =
            alpha * rC[2][2] +
            beta * C[CIndex + CinitOffset + (16 * 2 + 0) * ldc + 0];
      if ((gidx * 32 * 2 + idx * 2 + CinitOffset) < M &&
          (gidy * 32 * 2 + idy * 2 + (1 * 16 * 2 + 1)) < N)
        C[CIndex + CinitOffset + (16 * 2 + 1) * ldc + 0] =
            alpha * rC[2][3] +
            beta * C[CIndex + CinitOffset + (16 * 2 + 1) * ldc + 0];
      if ((gidx * 32 * 2 + idx * 2 + CinitOffset + 1) < M &&
          (gidy * 32 * 2 + idy * 2 + (0 * 16 * 2 + 0)) < N)
        C[CIndex + CinitOffset + (0 * 2 + 0) * ldc + 1] =
            alpha * rC[3][0] +
            beta * C[CIndex + CinitOffset + (0 * 2 + 0) * ldc + 1];
      if ((gidx * 32 * 2 + idx * 2 + CinitOffset + 1) < M &&
          (gidy * 32 * 2 + idy * 2 + (0 * 16 * 2 + 1)) < N)
        C[CIndex + CinitOffset + (0 * 2 + 1) * ldc + 1] =
            alpha * rC[3][1] +
            beta * C[CIndex + CinitOffset + (0 * 2 + 1) * ldc + 1];
      if ((gidx * 32 * 2 + idx * 2 + CinitOffset + 1) < M &&
          (gidy * 32 * 2 + idy * 2 + (1 * 16 * 2 + 0)) < N)
        C[CIndex + CinitOffset + (16 * 2 + 0) * ldc + 1] =
            alpha * rC[3][2] +
            beta * C[CIndex + CinitOffset + (16 * 2 + 0) * ldc + 1];
      if ((gidx * 32 * 2 + idx * 2 + CinitOffset + 1) < M &&
          (gidy * 32 * 2 + idy * 2 + (1 * 16 * 2 + 1)) < N)
        C[CIndex + CinitOffset + (16 * 2 + 1) * ldc + 1] =
            alpha * rC[3][3] +
            beta * C[CIndex + CinitOffset + (16 * 2 + 1) * ldc + 1];
    } else {
      C[CIndex + CinitOffset + (0 * 2 + 0) * ldc + 0] =
          alpha * rC[0][0] +
          beta * C[CIndex + CinitOffset + (0 * 2 + 0) * ldc + 0];
      C[CIndex + CinitOffset + (0 * 2 + 1) * ldc + 0] =
          alpha * rC[0][1] +
          beta * C[CIndex + CinitOffset + (0 * 2 + 1) * ldc + 0];
      C[CIndex + CinitOffset + (16 * 2 + 0) * ldc + 0] =
          alpha * rC[0][2] +
          beta * C[CIndex + CinitOffset + (16 * 2 + 0) * ldc + 0];
      C[CIndex + CinitOffset + (16 * 2 + 1) * ldc + 0] =
          alpha * rC[0][3] +
          beta * C[CIndex + CinitOffset + (16 * 2 + 1) * ldc + 0];
      C[CIndex + CinitOffset + (0 * 2 + 0) * ldc + 1] =
          alpha * rC[1][0] +
          beta * C[CIndex + CinitOffset + (0 * 2 + 0) * ldc + 1];
      C[CIndex + CinitOffset + (0 * 2 + 1) * ldc + 1] =
          alpha * rC[1][1] +
          beta * C[CIndex + CinitOffset + (0 * 2 + 1) * ldc + 1];
      C[CIndex + CinitOffset + (16 * 2 + 0) * ldc + 1] =
          alpha * rC[1][2] +
          beta * C[CIndex + CinitOffset + (16 * 2 + 0) * ldc + 1];
      C[CIndex + CinitOffset + (16 * 2 + 1) * ldc + 1] =
          alpha * rC[1][3] +
          beta * C[CIndex + CinitOffset + (16 * 2 + 1) * ldc + 1];
      CinitOffset += 16 * 2;
      C[CIndex + CinitOffset + (0 * 2 + 0) * ldc + 0] =
          alpha * rC[2][0] +
          beta * C[CIndex + CinitOffset + (0 * 2 + 0) * ldc + 0];
      C[CIndex + CinitOffset + (0 * 2 + 1) * ldc + 0] =
          alpha * rC[2][1] +
          beta * C[CIndex + CinitOffset + (0 * 2 + 1) * ldc + 0];
      C[CIndex + CinitOffset + (16 * 2 + 0) * ldc + 0] =
          alpha * rC[2][2] +
          beta * C[CIndex + CinitOffset + (16 * 2 + 0) * ldc + 0];
      C[CIndex + CinitOffset + (16 * 2 + 1) * ldc + 0] =
          alpha * rC[2][3] +
          beta * C[CIndex + CinitOffset + (16 * 2 + 1) * ldc + 0];
      C[CIndex + CinitOffset + (0 * 2 + 0) * ldc + 1] =
          alpha * rC[3][0] +
          beta * C[CIndex + CinitOffset + (0 * 2 + 0) * ldc + 1];
      C[CIndex + CinitOffset + (0 * 2 + 1) * ldc + 1] =
          alpha * rC[3][1] +
          beta * C[CIndex + CinitOffset + (0 * 2 + 1) * ldc + 1];
      C[CIndex + CinitOffset + (16 * 2 + 0) * ldc + 1] =
          alpha * rC[3][2] +
          beta * C[CIndex + CinitOffset + (16 * 2 + 0) * ldc + 1];
      C[CIndex + CinitOffset + (16 * 2 + 1) * ldc + 1] =
          alpha * rC[3][3] +
          beta * C[CIndex + CinitOffset + (16 * 2 + 1) * ldc + 1];
    }
  }) ;
  return HCBLAS_SUCCEEDS;
}

hcblasStatus gemm_NoTransA_MICRO_NBK_Mini_Batch_M_N_K_TS16XMTS4_MB2(
    hc::accelerator_view accl_view, const float *A, __int64_t aOffset,
    const float *B, __int64_t bOffset, float *C, __int64_t cOffset, int M,
    int N, int K, int lda, int ldb, int ldc, float alpha, float beta) {
  int M_ = (M - 1) / 8 + 1;
  int N_ = (N - 1) / 8 + 1;
  int N_R = (N_ + 15) & ~15;
  int M_R = (M_ + 15) & ~15;
  int K_R = (K + 15) & ~15;
  hc::extent<2> grdExt(N_R, M_R);
  hc::tiled_extent<2> t_ext = grdExt.tile(16, 16);
  hc::parallel_for_each(accl_view, t_ext, [=](hc::tiled_index<2> tidx)[[hc]] {
    float rC[8][8] = {{static_cast<float>(0)}};
    float rA[1][8];
    float rB[1][8];
    tile_static float lA[16 * 64 * 2 + 16];
    tile_static float lB[16 * 64 * 2 + 16];
    int gidx = tidx.tile[1];
    int gidy = tidx.tile[0];
    int idx = tidx.local[1];
    int idy = tidx.local[0];
    int block_k = 0;
    int alIndex = (idy * (64 * 2 + 1)) + idx * 2;
    int blIndex = (idy * (64 * 2 + 1)) + idx * 2;
    int AIndex = aOffset + (gidx * 64 * 2) + idx * 2 + (idy * lda);
    int BIndex = bOffset + (gidy * 64 * 2) + idx * 2 + (idy * ldb);
    __int64_t CIndex = cOffset + (gidx * 64 * 2) + idx * 2 +
                       (((gidy * 64 * 2) + idy * 2) * ldc);
    __int64_t AinitOffset = 0;
    __int64_t BinitOffset = 0;
    __int64_t CinitOffset = 0;
    int N_block = N_R >> 4;
    int M_block = M_R >> 4;
    int K_block = K_R >> 4;
    do {
      tidx.barrier.wait();

      if (gidx == M_block - 1 || gidy == N_block - 1 ||
          block_k == K_block - 1) {
        for (int sec = 0; sec < 4; ++sec) {
          int secVal = sec << 4;

          if ((gidy * 64 * 2 + idx * 2 + secVal * 2 + 0) < N &&
              (block_k * 16 + idy) < K) {
            lB[blIndex + secVal * 2 + 0] =
                B[BIndex + BinitOffset + secVal * 2 + 0];
          } else {
            lB[blIndex + secVal * 2 + 0] = 0;
          }
          if ((gidy * 64 * 2 + idx * 2 + secVal * 2 + 1) < N &&
              (block_k * 16 + idy) < K) {
            lB[blIndex + secVal * 2 + 1] =
                B[BIndex + BinitOffset + secVal * 2 + 1];
          } else {
            lB[blIndex + secVal * 2 + 1] = 0;
          }

          if ((gidx * 64 * 2 + idx * 2 + secVal * 2 + 0) < M &&
              (block_k * 16 + idy) < K) {
            lA[alIndex + secVal * 2 + 0] =
                A[AIndex + secVal * 2 + 0 + AinitOffset];
          } else {
            lA[alIndex + secVal * 2 + 0] = 0;
          }
          if ((gidx * 64 * 2 + idx * 2 + secVal * 2 + 1) < M &&
              (block_k * 16 + idy) < K) {
            lA[alIndex + secVal * 2 + 1] =
                A[AIndex + secVal * 2 + 1 + AinitOffset];
          } else {
            lA[alIndex + secVal * 2 + 1] = 0;
          }
        }
      } else {
        lB[blIndex + 0 * 2 + 0] = B[BIndex + BinitOffset + 0 * 2 + 0];
        lB[blIndex + 0 * 2 + 1] = B[BIndex + BinitOffset + 0 * 2 + 1];
        lB[blIndex + 16 * 2 + 0] = B[BIndex + BinitOffset + 16 * 2 + 0];
        lB[blIndex + 16 * 2 + 1] = B[BIndex + BinitOffset + 16 * 2 + 1];
        lB[blIndex + 32 * 2 + 0] = B[BIndex + BinitOffset + 32 * 2 + 0];
        lB[blIndex + 32 * 2 + 1] = B[BIndex + BinitOffset + 32 * 2 + 1];
        lB[blIndex + 48 * 2 + 0] = B[BIndex + BinitOffset + 48 * 2 + 0];
        lB[blIndex + 48 * 2 + 1] = B[BIndex + BinitOffset + 48 * 2 + 1];
        lA[alIndex + 0 * 2 + 0] = A[AIndex + 0 * 2 + 0 + AinitOffset];
        lA[alIndex + 0 * 2 + 1] = A[AIndex + 0 * 2 + 1 + AinitOffset];
        lA[alIndex + 16 * 2 + 0] = A[AIndex + 16 * 2 + 0 + AinitOffset];
        lA[alIndex + 16 * 2 + 1] = A[AIndex + 16 * 2 + 1 + AinitOffset];
        lA[alIndex + 32 * 2 + 0] = A[AIndex + 32 * 2 + 0 + AinitOffset];
        lA[alIndex + 32 * 2 + 1] = A[AIndex + 32 * 2 + 1 + AinitOffset];
        lA[alIndex + 48 * 2 + 0] = A[AIndex + 48 * 2 + 0 + AinitOffset];
        lA[alIndex + 48 * 2 + 1] = A[AIndex + 48 * 2 + 1 + AinitOffset];
      }

      tidx.barrier.wait();

      int offA = idx * 2;
      int offB = idy * 2;

      for (int iter = 0; iter < 16; iter += 1) {
        M4x4_MB;
      }

      AinitOffset += lda << 4;
      BinitOffset += ldb << 4;
    } while (++block_k < (K_R >> 4));

    if (gidx == M_block - 1 || gidy == N_block - 1) {
      if ((gidx * 64 * 2 + idx * 2 + CinitOffset) < M &&
          (gidy * 64 * 2 + idy * 2 + (0 * 16 * 2 + 0)) < N)
        C[CIndex + CinitOffset + (0 * 2 + 0) * ldc + 0] =
            alpha * rC[0][0] +
            beta * C[CIndex + CinitOffset + (0 * 2 + 0) * ldc + 0];
      if ((gidx * 64 * 2 + idx * 2 + CinitOffset) < M &&
          (gidy * 64 * 2 + idy * 2 + (0 * 16 * 2 + 1)) < N)
        C[CIndex + CinitOffset + (0 * 2 + 1) * ldc + 0] =
            alpha * rC[0][1] +
            beta * C[CIndex + CinitOffset + (0 * 2 + 1) * ldc + 0];
      if ((gidx * 64 * 2 + idx * 2 + CinitOffset) < M &&
          (gidy * 64 * 2 + idy * 2 + (1 * 16 * 2 + 0)) < N)
        C[CIndex + CinitOffset + (16 * 2 + 0) * ldc + 0] =
            alpha * rC[0][2] +
            beta * C[CIndex + CinitOffset + (16 * 2 + 0) * ldc + 0];
      if ((gidx * 64 * 2 + idx * 2 + CinitOffset) < M &&
          (gidy * 64 * 2 + idy * 2 + (1 * 16 * 2 + 1)) < N)
        C[CIndex + CinitOffset + (16 * 2 + 1) * ldc + 0] =
            alpha * rC[0][3] +
            beta * C[CIndex + CinitOffset + (16 * 2 + 1) * ldc + 0];
      if ((gidx * 64 * 2 + idx * 2 + CinitOffset) < M &&
          (gidy * 64 * 2 + idy * 2 + (2 * 16 * 2 + 0)) < N)
        C[CIndex + CinitOffset + (32 * 2 + 0) * ldc + 0] =
            alpha * rC[0][4] +
            beta * C[CIndex + CinitOffset + (32 * 2 + 0) * ldc + 0];
      if ((gidx * 64 * 2 + idx * 2 + CinitOffset) < M &&
          (gidy * 64 * 2 + idy * 2 + (2 * 16 * 2 + 1)) < N)
        C[CIndex + CinitOffset + (32 * 2 + 1) * ldc + 0] =
            alpha * rC[0][5] +
            beta * C[CIndex + CinitOffset + (32 * 2 + 1) * ldc + 0];
      if ((gidx * 64 * 2 + idx * 2 + CinitOffset) < M &&
          (gidy * 64 * 2 + idy * 2 + (3 * 16 * 2 + 0)) < N)
        C[CIndex + CinitOffset + (48 * 2 + 0) * ldc + 0] =
            alpha * rC[0][6] +
            beta * C[CIndex + CinitOffset + (48 * 2 + 0) * ldc + 0];
      if ((gidx * 64 * 2 + idx * 2 + CinitOffset) < M &&
          (gidy * 64 * 2 + idy * 2 + (3 * 16 * 2 + 1)) < N)
        C[CIndex + CinitOffset + (48 * 2 + 1) * ldc + 0] =
            alpha * rC[0][7] +
            beta * C[CIndex + CinitOffset + (48 * 2 + 1) * ldc + 0];
      if ((gidx * 64 * 2 + idx * 2 + CinitOffset + 1) < M &&
          (gidy * 64 * 2 + idy * 2 + (0 * 16 * 2 + 0)) < N)
        C[CIndex + CinitOffset + (0 * 2 + 0) * ldc + 1] =
            alpha * rC[1][0] +
            beta * C[CIndex + CinitOffset + (0 * 2 + 0) * ldc + 1];
      if ((gidx * 64 * 2 + idx * 2 + CinitOffset + 1) < M &&
          (gidy * 64 * 2 + idy * 2 + (0 * 16 * 2 + 1)) < N)
        C[CIndex + CinitOffset + (0 * 2 + 1) * ldc + 1] =
            alpha * rC[1][1] +
            beta * C[CIndex + CinitOffset + (0 * 2 + 1) * ldc + 1];
      if ((gidx * 64 * 2 + idx * 2 + CinitOffset + 1) < M &&
          (gidy * 64 * 2 + idy * 2 + (1 * 16 * 2 + 0)) < N)
        C[CIndex + CinitOffset + (16 * 2 + 0) * ldc + 1] =
            alpha * rC[1][2] +
            beta * C[CIndex + CinitOffset + (16 * 2 + 0) * ldc + 1];
      if ((gidx * 64 * 2 + idx * 2 + CinitOffset + 1) < M &&
          (gidy * 64 * 2 + idy * 2 + (1 * 16 * 2 + 1)) < N)
        C[CIndex + CinitOffset + (16 * 2 + 1) * ldc + 1] =
            alpha * rC[1][3] +
            beta * C[CIndex + CinitOffset + (16 * 2 + 1) * ldc + 1];
      if ((gidx * 64 * 2 + idx * 2 + CinitOffset + 1) < M &&
          (gidy * 64 * 2 + idy * 2 + (2 * 16 * 2 + 0)) < N)
        C[CIndex + CinitOffset + (32 * 2 + 0) * ldc + 1] =
            alpha * rC[1][4] +
            beta * C[CIndex + CinitOffset + (32 * 2 + 0) * ldc + 1];
      if ((gidx * 64 * 2 + idx * 2 + CinitOffset + 1) < M &&
          (gidy * 64 * 2 + idy * 2 + (2 * 16 * 2 + 1)) < N)
        C[CIndex + CinitOffset + (32 * 2 + 1) * ldc + 1] =
            alpha * rC[1][5] +
            beta * C[CIndex + CinitOffset + (32 * 2 + 1) * ldc + 1];
      if ((gidx * 64 * 2 + idx * 2 + CinitOffset + 1) < M &&
          (gidy * 64 * 2 + idy * 2 + (3 * 16 * 2 + 0)) < N)
        C[CIndex + CinitOffset + (48 * 2 + 0) * ldc + 1] =
            alpha * rC[1][6] +
            beta * C[CIndex + CinitOffset + (48 * 2 + 0) * ldc + 1];
      if ((gidx * 64 * 2 + idx * 2 + CinitOffset + 1) < M &&
          (gidy * 64 * 2 + idy * 2 + (3 * 16 * 2 + 1)) < N)
        C[CIndex + CinitOffset + (48 * 2 + 1) * ldc + 1] =
            alpha * rC[1][7] +
            beta * C[CIndex + CinitOffset + (48 * 2 + 1) * ldc + 1];
      CinitOffset += 16 * 2;
      if ((gidx * 64 * 2 + idx * 2 + CinitOffset) < M &&
          (gidy * 64 * 2 + idy * 2 + (0 * 16 * 2 + 0)) < N)
        C[CIndex + CinitOffset + (0 * 2 + 0) * ldc + 0] =
            alpha * rC[2][0] +
            beta * C[CIndex + CinitOffset + (0 * 2 + 0) * ldc + 0];
      if ((gidx * 64 * 2 + idx * 2 + CinitOffset) < M &&
          (gidy * 64 * 2 + idy * 2 + (0 * 16 * 2 + 1)) < N)
        C[CIndex + CinitOffset + (0 * 2 + 1) * ldc + 0] =
            alpha * rC[2][1] +
            beta * C[CIndex + CinitOffset + (0 * 2 + 1) * ldc + 0];
      if ((gidx * 64 * 2 + idx * 2 + CinitOffset) < M &&
          (gidy * 64 * 2 + idy * 2 + (1 * 16 * 2 + 0)) < N)
        C[CIndex + CinitOffset + (16 * 2 + 0) * ldc + 0] =
            alpha * rC[2][2] +
            beta * C[CIndex + CinitOffset + (16 * 2 + 0) * ldc + 0];
      if ((gidx * 64 * 2 + idx * 2 + CinitOffset) < M &&
          (gidy * 64 * 2 + idy * 2 + (1 * 16 * 2 + 1)) < N)
        C[CIndex + CinitOffset + (16 * 2 + 1) * ldc + 0] =
            alpha * rC[2][3] +
            beta * C[CIndex + CinitOffset + (16 * 2 + 1) * ldc + 0];
      if ((gidx * 64 * 2 + idx * 2 + CinitOffset) < M &&
          (gidy * 64 * 2 + idy * 2 + (2 * 16 * 2 + 0)) < N)
        C[CIndex + CinitOffset + (32 * 2 + 0) * ldc + 0] =
            alpha * rC[2][4] +
            beta * C[CIndex + CinitOffset + (32 * 2 + 0) * ldc + 0];
      if ((gidx * 64 * 2 + idx * 2 + CinitOffset) < M &&
          (gidy * 64 * 2 + idy * 2 + (2 * 16 * 2 + 1)) < N)
        C[CIndex + CinitOffset + (32 * 2 + 1) * ldc + 0] =
            alpha * rC[2][5] +
            beta * C[CIndex + CinitOffset + (32 * 2 + 1) * ldc + 0];
      if ((gidx * 64 * 2 + idx * 2 + CinitOffset) < M &&
          (gidy * 64 * 2 + idy * 2 + (3 * 16 * 2 + 0)) < N)
        C[CIndex + CinitOffset + (48 * 2 + 0) * ldc + 0] =
            alpha * rC[2][6] +
            beta * C[CIndex + CinitOffset + (48 * 2 + 0) * ldc + 0];
      if ((gidx * 64 * 2 + idx * 2 + CinitOffset) < M &&
          (gidy * 64 * 2 + idy * 2 + (3 * 16 * 2 + 1)) < N)
        C[CIndex + CinitOffset + (48 * 2 + 1) * ldc + 0] =
            alpha * rC[2][7] +
            beta * C[CIndex + CinitOffset + (48 * 2 + 1) * ldc + 0];
      if ((gidx * 64 * 2 + idx * 2 + CinitOffset + 1) < M &&
          (gidy * 64 * 2 + idy * 2 + (0 * 16 * 2 + 0)) < N)
        C[CIndex + CinitOffset + (0 * 2 + 0) * ldc + 1] =
            alpha * rC[3][0] +
            beta * C[CIndex + CinitOffset + (0 * 2 + 0) * ldc + 1];
      if ((gidx * 64 * 2 + idx * 2 + CinitOffset + 1) < M &&
          (gidy * 64 * 2 + idy * 2 + (0 * 16 * 2 + 1)) < N)
        C[CIndex + CinitOffset + (0 * 2 + 1) * ldc + 1] =
            alpha * rC[3][1] +
            beta * C[CIndex + CinitOffset + (0 * 2 + 1) * ldc + 1];
      if ((gidx * 64 * 2 + idx * 2 + CinitOffset + 1) < M &&
          (gidy * 64 * 2 + idy * 2 + (1 * 16 * 2 + 0)) < N)
        C[CIndex + CinitOffset + (16 * 2 + 0) * ldc + 1] =
            alpha * rC[3][2] +
            beta * C[CIndex + CinitOffset + (16 * 2 + 0) * ldc + 1];
      if ((gidx * 64 * 2 + idx * 2 + CinitOffset + 1) < M &&
          (gidy * 64 * 2 + idy * 2 + (1 * 16 * 2 + 1)) < N)
        C[CIndex + CinitOffset + (16 * 2 + 1) * ldc + 1] =
            alpha * rC[3][3] +
            beta * C[CIndex + CinitOffset + (16 * 2 + 1) * ldc + 1];
      if ((gidx * 64 * 2 + idx * 2 + CinitOffset + 1) < M &&
          (gidy * 64 * 2 + idy * 2 + (2 * 16 * 2 + 0)) < N)
        C[CIndex + CinitOffset + (32 * 2 + 0) * ldc + 1] =
            alpha * rC[3][4] +
            beta * C[CIndex + CinitOffset + (32 * 2 + 0) * ldc + 1];
      if ((gidx * 64 * 2 + idx * 2 + CinitOffset + 1) < M &&
          (gidy * 64 * 2 + idy * 2 + (2 * 16 * 2 + 1)) < N)
        C[CIndex + CinitOffset + (32 * 2 + 1) * ldc + 1] =
            alpha * rC[3][5] +
            beta * C[CIndex + CinitOffset + (32 * 2 + 1) * ldc + 1];
      if ((gidx * 64 * 2 + idx * 2 + CinitOffset + 1) < M &&
          (gidy * 64 * 2 + idy * 2 + (3 * 16 * 2 + 0)) < N)
        C[CIndex + CinitOffset + (48 * 2 + 0) * ldc + 1] =
            alpha * rC[3][6] +
            beta * C[CIndex + CinitOffset + (48 * 2 + 0) * ldc + 1];
      if ((gidx * 64 * 2 + idx * 2 + CinitOffset + 1) < M &&
          (gidy * 64 * 2 + idy * 2 + (3 * 16 * 2 + 1)) < N)
        C[CIndex + CinitOffset + (48 * 2 + 1) * ldc + 1] =
            alpha * rC[3][7] +
            beta * C[CIndex + CinitOffset + (48 * 2 + 1) * ldc + 1];
      CinitOffset += 16 * 2;
      if ((gidx * 64 * 2 + idx * 2 + CinitOffset) < M &&
          (gidy * 64 * 2 + idy * 2 + (0 * 16 * 2 + 0)) < N)
        C[CIndex + CinitOffset + (0 * 2 + 0) * ldc + 0] =
            alpha * rC[4][0] +
            beta * C[CIndex + CinitOffset + (0 * 2 + 0) * ldc + 0];
      if ((gidx * 64 * 2 + idx * 2 + CinitOffset) < M &&
          (gidy * 64 * 2 + idy * 2 + (0 * 16 * 2 + 1)) < N)
        C[CIndex + CinitOffset + (0 * 2 + 1) * ldc + 0] =
            alpha * rC[4][1] +
            beta * C[CIndex + CinitOffset + (0 * 2 + 1) * ldc + 0];
      if ((gidx * 64 * 2 + idx * 2 + CinitOffset) < M &&
          (gidy * 64 * 2 + idy * 2 + (1 * 16 * 2 + 0)) < N)
        C[CIndex + CinitOffset + (16 * 2 + 0) * ldc + 0] =
            alpha * rC[4][2] +
            beta * C[CIndex + CinitOffset + (16 * 2 + 0) * ldc + 0];
      if ((gidx * 64 * 2 + idx * 2 + CinitOffset) < M &&
          (gidy * 64 * 2 + idy * 2 + (1 * 16 * 2 + 1)) < N)
        C[CIndex + CinitOffset + (16 * 2 + 1) * ldc + 0] =
            alpha * rC[4][3] +
            beta * C[CIndex + CinitOffset + (16 * 2 + 1) * ldc + 0];
      if ((gidx * 64 * 2 + idx * 2 + CinitOffset) < M &&
          (gidy * 64 * 2 + idy * 2 + (2 * 16 * 2 + 0)) < N)
        C[CIndex + CinitOffset + (32 * 2 + 0) * ldc + 0] =
            alpha * rC[4][4] +
            beta * C[CIndex + CinitOffset + (32 * 2 + 0) * ldc + 0];
      if ((gidx * 64 * 2 + idx * 2 + CinitOffset) < M &&
          (gidy * 64 * 2 + idy * 2 + (2 * 16 * 2 + 1)) < N)
        C[CIndex + CinitOffset + (32 * 2 + 1) * ldc + 0] =
            alpha * rC[4][5] +
            beta * C[CIndex + CinitOffset + (32 * 2 + 1) * ldc + 0];
      if ((gidx * 64 * 2 + idx * 2 + CinitOffset) < M &&
          (gidy * 64 * 2 + idy * 2 + (3 * 16 * 2 + 0)) < N)
        C[CIndex + CinitOffset + (48 * 2 + 0) * ldc + 0] =
            alpha * rC[4][6] +
            beta * C[CIndex + CinitOffset + (48 * 2 + 0) * ldc + 0];
      if ((gidx * 64 * 2 + idx * 2 + CinitOffset) < M &&
          (gidy * 64 * 2 + idy * 2 + (3 * 16 * 2 + 1)) < N)
        C[CIndex + CinitOffset + (48 * 2 + 1) * ldc + 0] =
            alpha * rC[4][7] +
            beta * C[CIndex + CinitOffset + (48 * 2 + 1) * ldc + 0];
      if ((gidx * 64 * 2 + idx * 2 + CinitOffset + 1) < M &&
          (gidy * 64 * 2 + idy * 2 + (0 * 16 * 2 + 0)) < N)
        C[CIndex + CinitOffset + (0 * 2 + 0) * ldc + 1] =
            alpha * rC[5][0] +
            beta * C[CIndex + CinitOffset + (0 * 2 + 0) * ldc + 1];
      if ((gidx * 64 * 2 + idx * 2 + CinitOffset + 1) < M &&
          (gidy * 64 * 2 + idy * 2 + (0 * 16 * 2 + 1)) < N)
        C[CIndex + CinitOffset + (0 * 2 + 1) * ldc + 1] =
            alpha * rC[5][1] +
            beta * C[CIndex + CinitOffset + (0 * 2 + 1) * ldc + 1];
      if ((gidx * 64 * 2 + idx * 2 + CinitOffset + 1) < M &&
          (gidy * 64 * 2 + idy * 2 + (1 * 16 * 2 + 0)) < N)
        C[CIndex + CinitOffset + (16 * 2 + 0) * ldc + 1] =
            alpha * rC[5][2] +
            beta * C[CIndex + CinitOffset + (16 * 2 + 0) * ldc + 1];
      if ((gidx * 64 * 2 + idx * 2 + CinitOffset + 1) < M &&
          (gidy * 64 * 2 + idy * 2 + (1 * 16 * 2 + 1)) < N)
        C[CIndex + CinitOffset + (16 * 2 + 1) * ldc + 1] =
            alpha * rC[5][3] +
            beta * C[CIndex + CinitOffset + (16 * 2 + 1) * ldc + 1];
      if ((gidx * 64 * 2 + idx * 2 + CinitOffset + 1) < M &&
          (gidy * 64 * 2 + idy * 2 + (2 * 16 * 2 + 0)) < N)
        C[CIndex + CinitOffset + (32 * 2 + 0) * ldc + 1] =
            alpha * rC[5][4] +
            beta * C[CIndex + CinitOffset + (32 * 2 + 0) * ldc + 1];
      if ((gidx * 64 * 2 + idx * 2 + CinitOffset + 1) < M &&
          (gidy * 64 * 2 + idy * 2 + (2 * 16 * 2 + 1)) < N)
        C[CIndex + CinitOffset + (32 * 2 + 1) * ldc + 1] =
            alpha * rC[5][5] +
            beta * C[CIndex + CinitOffset + (32 * 2 + 1) * ldc + 1];
      if ((gidx * 64 * 2 + idx * 2 + CinitOffset + 1) < M &&
          (gidy * 64 * 2 + idy * 2 + (3 * 16 * 2 + 0)) < N)
        C[CIndex + CinitOffset + (48 * 2 + 0) * ldc + 1] =
            alpha * rC[5][6] +
            beta * C[CIndex + CinitOffset + (48 * 2 + 0) * ldc + 1];
      if ((gidx * 64 * 2 + idx * 2 + CinitOffset + 1) < M &&
          (gidy * 64 * 2 + idy * 2 + (3 * 16 * 2 + 1)) < N)
        C[CIndex + CinitOffset + (48 * 2 + 1) * ldc + 1] =
            alpha * rC[5][7] +
            beta * C[CIndex + CinitOffset + (48 * 2 + 1) * ldc + 1];
      CinitOffset += 16 * 2;
      if ((gidx * 64 * 2 + idx * 2 + CinitOffset) < M &&
          (gidy * 64 * 2 + idy * 2 + (0 * 16 * 2 + 0)) < N)
        C[CIndex + CinitOffset + (0 * 2 + 0) * ldc + 0] =
            alpha * rC[6][0] +
            beta * C[CIndex + CinitOffset + (0 * 2 + 0) * ldc + 0];
      if ((gidx * 64 * 2 + idx * 2 + CinitOffset) < M &&
          (gidy * 64 * 2 + idy * 2 + (0 * 16 * 2 + 1)) < N)
        C[CIndex + CinitOffset + (0 * 2 + 1) * ldc + 0] =
            alpha * rC[6][1] +
            beta * C[CIndex + CinitOffset + (0 * 2 + 1) * ldc + 0];
      if ((gidx * 64 * 2 + idx * 2 + CinitOffset) < M &&
          (gidy * 64 * 2 + idy * 2 + (1 * 16 * 2 + 0)) < N)
        C[CIndex + CinitOffset + (16 * 2 + 0) * ldc + 0] =
            alpha * rC[6][2] +
            beta * C[CIndex + CinitOffset + (16 * 2 + 0) * ldc + 0];
      if ((gidx * 64 * 2 + idx * 2 + CinitOffset) < M &&
          (gidy * 64 * 2 + idy * 2 + (1 * 16 * 2 + 1)) < N)
        C[CIndex + CinitOffset + (16 * 2 + 1) * ldc + 0] =
            alpha * rC[6][3] +
            beta * C[CIndex + CinitOffset + (16 * 2 + 1) * ldc + 0];
      if ((gidx * 64 * 2 + idx * 2 + CinitOffset) < M &&
          (gidy * 64 * 2 + idy * 2 + (2 * 16 * 2 + 0)) < N)
        C[CIndex + CinitOffset + (32 * 2 + 0) * ldc + 0] =
            alpha * rC[6][4] +
            beta * C[CIndex + CinitOffset + (32 * 2 + 0) * ldc + 0];
      if ((gidx * 64 * 2 + idx * 2 + CinitOffset) < M &&
          (gidy * 64 * 2 + idy * 2 + (2 * 16 * 2 + 1)) < N)
        C[CIndex + CinitOffset + (32 * 2 + 1) * ldc + 0] =
            alpha * rC[6][5] +
            beta * C[CIndex + CinitOffset + (32 * 2 + 1) * ldc + 0];
      if ((gidx * 64 * 2 + idx * 2 + CinitOffset) < M &&
          (gidy * 64 * 2 + idy * 2 + (3 * 16 * 2 + 0)) < N)
        C[CIndex + CinitOffset + (48 * 2 + 0) * ldc + 0] =
            alpha * rC[6][6] +
            beta * C[CIndex + CinitOffset + (48 * 2 + 0) * ldc + 0];
      if ((gidx * 64 * 2 + idx * 2 + CinitOffset) < M &&
          (gidy * 64 * 2 + idy * 2 + (3 * 16 * 2 + 1)) < N)
        C[CIndex + CinitOffset + (48 * 2 + 1) * ldc + 0] =
            alpha * rC[6][7] +
            beta * C[CIndex + CinitOffset + (48 * 2 + 1) * ldc + 0];
      if ((gidx * 64 * 2 + idx * 2 + CinitOffset + 1) < M &&
          (gidy * 64 * 2 + idy * 2 + (0 * 16 * 2 + 0)) < N)
        C[CIndex + CinitOffset + (0 * 2 + 0) * ldc + 1] =
            alpha * rC[7][0] +
            beta * C[CIndex + CinitOffset + (0 * 2 + 0) * ldc + 1];
      if ((gidx * 64 * 2 + idx * 2 + CinitOffset + 1) < M &&
          (gidy * 64 * 2 + idy * 2 + (0 * 16 * 2 + 1)) < N)
        C[CIndex + CinitOffset + (0 * 2 + 1) * ldc + 1] =
            alpha * rC[7][1] +
            beta * C[CIndex + CinitOffset + (0 * 2 + 1) * ldc + 1];
      if ((gidx * 64 * 2 + idx * 2 + CinitOffset + 1) < M &&
          (gidy * 64 * 2 + idy * 2 + (1 * 16 * 2 + 0)) < N)
        C[CIndex + CinitOffset + (16 * 2 + 0) * ldc + 1] =
            alpha * rC[7][2] +
            beta * C[CIndex + CinitOffset + (16 * 2 + 0) * ldc + 1];
      if ((gidx * 64 * 2 + idx * 2 + CinitOffset + 1) < M &&
          (gidy * 64 * 2 + idy * 2 + (1 * 16 * 2 + 1)) < N)
        C[CIndex + CinitOffset + (16 * 2 + 1) * ldc + 1] =
            alpha * rC[7][3] +
            beta * C[CIndex + CinitOffset + (16 * 2 + 1) * ldc + 1];
      if ((gidx * 64 * 2 + idx * 2 + CinitOffset + 1) < M &&
          (gidy * 64 * 2 + idy * 2 + (2 * 16 * 2 + 0)) < N)
        C[CIndex + CinitOffset + (32 * 2 + 0) * ldc + 1] =
            alpha * rC[7][4] +
            beta * C[CIndex + CinitOffset + (32 * 2 + 0) * ldc + 1];
      if ((gidx * 64 * 2 + idx * 2 + CinitOffset + 1) < M &&
          (gidy * 64 * 2 + idy * 2 + (2 * 16 * 2 + 1)) < N)
        C[CIndex + CinitOffset + (32 * 2 + 1) * ldc + 1] =
            alpha * rC[7][5] +
            beta * C[CIndex + CinitOffset + (32 * 2 + 1) * ldc + 1];
      if ((gidx * 64 * 2 + idx * 2 + CinitOffset + 1) < M &&
          (gidy * 64 * 2 + idy * 2 + (3 * 16 * 2 + 0)) < N)
        C[CIndex + CinitOffset + (48 * 2 + 0) * ldc + 1] =
            alpha * rC[7][6] +
            beta * C[CIndex + CinitOffset + (48 * 2 + 0) * ldc + 1];
      if ((gidx * 64 * 2 + idx * 2 + CinitOffset + 1) < M &&
          (gidy * 64 * 2 + idy * 2 + (3 * 16 * 2 + 1)) < N)
        C[CIndex + CinitOffset + (48 * 2 + 1) * ldc + 1] =
            alpha * rC[7][7] +
            beta * C[CIndex + CinitOffset + (48 * 2 + 1) * ldc + 1];
    } else {
      C[CIndex + CinitOffset + (0 * 2 + 0) * ldc + 0] =
          alpha * rC[0][0] +
          beta * C[CIndex + CinitOffset + (0 * 2 + 0) * ldc + 0];
      C[CIndex + CinitOffset + (0 * 2 + 1) * ldc + 0] =
          alpha * rC[0][1] +
          beta * C[CIndex + CinitOffset + (0 * 2 + 1) * ldc + 0];
      C[CIndex + CinitOffset + (16 * 2 + 0) * ldc + 0] =
          alpha * rC[0][2] +
          beta * C[CIndex + CinitOffset + (16 * 2 + 0) * ldc + 0];
      C[CIndex + CinitOffset + (16 * 2 + 1) * ldc + 0] =
          alpha * rC[0][3] +
          beta * C[CIndex + CinitOffset + (16 * 2 + 1) * ldc + 0];
      C[CIndex + CinitOffset + (32 * 2 + 0) * ldc + 0] =
          alpha * rC[0][4] +
          beta * C[CIndex + CinitOffset + (32 * 2 + 0) * ldc + 0];
      C[CIndex + CinitOffset + (32 * 2 + 1) * ldc + 0] =
          alpha * rC[0][5] +
          beta * C[CIndex + CinitOffset + (32 * 2 + 1) * ldc + 0];
      C[CIndex + CinitOffset + (48 * 2 + 0) * ldc + 0] =
          alpha * rC[0][6] +
          beta * C[CIndex + CinitOffset + (48 * 2 + 0) * ldc + 0];
      C[CIndex + CinitOffset + (48 * 2 + 1) * ldc + 0] =
          alpha * rC[0][7] +
          beta * C[CIndex + CinitOffset + (48 * 2 + 1) * ldc + 0];
      C[CIndex + CinitOffset + (0 * 2 + 0) * ldc + 1] =
          alpha * rC[1][0] +
          beta * C[CIndex + CinitOffset + (0 * 2 + 0) * ldc + 1];
      C[CIndex + CinitOffset + (0 * 2 + 1) * ldc + 1] =
          alpha * rC[1][1] +
          beta * C[CIndex + CinitOffset + (0 * 2 + 1) * ldc + 1];
      C[CIndex + CinitOffset + (16 * 2 + 0) * ldc + 1] =
          alpha * rC[1][2] +
          beta * C[CIndex + CinitOffset + (16 * 2 + 0) * ldc + 1];
      C[CIndex + CinitOffset + (16 * 2 + 1) * ldc + 1] =
          alpha * rC[1][3] +
          beta * C[CIndex + CinitOffset + (16 * 2 + 1) * ldc + 1];
      C[CIndex + CinitOffset + (32 * 2 + 0) * ldc + 1] =
          alpha * rC[1][4] +
          beta * C[CIndex + CinitOffset + (32 * 2 + 0) * ldc + 1];
      C[CIndex + CinitOffset + (32 * 2 + 1) * ldc + 1] =
          alpha * rC[1][5] +
          beta * C[CIndex + CinitOffset + (32 * 2 + 1) * ldc + 1];
      C[CIndex + CinitOffset + (48 * 2 + 0) * ldc + 1] =
          alpha * rC[1][6] +
          beta * C[CIndex + CinitOffset + (48 * 2 + 0) * ldc + 1];
      C[CIndex + CinitOffset + (48 * 2 + 1) * ldc + 1] =
          alpha * rC[1][7] +
          beta * C[CIndex + CinitOffset + (48 * 2 + 1) * ldc + 1];
      CinitOffset += 16 * 2;
      C[CIndex + CinitOffset + (0 * 2 + 0) * ldc + 0] =
          alpha * rC[2][0] +
          beta * C[CIndex + CinitOffset + (0 * 2 + 0) * ldc + 0];
      C[CIndex + CinitOffset + (0 * 2 + 1) * ldc + 0] =
          alpha * rC[2][1] +
          beta * C[CIndex + CinitOffset + (0 * 2 + 1) * ldc + 0];
      C[CIndex + CinitOffset + (16 * 2 + 0) * ldc + 0] =
          alpha * rC[2][2] +
          beta * C[CIndex + CinitOffset + (16 * 2 + 0) * ldc + 0];
      C[CIndex + CinitOffset + (16 * 2 + 1) * ldc + 0] =
          alpha * rC[2][3] +
          beta * C[CIndex + CinitOffset + (16 * 2 + 1) * ldc + 0];
      C[CIndex + CinitOffset + (32 * 2 + 0) * ldc + 0] =
          alpha * rC[2][4] +
          beta * C[CIndex + CinitOffset + (32 * 2 + 0) * ldc + 0];
      C[CIndex + CinitOffset + (32 * 2 + 1) * ldc + 0] =
          alpha * rC[2][5] +
          beta * C[CIndex + CinitOffset + (32 * 2 + 1) * ldc + 0];
      C[CIndex + CinitOffset + (48 * 2 + 0) * ldc + 0] =
          alpha * rC[2][6] +
          beta * C[CIndex + CinitOffset + (48 * 2 + 0) * ldc + 0];
      C[CIndex + CinitOffset + (48 * 2 + 1) * ldc + 0] =
          alpha * rC[2][7] +
          beta * C[CIndex + CinitOffset + (48 * 2 + 1) * ldc + 0];
      C[CIndex + CinitOffset + (0 * 2 + 0) * ldc + 1] =
          alpha * rC[3][0] +
          beta * C[CIndex + CinitOffset + (0 * 2 + 0) * ldc + 1];
      C[CIndex + CinitOffset + (0 * 2 + 1) * ldc + 1] =
          alpha * rC[3][1] +
          beta * C[CIndex + CinitOffset + (0 * 2 + 1) * ldc + 1];
      C[CIndex + CinitOffset + (16 * 2 + 0) * ldc + 1] =
          alpha * rC[3][2] +
          beta * C[CIndex + CinitOffset + (16 * 2 + 0) * ldc + 1];
      C[CIndex + CinitOffset + (16 * 2 + 1) * ldc + 1] =
          alpha * rC[3][3] +
          beta * C[CIndex + CinitOffset + (16 * 2 + 1) * ldc + 1];
      C[CIndex + CinitOffset + (32 * 2 + 0) * ldc + 1] =
          alpha * rC[3][4] +
          beta * C[CIndex + CinitOffset + (32 * 2 + 0) * ldc + 1];
      C[CIndex + CinitOffset + (32 * 2 + 1) * ldc + 1] =
          alpha * rC[3][5] +
          beta * C[CIndex + CinitOffset + (32 * 2 + 1) * ldc + 1];
      C[CIndex + CinitOffset + (48 * 2 + 0) * ldc + 1] =
          alpha * rC[3][6] +
          beta * C[CIndex + CinitOffset + (48 * 2 + 0) * ldc + 1];
      C[CIndex + CinitOffset + (48 * 2 + 1) * ldc + 1] =
          alpha * rC[3][7] +
          beta * C[CIndex + CinitOffset + (48 * 2 + 1) * ldc + 1];
      CinitOffset += 16 * 2;
      C[CIndex + CinitOffset + (0 * 2 + 0) * ldc + 0] =
          alpha * rC[4][0] +
          beta * C[CIndex + CinitOffset + (0 * 2 + 0) * ldc + 0];
      C[CIndex + CinitOffset + (0 * 2 + 1) * ldc + 0] =
          alpha * rC[4][1] +
          beta * C[CIndex + CinitOffset + (0 * 2 + 1) * ldc + 0];
      C[CIndex + CinitOffset + (16 * 2 + 0) * ldc + 0] =
          alpha * rC[4][2] +
          beta * C[CIndex + CinitOffset + (16 * 2 + 0) * ldc + 0];
      C[CIndex + CinitOffset + (16 * 2 + 1) * ldc + 0] =
          alpha * rC[4][3] +
          beta * C[CIndex + CinitOffset + (16 * 2 + 1) * ldc + 0];
      C[CIndex + CinitOffset + (32 * 2 + 0) * ldc + 0] =
          alpha * rC[4][4] +
          beta * C[CIndex + CinitOffset + (32 * 2 + 0) * ldc + 0];
      C[CIndex + CinitOffset + (32 * 2 + 1) * ldc + 0] =
          alpha * rC[4][5] +
          beta * C[CIndex + CinitOffset + (32 * 2 + 1) * ldc + 0];
      C[CIndex + CinitOffset + (48 * 2 + 0) * ldc + 0] =
          alpha * rC[4][6] +
          beta * C[CIndex + CinitOffset + (48 * 2 + 0) * ldc + 0];
      C[CIndex + CinitOffset + (48 * 2 + 1) * ldc + 0] =
          alpha * rC[4][7] +
          beta * C[CIndex + CinitOffset + (48 * 2 + 1) * ldc + 0];
      C[CIndex + CinitOffset + (0 * 2 + 0) * ldc + 1] =
          alpha * rC[5][0] +
          beta * C[CIndex + CinitOffset + (0 * 2 + 0) * ldc + 1];
      C[CIndex + CinitOffset + (0 * 2 + 1) * ldc + 1] =
          alpha * rC[5][1] +
          beta * C[CIndex + CinitOffset + (0 * 2 + 1) * ldc + 1];
      C[CIndex + CinitOffset + (16 * 2 + 0) * ldc + 1] =
          alpha * rC[5][2] +
          beta * C[CIndex + CinitOffset + (16 * 2 + 0) * ldc + 1];
      C[CIndex + CinitOffset + (16 * 2 + 1) * ldc + 1] =
          alpha * rC[5][3] +
          beta * C[CIndex + CinitOffset + (16 * 2 + 1) * ldc + 1];
      C[CIndex + CinitOffset + (32 * 2 + 0) * ldc + 1] =
          alpha * rC[5][4] +
          beta * C[CIndex + CinitOffset + (32 * 2 + 0) * ldc + 1];
      C[CIndex + CinitOffset + (32 * 2 + 1) * ldc + 1] =
          alpha * rC[5][5] +
          beta * C[CIndex + CinitOffset + (32 * 2 + 1) * ldc + 1];
      C[CIndex + CinitOffset + (48 * 2 + 0) * ldc + 1] =
          alpha * rC[5][6] +
          beta * C[CIndex + CinitOffset + (48 * 2 + 0) * ldc + 1];
      C[CIndex + CinitOffset + (48 * 2 + 1) * ldc + 1] =
          alpha * rC[5][7] +
          beta * C[CIndex + CinitOffset + (48 * 2 + 1) * ldc + 1];
      CinitOffset += 16 * 2;
      C[CIndex + CinitOffset + (0 * 2 + 0) * ldc + 0] =
          alpha * rC[6][0] +
          beta * C[CIndex + CinitOffset + (0 * 2 + 0) * ldc + 0];
      C[CIndex + CinitOffset + (0 * 2 + 1) * ldc + 0] =
          alpha * rC[6][1] +
          beta * C[CIndex + CinitOffset + (0 * 2 + 1) * ldc + 0];
      C[CIndex + CinitOffset + (16 * 2 + 0) * ldc + 0] =
          alpha * rC[6][2] +
          beta * C[CIndex + CinitOffset + (16 * 2 + 0) * ldc + 0];
      C[CIndex + CinitOffset + (16 * 2 + 1) * ldc + 0] =
          alpha * rC[6][3] +
          beta * C[CIndex + CinitOffset + (16 * 2 + 1) * ldc + 0];
      C[CIndex + CinitOffset + (32 * 2 + 0) * ldc + 0] =
          alpha * rC[6][4] +
          beta * C[CIndex + CinitOffset + (32 * 2 + 0) * ldc + 0];
      C[CIndex + CinitOffset + (32 * 2 + 1) * ldc + 0] =
          alpha * rC[6][5] +
          beta * C[CIndex + CinitOffset + (32 * 2 + 1) * ldc + 0];
      C[CIndex + CinitOffset + (48 * 2 + 0) * ldc + 0] =
          alpha * rC[6][6] +
          beta * C[CIndex + CinitOffset + (48 * 2 + 0) * ldc + 0];
      C[CIndex + CinitOffset + (48 * 2 + 1) * ldc + 0] =
          alpha * rC[6][7] +
          beta * C[CIndex + CinitOffset + (48 * 2 + 1) * ldc + 0];
      C[CIndex + CinitOffset + (0 * 2 + 0) * ldc + 1] =
          alpha * rC[7][0] +
          beta * C[CIndex + CinitOffset + (0 * 2 + 0) * ldc + 1];
      C[CIndex + CinitOffset + (0 * 2 + 1) * ldc + 1] =
          alpha * rC[7][1] +
          beta * C[CIndex + CinitOffset + (0 * 2 + 1) * ldc + 1];
      C[CIndex + CinitOffset + (16 * 2 + 0) * ldc + 1] =
          alpha * rC[7][2] +
          beta * C[CIndex + CinitOffset + (16 * 2 + 0) * ldc + 1];
      C[CIndex + CinitOffset + (16 * 2 + 1) * ldc + 1] =
          alpha * rC[7][3] +
          beta * C[CIndex + CinitOffset + (16 * 2 + 1) * ldc + 1];
      C[CIndex + CinitOffset + (32 * 2 + 0) * ldc + 1] =
          alpha * rC[7][4] +
          beta * C[CIndex + CinitOffset + (32 * 2 + 0) * ldc + 1];
      C[CIndex + CinitOffset + (32 * 2 + 1) * ldc + 1] =
          alpha * rC[7][5] +
          beta * C[CIndex + CinitOffset + (32 * 2 + 1) * ldc + 1];
      C[CIndex + CinitOffset + (48 * 2 + 0) * ldc + 1] =
          alpha * rC[7][6] +
          beta * C[CIndex + CinitOffset + (48 * 2 + 0) * ldc + 1];
      C[CIndex + CinitOffset + (48 * 2 + 1) * ldc + 1] =
          alpha * rC[7][7] +
          beta * C[CIndex + CinitOffset + (48 * 2 + 1) * ldc + 1];
    }
  }) ;
  return HCBLAS_SUCCEEDS;
}

hcblasStatus gemm_NoTransA_MICRO_NBK_M064_N064_K064_TS16XMTS4(
    hc::accelerator_view accl_view, const float *A, __int64_t aOffset,
    const float *B, __int64_t bOffset, float *C, __int64_t cOffset, int M,
    int N, int K, int lda, int ldb, int ldc, float alpha, float beta) {

#undef TILESIZE
#undef MICROTILESIZE
#define TILESIZE 16
#define MICROTILESIZE 4
  int M_ = M >> 2;
  int N_ = N >> 2;
  int N_R = (N_ + (TILESIZE - 1)) & ~(TILESIZE - 1);
  int M_R = (M_ + (TILESIZE - 1)) & ~(TILESIZE - 1);
  int K_R = (K + (TILESIZE - 1)) & ~(TILESIZE - 1);
  hc::extent<2> grdExt(N_R, M_R);
  hc::tiled_extent<2> t_ext = grdExt.tile(TILESIZE, TILESIZE);
  hc::parallel_for_each(accl_view, t_ext, [=](hc::tiled_index<2> tidx)[[hc]] {
    int shiftTS = hc::fast_math::log2f(TILESIZE);
    float rC[MICROTILESIZE][MICROTILESIZE] = {{static_cast<float>(0)}};
    float rA[1][MICROTILESIZE];
    float rB[1][MICROTILESIZE];
    tile_static float lA[TOTMICROTILEPROD + TILESIZE];
    tile_static float lB[TOTMICROTILEPROD + TILESIZE];
    int gidx = tidx.tile[1];
    int gidy = tidx.tile[0];
    int idx = tidx.local[1];
    int idy = tidx.local[0];
    int block_k = 0;
    int lIndex = (idy * BANKMICROTILESIZE) + idx;
    int AIndex = aOffset + (gidx * MICROTILEPROD) + idx + (idy * lda);
    int BIndex = bOffset + (gidy * MICROTILEPROD) + idx + (idy * ldb);
    __int64_t CIndex = cOffset + (gidx * MICROTILEPROD) + idx +
                       (((gidy * MICROTILEPROD) + idy) * ldc);
    __int64_t CinitOffset = 0;

    do {
      tidx.barrier.wait();

      lB[lIndex + 0 * TILESIZE] =
          B[BIndex + block_k * TILESIZE * ldb + 0 * TILESIZE];
      lB[lIndex + 1 * TILESIZE] =
          B[BIndex + block_k * TILESIZE * ldb + 1 * TILESIZE];
      lB[lIndex + 2 * TILESIZE] =
          B[BIndex + block_k * TILESIZE * ldb + 2 * TILESIZE];
      lB[lIndex + 3 * TILESIZE] =
          B[BIndex + block_k * TILESIZE * ldb + 3 * TILESIZE];
      lA[lIndex + 0 * TILESIZE] =
          A[AIndex + block_k * TILESIZE * lda + 0 * TILESIZE];
      lA[lIndex + 1 * TILESIZE] =
          A[AIndex + block_k * TILESIZE * lda + 1 * TILESIZE];
      lA[lIndex + 2 * TILESIZE] =
          A[AIndex + block_k * TILESIZE * lda + 2 * TILESIZE];
      lA[lIndex + 3 * TILESIZE] =
          A[AIndex + block_k * TILESIZE * lda + 3 * TILESIZE];

      tidx.barrier.wait();
      int offA = idx;
      int offB = idy;

      for (int iter = 0; iter < TILESIZE; iter += 8) {
        M4x4 M4x4 M4x4 M4x4 M4x4 M4x4 M4x4 M4x4
      }

      tidx.barrier.wait();
    } while (++block_k < (K_R / TILESIZE));

    C[CIndex + CinitOffset + 0 * ldc] =
        alpha * rC[0][0] + beta * C[CIndex + CinitOffset + 0 * ldc];
    C[CIndex + CinitOffset + 1 * TILESIZE * ldc] =
        alpha * rC[0][1] + beta * C[CIndex + CinitOffset + 1 * TILESIZE * ldc];
    C[CIndex + CinitOffset + 2 * TILESIZE * ldc] =
        alpha * rC[0][2] + beta * C[CIndex + CinitOffset + 2 * TILESIZE * ldc];
    C[CIndex + CinitOffset + 3 * TILESIZE * ldc] =
        alpha * rC[0][3] + beta * C[CIndex + CinitOffset + 3 * TILESIZE * ldc];
    CinitOffset += TILESIZE;
    C[CIndex + CinitOffset + 0 * ldc] =
        alpha * rC[1][0] + beta * C[CIndex + CinitOffset + 0 * ldc];
    C[CIndex + CinitOffset + 1 * TILESIZE * ldc] =
        alpha * rC[1][1] + beta * C[CIndex + CinitOffset + 1 * TILESIZE * ldc];
    C[CIndex + CinitOffset + 2 * TILESIZE * ldc] =
        alpha * rC[1][2] + beta * C[CIndex + CinitOffset + 2 * TILESIZE * ldc];
    C[CIndex + CinitOffset + 3 * TILESIZE * ldc] =
        alpha * rC[1][3] + beta * C[CIndex + CinitOffset + 3 * TILESIZE * ldc];
    CinitOffset += TILESIZE;
    C[CIndex + CinitOffset + 0 * ldc] =
        alpha * rC[2][0] + beta * C[CIndex + CinitOffset + 0 * ldc];
    C[CIndex + CinitOffset + 1 * TILESIZE * ldc] =
        alpha * rC[2][1] + beta * C[CIndex + CinitOffset + 1 * TILESIZE * ldc];
    C[CIndex + CinitOffset + 2 * TILESIZE * ldc] =
        alpha * rC[2][2] + beta * C[CIndex + CinitOffset + 2 * TILESIZE * ldc];
    C[CIndex + CinitOffset + 3 * TILESIZE * ldc] =
        alpha * rC[2][3] + beta * C[CIndex + CinitOffset + 3 * TILESIZE * ldc];
    CinitOffset += TILESIZE;
    C[CIndex + CinitOffset + 0 * ldc] =
        alpha * rC[3][0] + beta * C[CIndex + CinitOffset + 0 * ldc];
    C[CIndex + CinitOffset + 1 * TILESIZE * ldc] =
        alpha * rC[3][1] + beta * C[CIndex + CinitOffset + 1 * TILESIZE * ldc];
    C[CIndex + CinitOffset + 2 * TILESIZE * ldc] =
        alpha * rC[3][2] + beta * C[CIndex + CinitOffset + 2 * TILESIZE * ldc];
    C[CIndex + CinitOffset + 3 * TILESIZE * ldc] =
        alpha * rC[3][3] + beta * C[CIndex + CinitOffset + 3 * TILESIZE * ldc];
  }) ;
#undef TILESIZE
#undef MICROTILESIZE
  return HCBLAS_SUCCEEDS;
}

hcblasStatus gemm_NoTransA_MICRO_NBK_M096_N096_K096_TS16XMTS6(
    hc::accelerator_view accl_view, const float *A, __int64_t aOffset,
    const float *B, __int64_t bOffset, float *C, __int64_t cOffset, int M,
    int N, int K, int lda, int ldb, int ldc, float alpha, float beta) {

#undef TILESIZE
#undef MICROTILESIZE
#define TILESIZE 16
#define MICROTILESIZE 6
  int M_ = M / 6;
  int N_ = N / 6;
  int N_R = (N_ + (TILESIZE - 1)) & ~(TILESIZE - 1);
  int M_R = (M_ + (TILESIZE - 1)) & ~(TILESIZE - 1);
  int K_R = (K + (TILESIZE - 1)) & ~(TILESIZE - 1);
  hc::extent<2> grdExt(N_R, M_R);
  hc::tiled_extent<2> t_ext = grdExt.tile(TILESIZE, TILESIZE);
  hc::parallel_for_each(accl_view, t_ext, [=](hc::tiled_index<2> tidx)[[hc]] {
    int shiftTS = hc::fast_math::log2f(TILESIZE);
    float rC[MICROTILESIZE][MICROTILESIZE] = {{static_cast<float>(0)}};
    float rA[1][MICROTILESIZE];
    float rB[1][MICROTILESIZE];
    tile_static float lA[TOTMICROTILEPROD + TILESIZE];
    tile_static float lB[TOTMICROTILEPROD + TILESIZE];
    int gidx = tidx.tile[1];
    int gidy = tidx.tile[0];
    int idx = tidx.local[1];
    int idy = tidx.local[0];
    int block_k = 0;
    int lIndex = (idy * BANKMICROTILESIZE) + idx;
    int AIndex = aOffset + (gidx * MICROTILEPROD) + idx + (idy * lda);
    int BIndex = bOffset + (gidy * MICROTILEPROD) + idx + (idy * ldb);
    __int64_t CIndex = cOffset + (gidx * MICROTILEPROD) + idx +
                       (((gidy * MICROTILEPROD) + idy) * ldc);
    __int64_t CinitOffset = 0;

    do {
      tidx.barrier.wait();

      lB[lIndex + 0 * TILESIZE] =
          B[BIndex + block_k * TILESIZE * ldb + 0 * TILESIZE];
      lB[lIndex + 1 * TILESIZE] =
          B[BIndex + block_k * TILESIZE * ldb + 1 * TILESIZE];
      lB[lIndex + 2 * TILESIZE] =
          B[BIndex + block_k * TILESIZE * ldb + 2 * TILESIZE];
      lB[lIndex + 3 * TILESIZE] =
          B[BIndex + block_k * TILESIZE * ldb + 3 * TILESIZE];
      lB[lIndex + 4 * TILESIZE] =
          B[BIndex + block_k * TILESIZE * ldb + 4 * TILESIZE];
      lB[lIndex + 5 * TILESIZE] =
          B[BIndex + block_k * TILESIZE * ldb + 5 * TILESIZE];
      lA[lIndex + 0 * TILESIZE] =
          A[AIndex + block_k * TILESIZE * lda + 0 * TILESIZE];
      lA[lIndex + 1 * TILESIZE] =
          A[AIndex + block_k * TILESIZE * lda + 1 * TILESIZE];
      lA[lIndex + 2 * TILESIZE] =
          A[AIndex + block_k * TILESIZE * lda + 2 * TILESIZE];
      lA[lIndex + 3 * TILESIZE] =
          A[AIndex + block_k * TILESIZE * lda + 3 * TILESIZE];
      lA[lIndex + 4 * TILESIZE] =
          A[AIndex + block_k * TILESIZE * lda + 4 * TILESIZE];
      lA[lIndex + 5 * TILESIZE] =
          A[AIndex + block_k * TILESIZE * lda + 5 * TILESIZE];

      tidx.barrier.wait();
      int offA = idx;
      int offB = idy;

      for (int iter = 0; iter < TILESIZE; iter += 8) {
        M6x6 M6x6 M6x6 M6x6 M6x6 M6x6 M6x6 M6x6
      }

      tidx.barrier.wait();
    } while (++block_k < (K_R / TILESIZE));

    C[CIndex + CinitOffset + 0 * ldc] =
        alpha * rC[0][0] + beta * C[CIndex + CinitOffset + 0 * ldc];
    C[CIndex + CinitOffset + 1 * TILESIZE * ldc] =
        alpha * rC[0][1] + beta * C[CIndex + CinitOffset + 1 * TILESIZE * ldc];
    C[CIndex + CinitOffset + 2 * TILESIZE * ldc] =
        alpha * rC[0][2] + beta * C[CIndex + CinitOffset + 2 * TILESIZE * ldc];
    C[CIndex + CinitOffset + 3 * TILESIZE * ldc] =
        alpha * rC[0][3] + beta * C[CIndex + CinitOffset + 3 * TILESIZE * ldc];
    C[CIndex + CinitOffset + 4 * TILESIZE * ldc] =
        alpha * rC[0][4] + beta * C[CIndex + CinitOffset + 4 * TILESIZE * ldc];
    C[CIndex + CinitOffset + 5 * TILESIZE * ldc] =
        alpha * rC[0][5] + beta * C[CIndex + CinitOffset + 5 * TILESIZE * ldc];
    CinitOffset += TILESIZE;
    C[CIndex + CinitOffset + 0 * ldc] =
        alpha * rC[1][0] + beta * C[CIndex + CinitOffset + 0 * ldc];
    C[CIndex + CinitOffset + 1 * TILESIZE * ldc] =
        alpha * rC[1][1] + beta * C[CIndex + CinitOffset + 1 * TILESIZE * ldc];
    C[CIndex + CinitOffset + 2 * TILESIZE * ldc] =
        alpha * rC[1][2] + beta * C[CIndex + CinitOffset + 2 * TILESIZE * ldc];
    C[CIndex + CinitOffset + 3 * TILESIZE * ldc] =
        alpha * rC[1][3] + beta * C[CIndex + CinitOffset + 3 * TILESIZE * ldc];
    C[CIndex + CinitOffset + 4 * TILESIZE * ldc] =
        alpha * rC[1][4] + beta * C[CIndex + CinitOffset + 4 * TILESIZE * ldc];
    C[CIndex + CinitOffset + 5 * TILESIZE * ldc] =
        alpha * rC[1][5] + beta * C[CIndex + CinitOffset + 5 * TILESIZE * ldc];
    CinitOffset += TILESIZE;
    C[CIndex + CinitOffset + 0 * ldc] =
        alpha * rC[2][0] + beta * C[CIndex + CinitOffset + 0 * ldc];
    C[CIndex + CinitOffset + 1 * TILESIZE * ldc] =
        alpha * rC[2][1] + beta * C[CIndex + CinitOffset + 1 * TILESIZE * ldc];
    C[CIndex + CinitOffset + 2 * TILESIZE * ldc] =
        alpha * rC[2][2] + beta * C[CIndex + CinitOffset + 2 * TILESIZE * ldc];
    C[CIndex + CinitOffset + 3 * TILESIZE * ldc] =
        alpha * rC[2][3] + beta * C[CIndex + CinitOffset + 3 * TILESIZE * ldc];
    C[CIndex + CinitOffset + 4 * TILESIZE * ldc] =
        alpha * rC[2][4] + beta * C[CIndex + CinitOffset + 4 * TILESIZE * ldc];
    C[CIndex + CinitOffset + 5 * TILESIZE * ldc] =
        alpha * rC[2][5] + beta * C[CIndex + CinitOffset + 5 * TILESIZE * ldc];
    CinitOffset += TILESIZE;
    C[CIndex + CinitOffset + 0 * ldc] =
        alpha * rC[3][0] + beta * C[CIndex + CinitOffset + 0 * ldc];
    C[CIndex + CinitOffset + 1 * TILESIZE * ldc] =
        alpha * rC[3][1] + beta * C[CIndex + CinitOffset + 1 * TILESIZE * ldc];
    C[CIndex + CinitOffset + 2 * TILESIZE * ldc] =
        alpha * rC[3][2] + beta * C[CIndex + CinitOffset + 2 * TILESIZE * ldc];
    C[CIndex + CinitOffset + 3 * TILESIZE * ldc] =
        alpha * rC[3][3] + beta * C[CIndex + CinitOffset + 3 * TILESIZE * ldc];
    C[CIndex + CinitOffset + 4 * TILESIZE * ldc] =
        alpha * rC[3][4] + beta * C[CIndex + CinitOffset + 4 * TILESIZE * ldc];
    C[CIndex + CinitOffset + 5 * TILESIZE * ldc] =
        alpha * rC[3][5] + beta * C[CIndex + CinitOffset + 5 * TILESIZE * ldc];
    CinitOffset += TILESIZE;
    C[CIndex + CinitOffset + 0 * ldc] =
        alpha * rC[4][0] + beta * C[CIndex + CinitOffset + 0 * ldc];
    C[CIndex + CinitOffset + 1 * TILESIZE * ldc] =
        alpha * rC[4][1] + beta * C[CIndex + CinitOffset + 1 * TILESIZE * ldc];
    C[CIndex + CinitOffset + 2 * TILESIZE * ldc] =
        alpha * rC[4][2] + beta * C[CIndex + CinitOffset + 2 * TILESIZE * ldc];
    C[CIndex + CinitOffset + 3 * TILESIZE * ldc] =
        alpha * rC[4][3] + beta * C[CIndex + CinitOffset + 3 * TILESIZE * ldc];
    C[CIndex + CinitOffset + 4 * TILESIZE * ldc] =
        alpha * rC[4][4] + beta * C[CIndex + CinitOffset + 4 * TILESIZE * ldc];
    C[CIndex + CinitOffset + 5 * TILESIZE * ldc] =
        alpha * rC[4][5] + beta * C[CIndex + CinitOffset + 5 * TILESIZE * ldc];
    CinitOffset += TILESIZE;
    C[CIndex + CinitOffset + 0 * ldc] =
        alpha * rC[5][0] + beta * C[CIndex + CinitOffset + 0 * ldc];
    C[CIndex + CinitOffset + 1 * TILESIZE * ldc] =
        alpha * rC[5][1] + beta * C[CIndex + CinitOffset + 1 * TILESIZE * ldc];
    C[CIndex + CinitOffset + 2 * TILESIZE * ldc] =
        alpha * rC[5][2] + beta * C[CIndex + CinitOffset + 2 * TILESIZE * ldc];
    C[CIndex + CinitOffset + 3 * TILESIZE * ldc] =
        alpha * rC[5][3] + beta * C[CIndex + CinitOffset + 3 * TILESIZE * ldc];
    C[CIndex + CinitOffset + 4 * TILESIZE * ldc] =
        alpha * rC[5][4] + beta * C[CIndex + CinitOffset + 4 * TILESIZE * ldc];
    C[CIndex + CinitOffset + 5 * TILESIZE * ldc] =
        alpha * rC[5][5] + beta * C[CIndex + CinitOffset + 5 * TILESIZE * ldc];
  }) ;
#undef TILESIZE
#undef MICROTILESIZE
  return HCBLAS_SUCCEEDS;
}

hcblasStatus gemm_NoTransA_MICRO_NBK_M_N_K_TS16XMTS2(
    hc::accelerator_view accl_view, const float *A, __int64_t aOffset,
    const float *B, __int64_t bOffset, float *C, __int64_t cOffset, int M,
    int N, int K, int lda, int ldb, int ldc, float alpha, float beta) {

#undef TILESIZE
#undef MICROTILESIZE
#define TILESIZE 16
#define MICROTILESIZE 2
  int M_ = (M - 1) / MICROTILESIZE + 1;
  int N_ = (N - 1) / MICROTILESIZE + 1;
  int N_R = (N_ + (TILESIZE - 1)) & ~(TILESIZE - 1);
  int M_R = (M_ + (TILESIZE - 1)) & ~(TILESIZE - 1);
  int K_R = (K + (TILESIZE - 1)) & ~(TILESIZE - 1);
  hc::extent<2> grdExt(N_R, M_R);
  hc::tiled_extent<2> t_ext = grdExt.tile(TILESIZE, TILESIZE);
  hc::parallel_for_each(accl_view, t_ext, [=](hc::tiled_index<2> tidx)[[hc]] {
    int shiftTS = hc::fast_math::log2f(TILESIZE);
    float rC[MICROTILESIZE][MICROTILESIZE] = {{static_cast<float>(0)}};
    float rA[1][MICROTILESIZE];
    float rB[1][MICROTILESIZE];
    tile_static float lA[TOTMICROTILEPROD + TILESIZE];
    tile_static float lB[TOTMICROTILEPROD + TILESIZE];
    int gidx = tidx.tile[1];
    int gidy = tidx.tile[0];
    int idx = tidx.local[1];
    int idy = tidx.local[0];
    int block_k = 0;
    int lIndex = (idy * BANKMICROTILESIZE) + idx;
    int AIndex = aOffset + (gidx * MICROTILEPROD) + idx + (idy * lda);
    int BIndex = bOffset + (gidy * MICROTILEPROD) + idx + (idy * ldb);
    __int64_t CIndex = cOffset + (gidx * MICROTILEPROD) + idx +
                       (((gidy * MICROTILEPROD) + idy) * ldc);
    int N_block = N_R / TILESIZE;
    int M_block = M_R / TILESIZE;
    int K_block = K_R / TILESIZE;
    __int64_t CinitOffset = 0;

    do {
      tidx.barrier.wait();

      if (gidx == M_block - 1 || gidy == N_block - 1 ||
          block_k == K_block - 1) {
        for (int sec = 0; sec < MICROTILESIZE; ++sec) {
          int secVal = sec << shiftTS;

          if ((gidy * MICROTILEPROD + idx + secVal) < N &&
              (block_k * TILESIZE + idy) < K) {
            lB[lIndex + secVal] = B[BIndex + block_k * TILESIZE * ldb + secVal];
          } else {
            lB[lIndex + secVal] = 0;
          }

          if (((gidx * MICROTILEPROD) + idx + secVal) < M &&
              (block_k * TILESIZE + idy) < K) {
            lA[lIndex + secVal] = A[AIndex + block_k * TILESIZE * lda + secVal];
          } else {
            lA[lIndex + secVal] = 0;
          }
        }
      } else {
        lB[lIndex] = B[BIndex + block_k * TILESIZE * ldb];
        lB[lIndex + TILESIZE] = B[BIndex + block_k * TILESIZE * ldb + TILESIZE];
        lA[lIndex] = A[AIndex + block_k * TILESIZE * lda];
        lA[lIndex + TILESIZE] = A[AIndex + block_k * TILESIZE * lda + TILESIZE];
      }

      tidx.barrier.wait();
      int offA = idx;
      int offB = idy;

      for (int iter = 0; iter < TILESIZE; iter += 8) {
        M2x2 M2x2 M2x2 M2x2 M2x2 M2x2 M2x2 M2x2
      }

      tidx.barrier.wait();
    } while (++block_k < (K_R / TILESIZE));

    if (gidx == M_block - 1 || gidy == N_block - 1) {
      if ((gidx * MICROTILEPROD + idx + CinitOffset) < M &&
          (gidy * MICROTILEPROD + idy + 0 * TILESIZE) < N)
        C[CIndex + CinitOffset + 0 * ldc] =
            alpha * rC[0][0] + beta * C[CIndex + CinitOffset + 0 * ldc];
      if ((gidx * MICROTILEPROD + idx + CinitOffset) < M &&
          (gidy * MICROTILEPROD + idy + 1 * TILESIZE) < N)
        C[CIndex + CinitOffset + TILESIZE * ldc] =
            alpha * rC[0][1] + beta * C[CIndex + CinitOffset + TILESIZE * ldc];
      CinitOffset += TILESIZE;
      if ((gidx * MICROTILEPROD + idx + CinitOffset) < M &&
          (gidy * MICROTILEPROD + idy + 0 * TILESIZE) < N)
        C[CIndex + CinitOffset + 0 * ldc] =
            alpha * rC[1][0] + beta * C[CIndex + CinitOffset + 0 * ldc];
      if ((gidx * MICROTILEPROD + idx + CinitOffset) < M &&
          (gidy * MICROTILEPROD + idy + 1 * TILESIZE) < N)
        C[CIndex + CinitOffset + TILESIZE * ldc] =
            alpha * rC[1][1] + beta * C[CIndex + CinitOffset + TILESIZE * ldc];
    } else {
      C[CIndex + CinitOffset + 0 * ldc] =
          alpha * rC[0][0] + beta * C[CIndex + CinitOffset + 0 * ldc];
      C[CIndex + CinitOffset + TILESIZE * ldc] =
          alpha * rC[0][1] + beta * C[CIndex + CinitOffset + TILESIZE * ldc];
      CinitOffset += TILESIZE;
      C[CIndex + CinitOffset + 0 * ldc] =
          alpha * rC[1][0] + beta * C[CIndex + CinitOffset + 0 * ldc];
      C[CIndex + CinitOffset + TILESIZE * ldc] =
          alpha * rC[1][1] + beta * C[CIndex + CinitOffset + TILESIZE * ldc];
    }
  }) ;
#undef TILESIZE
#undef MICROTILESIZE
  return HCBLAS_SUCCEEDS;
}

hcblasStatus gemm_NoTransA_MICRO_NBK_M_N_K_TS16XMTS4(
    hc::accelerator_view accl_view, const float *A, __int64_t aOffset,
    const float *B, __int64_t bOffset, float *C, __int64_t cOffset, int M,
    int N, int K, int lda, int ldb, int ldc, float alpha, float beta) {

#undef TILESIZE
#undef MICROTILESIZE
#define TILESIZE 16
#define MICROTILESIZE 4
  int M_ = (M - 1) / MICROTILESIZE + 1;
  int N_ = (N - 1) / MICROTILESIZE + 1;
  int N_R = (N_ + (TILESIZE - 1)) & ~(TILESIZE - 1);
  int M_R = (M_ + (TILESIZE - 1)) & ~(TILESIZE - 1);
  int K_R = (K + (TILESIZE - 1)) & ~(TILESIZE - 1);
  hc::extent<2> grdExt(N_R, M_R);
  hc::tiled_extent<2> t_ext = grdExt.tile(TILESIZE, TILESIZE);
  hc::parallel_for_each(accl_view, t_ext, [=](hc::tiled_index<2> tidx)[[hc]] {
    int shiftTS = hc::fast_math::log2f(TILESIZE);
    float rC[MICROTILESIZE][MICROTILESIZE] = {{static_cast<float>(0)}};
    float rA[1][MICROTILESIZE];
    float rB[1][MICROTILESIZE];
    tile_static float lA[TOTMICROTILEPROD + TILESIZE];
    tile_static float lB[TOTMICROTILEPROD + TILESIZE];
    int gidx = tidx.tile[1];
    int gidy = tidx.tile[0];
    int idx = tidx.local[1];
    int idy = tidx.local[0];
    int block_k = 0;
    int lIndex = (idy * BANKMICROTILESIZE) + idx;
    int AIndex = aOffset + (gidx * MICROTILEPROD) + idx + (idy * lda);
    int BIndex = bOffset + (gidy * MICROTILEPROD) + idx + (idy * ldb);
    __int64_t CIndex = cOffset + (gidx * MICROTILEPROD) + idx +
                       (((gidy * MICROTILEPROD) + idy) * ldc);
    int N_block = N_R / TILESIZE;
    int M_block = M_R / TILESIZE;
    int K_block = K_R / TILESIZE;
    __int64_t CinitOffset = 0;

    do {
      tidx.barrier.wait();

      if (gidx == M_block - 1 || gidy == N_block - 1 ||
          block_k == K_block - 1) {
        for (int sec = 0; sec < MICROTILESIZE; ++sec) {
          int secVal = sec << shiftTS;

          if ((gidy * MICROTILEPROD + idx + secVal) < N &&
              (block_k * TILESIZE + idy) < K) {
            lB[lIndex + secVal] = B[BIndex + block_k * TILESIZE * ldb + secVal];
          } else {
            lB[lIndex + secVal] = 0;
          }
          tidx.barrier.wait();

          if ((gidx * MICROTILEPROD + idx + secVal) < M &&
              (block_k * TILESIZE + idy) < K) {
            lA[lIndex + secVal] = A[AIndex + block_k * TILESIZE * lda + secVal];
          } else {
            lA[lIndex + secVal] = 0;
          }
        }
      } else {
        lB[lIndex + 0 * TILESIZE] =
            B[BIndex + block_k * TILESIZE * ldb + 0 * TILESIZE];
        lB[lIndex + 1 * TILESIZE] =
            B[BIndex + block_k * TILESIZE * ldb + 1 * TILESIZE];
        lB[lIndex + 2 * TILESIZE] =
            B[BIndex + block_k * TILESIZE * ldb + 2 * TILESIZE];
        lB[lIndex + 3 * TILESIZE] =
            B[BIndex + block_k * TILESIZE * ldb + 3 * TILESIZE];
        lA[lIndex + 0 * TILESIZE] =
            A[AIndex + block_k * TILESIZE * lda + 0 * TILESIZE];
        lA[lIndex + 1 * TILESIZE] =
            A[AIndex + block_k * TILESIZE * lda + 1 * TILESIZE];
        lA[lIndex + 2 * TILESIZE] =
            A[AIndex + block_k * TILESIZE * lda + 2 * TILESIZE];
        lA[lIndex + 3 * TILESIZE] =
            A[AIndex + block_k * TILESIZE * lda + 3 * TILESIZE];
      }

      tidx.barrier.wait();
      int offA = idx;
      int offB = idy;

      for (int iter = 0; iter < TILESIZE; iter += 8) {
        M4x4 M4x4 M4x4 M4x4 M4x4 M4x4 M4x4 M4x4
      }

      tidx.barrier.wait();
    } while (++block_k < (K_R / TILESIZE));

    if (gidx == M_block - 1 || gidy == N_block - 1) {
      if ((gidx * MICROTILEPROD + idx + CinitOffset) < M &&
          (gidy * MICROTILEPROD + idy + 0 * TILESIZE) < N)
        C[CIndex + CinitOffset + 0 * ldc] =
            alpha * rC[0][0] + beta * C[CIndex + CinitOffset + 0 * ldc];
      if ((gidx * MICROTILEPROD + idx + CinitOffset) < M &&
          (gidy * MICROTILEPROD + idy + 1 * TILESIZE) < N)
        C[CIndex + CinitOffset + 1 * TILESIZE * ldc] =
            alpha * rC[0][1] +
            beta * C[CIndex + CinitOffset + 1 * TILESIZE * ldc];
      if ((gidx * MICROTILEPROD + idx + CinitOffset) < M &&
          (gidy * MICROTILEPROD + idy + 2 * TILESIZE) < N)
        C[CIndex + CinitOffset + 2 * TILESIZE * ldc] =
            alpha * rC[0][2] +
            beta * C[CIndex + CinitOffset + 2 * TILESIZE * ldc];
      if ((gidx * MICROTILEPROD + idx + CinitOffset) < M &&
          (gidy * MICROTILEPROD + idy + 3 * TILESIZE) < N)
        C[CIndex + CinitOffset + 3 * TILESIZE * ldc] =
            alpha * rC[0][3] +
            beta * C[CIndex + CinitOffset + 3 * TILESIZE * ldc];
      CinitOffset += TILESIZE;
      if ((gidx * MICROTILEPROD + idx + CinitOffset) < M &&
          (gidy * MICROTILEPROD + idy + 0 * TILESIZE) < N)
        C[CIndex + CinitOffset + 0 * ldc] =
            alpha * rC[1][0] + beta * C[CIndex + CinitOffset + 0 * ldc];
      if ((gidx * MICROTILEPROD + idx + CinitOffset) < M &&
          (gidy * MICROTILEPROD + idy + 1 * TILESIZE) < N)
        C[CIndex + CinitOffset + 1 * TILESIZE * ldc] =
            alpha * rC[1][1] +
            beta * C[CIndex + CinitOffset + 1 * TILESIZE * ldc];
      if ((gidx * MICROTILEPROD + idx + CinitOffset) < M &&
          (gidy * MICROTILEPROD + idy + 2 * TILESIZE) < N)
        C[CIndex + CinitOffset + 2 * TILESIZE * ldc] =
            alpha * rC[1][2] +
            beta * C[CIndex + CinitOffset + 2 * TILESIZE * ldc];
      if ((gidx * MICROTILEPROD + idx + CinitOffset) < M &&
          (gidy * MICROTILEPROD + idy + 3 * TILESIZE) < N)
        C[CIndex + CinitOffset + 3 * TILESIZE * ldc] =
            alpha * rC[1][3] +
            beta * C[CIndex + CinitOffset + 3 * TILESIZE * ldc];
      CinitOffset += TILESIZE;
      if ((gidx * MICROTILEPROD + idx + CinitOffset) < M &&
          (gidy * MICROTILEPROD + idy + 0 * TILESIZE) < N)
        C[CIndex + CinitOffset + 0 * ldc] =
            alpha * rC[2][0] + beta * C[CIndex + CinitOffset + 0 * ldc];
      if ((gidx * MICROTILEPROD + idx + CinitOffset) < M &&
          (gidy * MICROTILEPROD + idy + 1 * TILESIZE) < N)
        C[CIndex + CinitOffset + 1 * TILESIZE * ldc] =
            alpha * rC[2][1] +
            beta * C[CIndex + CinitOffset + 1 * TILESIZE * ldc];
      if ((gidx * MICROTILEPROD + idx + CinitOffset) < M &&
          (gidy * MICROTILEPROD + idy + 2 * TILESIZE) < N)
        C[CIndex + CinitOffset + 2 * TILESIZE * ldc] =
            alpha * rC[2][2] +
            beta * C[CIndex + CinitOffset + 2 * TILESIZE * ldc];
      if ((gidx * MICROTILEPROD + idx + CinitOffset) < M &&
          (gidy * MICROTILEPROD + idy + 3 * TILESIZE) < N)
        C[CIndex + CinitOffset + 3 * TILESIZE * ldc] =
            alpha * rC[2][3] +
            beta * C[CIndex + CinitOffset + 3 * TILESIZE * ldc];
      CinitOffset += TILESIZE;
      if ((gidx * MICROTILEPROD + idx + CinitOffset) < M &&
          (gidy * MICROTILEPROD + idy + 0 * TILESIZE) < N)
        C[CIndex + CinitOffset + 0 * ldc] =
            alpha * rC[3][0] + beta * C[CIndex + CinitOffset + 0 * ldc];
      if ((gidx * MICROTILEPROD + idx + CinitOffset) < M &&
          (gidy * MICROTILEPROD + idy + 1 * TILESIZE) < N)
        C[CIndex + CinitOffset + 1 * TILESIZE * ldc] =
            alpha * rC[3][1] +
            beta * C[CIndex + CinitOffset + 1 * TILESIZE * ldc];
      if ((gidx * MICROTILEPROD + idx + CinitOffset) < M &&
          (gidy * MICROTILEPROD + idy + 2 * TILESIZE) < N)
        C[CIndex + CinitOffset + 2 * TILESIZE * ldc] =
            alpha * rC[3][2] +
            beta * C[CIndex + CinitOffset + 2 * TILESIZE * ldc];
      if ((gidx * MICROTILEPROD + idx + CinitOffset) < M &&
          (gidy * MICROTILEPROD + idy + 3 * TILESIZE) < N)
        C[CIndex + CinitOffset + 3 * TILESIZE * ldc] =
            alpha * rC[3][3] +
            beta * C[CIndex + CinitOffset + 3 * TILESIZE * ldc];
    } else {
      C[CIndex + CinitOffset + 0 * ldc] =
          alpha * rC[0][0] + beta * C[CIndex + CinitOffset + 0 * ldc];
      C[CIndex + CinitOffset + 1 * TILESIZE * ldc] =
          alpha * rC[0][1] +
          beta * C[CIndex + CinitOffset + 1 * TILESIZE * ldc];
      C[CIndex + CinitOffset + 2 * TILESIZE * ldc] =
          alpha * rC[0][2] +
          beta * C[CIndex + CinitOffset + 2 * TILESIZE * ldc];
      C[CIndex + CinitOffset + 3 * TILESIZE * ldc] =
          alpha * rC[0][3] +
          beta * C[CIndex + CinitOffset + 3 * TILESIZE * ldc];
      CinitOffset += TILESIZE;
      C[CIndex + CinitOffset + 0 * ldc] =
          alpha * rC[1][0] + beta * C[CIndex + CinitOffset + 0 * ldc];
      C[CIndex + CinitOffset + 1 * TILESIZE * ldc] =
          alpha * rC[1][1] +
          beta * C[CIndex + CinitOffset + 1 * TILESIZE * ldc];
      C[CIndex + CinitOffset + 2 * TILESIZE * ldc] =
          alpha * rC[1][2] +
          beta * C[CIndex + CinitOffset + 2 * TILESIZE * ldc];
      C[CIndex + CinitOffset + 3 * TILESIZE * ldc] =
          alpha * rC[1][3] +
          beta * C[CIndex + CinitOffset + 3 * TILESIZE * ldc];
      CinitOffset += TILESIZE;
      C[CIndex + CinitOffset + 0 * ldc] =
          alpha * rC[2][0] + beta * C[CIndex + CinitOffset + 0 * ldc];
      C[CIndex + CinitOffset + 1 * TILESIZE * ldc] =
          alpha * rC[2][1] +
          beta * C[CIndex + CinitOffset + 1 * TILESIZE * ldc];
      C[CIndex + CinitOffset + 2 * TILESIZE * ldc] =
          alpha * rC[2][2] +
          beta * C[CIndex + CinitOffset + 2 * TILESIZE * ldc];
      C[CIndex + CinitOffset + 3 * TILESIZE * ldc] =
          alpha * rC[2][3] +
          beta * C[CIndex + CinitOffset + 3 * TILESIZE * ldc];
      CinitOffset += TILESIZE;
      C[CIndex + CinitOffset + 0 * ldc] =
          alpha * rC[3][0] + beta * C[CIndex + CinitOffset + 0 * ldc];
      C[CIndex + CinitOffset + 1 * TILESIZE * ldc] =
          alpha * rC[3][1] +
          beta * C[CIndex + CinitOffset + 1 * TILESIZE * ldc];
      C[CIndex + CinitOffset + 2 * TILESIZE * ldc] =
          alpha * rC[3][2] +
          beta * C[CIndex + CinitOffset + 2 * TILESIZE * ldc];
      C[CIndex + CinitOffset + 3 * TILESIZE * ldc] =
          alpha * rC[3][3] +
          beta * C[CIndex + CinitOffset + 3 * TILESIZE * ldc];
    }
  }) ;
#undef TILESIZE
#undef MICROTILESIZE
  return HCBLAS_SUCCEEDS;
}

hcblasStatus gemm_NoTransA_MICRO_NBK_M_N_K_TS16XMTS6(
    hc::accelerator_view accl_view, const float *A, __int64_t aOffset,
    const float *B, __int64_t bOffset, float *C, __int64_t cOffset, int M,
    int N, int K, int lda, int ldb, int ldc, float alpha, float beta) {

#undef TILESIZE
#undef MICROTILESIZE
#define TILESIZE 16
#define MICROTILESIZE 6
  int M_ = (M - 1) / MICROTILESIZE + 1;
  int N_ = (N - 1) / MICROTILESIZE + 1;
  int N_R = (N_ + (TILESIZE - 1)) & ~(TILESIZE - 1);
  int M_R = (M_ + (TILESIZE - 1)) & ~(TILESIZE - 1);
  int K_R = (K + (TILESIZE - 1)) & ~(TILESIZE - 1);
  hc::extent<2> grdExt(N_R, M_R);
  hc::tiled_extent<2> t_ext = grdExt.tile(TILESIZE, TILESIZE);
  hc::parallel_for_each(accl_view, t_ext, [=](hc::tiled_index<2> tidx)[[hc]] {
    int shiftTS = hc::fast_math::log2f(TILESIZE);
    float rC[MICROTILESIZE][MICROTILESIZE] = {{static_cast<float>(0)}};
    float rA[1][MICROTILESIZE];
    float rB[1][MICROTILESIZE];
    tile_static float lA[TOTMICROTILEPROD + TILESIZE];
    tile_static float lB[TOTMICROTILEPROD + TILESIZE];
    int gidx = tidx.tile[1];
    int gidy = tidx.tile[0];
    int idx = tidx.local[1];
    int idy = tidx.local[0];
    int block_k = 0;
    int lIndex = (idy * BANKMICROTILESIZE) + idx;
    int AIndex = aOffset + (gidx * MICROTILEPROD) + idx + (idy * lda);
    int BIndex = bOffset + (gidy * MICROTILEPROD) + idx + (idy * ldb);
    __int64_t CIndex = cOffset + (gidx * MICROTILEPROD) + idx +
                       (((gidy * MICROTILEPROD) + idy) * ldc);
    int N_block = N_R / TILESIZE;
    int M_block = M_R / TILESIZE;
    int K_block = K_R / TILESIZE;
    __int64_t CinitOffset = 0;

    do {
      tidx.barrier.wait();

      if (gidx == M_block - 1 || gidy == N_block - 1 ||
          block_k == K_block - 1) {
        for (int sec = 0; sec < MICROTILESIZE; ++sec) {
          int secVal = sec << shiftTS;

          if ((gidy * MICROTILEPROD + idx + secVal) < N &&
              (block_k * TILESIZE + idy) < K) {
            lB[lIndex + secVal] = B[BIndex + block_k * TILESIZE * ldb + secVal];
          } else {
            lB[lIndex + secVal] = 0;
          }
          tidx.barrier.wait();

          if ((gidx * MICROTILEPROD + idx + secVal) < M &&
              (block_k * TILESIZE + idy) < K) {
            lA[lIndex + secVal] = A[AIndex + block_k * TILESIZE * lda + secVal];
          } else {
            lA[lIndex + secVal] = 0;
          }
        }
      } else {
        lB[lIndex + 0 * TILESIZE] =
            B[BIndex + block_k * TILESIZE * ldb + 0 * TILESIZE];
        lB[lIndex + 1 * TILESIZE] =
            B[BIndex + block_k * TILESIZE * ldb + 1 * TILESIZE];
        lB[lIndex + 2 * TILESIZE] =
            B[BIndex + block_k * TILESIZE * ldb + 2 * TILESIZE];
        lB[lIndex + 3 * TILESIZE] =
            B[BIndex + block_k * TILESIZE * ldb + 3 * TILESIZE];
        lB[lIndex + 4 * TILESIZE] =
            B[BIndex + block_k * TILESIZE * ldb + 4 * TILESIZE];
        lB[lIndex + 5 * TILESIZE] =
            B[BIndex + block_k * TILESIZE * ldb + 5 * TILESIZE];
        lA[lIndex + 0 * TILESIZE] =
            A[AIndex + block_k * TILESIZE * lda + 0 * TILESIZE];
        lA[lIndex + 1 * TILESIZE] =
            A[AIndex + block_k * TILESIZE * lda + 1 * TILESIZE];
        lA[lIndex + 2 * TILESIZE] =
            A[AIndex + block_k * TILESIZE * lda + 2 * TILESIZE];
        lA[lIndex + 3 * TILESIZE] =
            A[AIndex + block_k * TILESIZE * lda + 3 * TILESIZE];
        lA[lIndex + 4 * TILESIZE] =
            A[AIndex + block_k * TILESIZE * lda + 4 * TILESIZE];
        lA[lIndex + 5 * TILESIZE] =
            A[AIndex + block_k * TILESIZE * lda + 5 * TILESIZE];
      }

      tidx.barrier.wait();
      int offA = idx;
      int offB = idy;

      for (int iter = 0; iter < TILESIZE; iter += 8) {
        M6x6 M6x6 M6x6 M6x6 M6x6 M6x6 M6x6 M6x6
      }

      tidx.barrier.wait();
    } while (++block_k < (K_R / TILESIZE));

    if (gidx == M_block - 1 || gidy == N_block - 1) {
      if ((gidx * MICROTILEPROD + idx + CinitOffset) < M &&
          (gidy * MICROTILEPROD + idy + 0 * TILESIZE) < N)
        C[CIndex + CinitOffset + 0 * ldc] =
            alpha * rC[0][0] + beta * C[CIndex + CinitOffset + 0 * ldc];
      if ((gidx * MICROTILEPROD + idx + CinitOffset) < M &&
          (gidy * MICROTILEPROD + idy + 1 * TILESIZE) < N)
        C[CIndex + CinitOffset + 1 * TILESIZE * ldc] =
            alpha * rC[0][1] +
            beta * C[CIndex + CinitOffset + 1 * TILESIZE * ldc];
      if ((gidx * MICROTILEPROD + idx + CinitOffset) < M &&
          (gidy * MICROTILEPROD + idy + 2 * TILESIZE) < N)
        C[CIndex + CinitOffset + 2 * TILESIZE * ldc] =
            alpha * rC[0][2] +
            beta * C[CIndex + CinitOffset + 2 * TILESIZE * ldc];
      if ((gidx * MICROTILEPROD + idx + CinitOffset) < M &&
          (gidy * MICROTILEPROD + idy + 3 * TILESIZE) < N)
        C[CIndex + CinitOffset + 3 * TILESIZE * ldc] =
            alpha * rC[0][3] +
            beta * C[CIndex + CinitOffset + 3 * TILESIZE * ldc];
      if ((gidx * MICROTILEPROD + idx + CinitOffset) < M &&
          (gidy * MICROTILEPROD + idy + 4 * TILESIZE) < N)
        C[CIndex + CinitOffset + 4 * TILESIZE * ldc] =
            alpha * rC[0][4] +
            beta * C[CIndex + CinitOffset + 4 * TILESIZE * ldc];
      if ((gidx * MICROTILEPROD + idx + CinitOffset) < M &&
          (gidy * MICROTILEPROD + idy + 5 * TILESIZE) < N)
        C[CIndex + CinitOffset + 5 * TILESIZE * ldc] =
            alpha * rC[0][5] +
            beta * C[CIndex + CinitOffset + 5 * TILESIZE * ldc];
      CinitOffset += TILESIZE;
      if ((gidx * MICROTILEPROD + idx + CinitOffset) < M &&
          (gidy * MICROTILEPROD + idy + 0 * TILESIZE) < N)
        C[CIndex + CinitOffset + 0 * ldc] =
            alpha * rC[1][0] + beta * C[CIndex + CinitOffset + 0 * ldc];
      if ((gidx * MICROTILEPROD + idx + CinitOffset) < M &&
          (gidy * MICROTILEPROD + idy + 1 * TILESIZE) < N)
        C[CIndex + CinitOffset + 1 * TILESIZE * ldc] =
            alpha * rC[1][1] +
            beta * C[CIndex + CinitOffset + 1 * TILESIZE * ldc];
      if ((gidx * MICROTILEPROD + idx + CinitOffset) < M &&
          (gidy * MICROTILEPROD + idy + 2 * TILESIZE) < N)
        C[CIndex + CinitOffset + 2 * TILESIZE * ldc] =
            alpha * rC[1][2] +
            beta * C[CIndex + CinitOffset + 2 * TILESIZE * ldc];
      if ((gidx * MICROTILEPROD + idx + CinitOffset) < M &&
          (gidy * MICROTILEPROD + idy + 3 * TILESIZE) < N)
        C[CIndex + CinitOffset + 3 * TILESIZE * ldc] =
            alpha * rC[1][3] +
            beta * C[CIndex + CinitOffset + 3 * TILESIZE * ldc];
      if ((gidx * MICROTILEPROD + idx + CinitOffset) < M &&
          (gidy * MICROTILEPROD + idy + 4 * TILESIZE) < N)
        C[CIndex + CinitOffset + 4 * TILESIZE * ldc] =
            alpha * rC[1][4] +
            beta * C[CIndex + CinitOffset + 4 * TILESIZE * ldc];
      if ((gidx * MICROTILEPROD + idx + CinitOffset) < M &&
          (gidy * MICROTILEPROD + idy + 5 * TILESIZE) < N)
        C[CIndex + CinitOffset + 5 * TILESIZE * ldc] =
            alpha * rC[1][5] +
            beta * C[CIndex + CinitOffset + 5 * TILESIZE * ldc];
      CinitOffset += TILESIZE;
      if ((gidx * MICROTILEPROD + idx + CinitOffset) < M &&
          (gidy * MICROTILEPROD + idy + 0 * TILESIZE) < N)
        C[CIndex + CinitOffset + 0 * ldc] =
            alpha * rC[2][0] + beta * C[CIndex + CinitOffset + 0 * ldc];
      if ((gidx * MICROTILEPROD + idx + CinitOffset) < M &&
          (gidy * MICROTILEPROD + idy + 1 * TILESIZE) < N)
        C[CIndex + CinitOffset + 1 * TILESIZE * ldc] =
            alpha * rC[2][1] +
            beta * C[CIndex + CinitOffset + 1 * TILESIZE * ldc];
      if ((gidx * MICROTILEPROD + idx + CinitOffset) < M &&
          (gidy * MICROTILEPROD + idy + 2 * TILESIZE) < N)
        C[CIndex + CinitOffset + 2 * TILESIZE * ldc] =
            alpha * rC[2][2] +
            beta * C[CIndex + CinitOffset + 2 * TILESIZE * ldc];
      if ((gidx * MICROTILEPROD + idx + CinitOffset) < M &&
          (gidy * MICROTILEPROD + idy + 3 * TILESIZE) < N)
        C[CIndex + CinitOffset + 3 * TILESIZE * ldc] =
            alpha * rC[2][3] +
            beta * C[CIndex + CinitOffset + 3 * TILESIZE * ldc];
      if ((gidx * MICROTILEPROD + idx + CinitOffset) < M &&
          (gidy * MICROTILEPROD + idy + 4 * TILESIZE) < N)
        C[CIndex + CinitOffset + 4 * TILESIZE * ldc] =
            alpha * rC[2][4] +
            beta * C[CIndex + CinitOffset + 4 * TILESIZE * ldc];
      if ((gidx * MICROTILEPROD + idx + CinitOffset) < M &&
          (gidy * MICROTILEPROD + idy + 5 * TILESIZE) < N)
        C[CIndex + CinitOffset + 5 * TILESIZE * ldc] =
            alpha * rC[2][5] +
            beta * C[CIndex + CinitOffset + 5 * TILESIZE * ldc];
      CinitOffset += TILESIZE;
      if ((gidx * MICROTILEPROD + idx + CinitOffset) < M &&
          (gidy * MICROTILEPROD + idy + 0 * TILESIZE) < N)
        C[CIndex + CinitOffset + 0 * ldc] =
            alpha * rC[3][0] + beta * C[CIndex + CinitOffset + 0 * ldc];
      if ((gidx * MICROTILEPROD + idx + CinitOffset) < M &&
          (gidy * MICROTILEPROD + idy + 1 * TILESIZE) < N)
        C[CIndex + CinitOffset + 1 * TILESIZE * ldc] =
            alpha * rC[3][1] +
            beta * C[CIndex + CinitOffset + 1 * TILESIZE * ldc];
      if ((gidx * MICROTILEPROD + idx + CinitOffset) < M &&
          (gidy * MICROTILEPROD + idy + 2 * TILESIZE) < N)
        C[CIndex + CinitOffset + 2 * TILESIZE * ldc] =
            alpha * rC[3][2] +
            beta * C[CIndex + CinitOffset + 2 * TILESIZE * ldc];
      if ((gidx * MICROTILEPROD + idx + CinitOffset) < M &&
          (gidy * MICROTILEPROD + idy + 3 * TILESIZE) < N)
        C[CIndex + CinitOffset + 3 * TILESIZE * ldc] =
            alpha * rC[3][3] +
            beta * C[CIndex + CinitOffset + 3 * TILESIZE * ldc];
      if ((gidx * MICROTILEPROD + idx + CinitOffset) < M &&
          (gidy * MICROTILEPROD + idy + 4 * TILESIZE) < N)
        C[CIndex + CinitOffset + 4 * TILESIZE * ldc] =
            alpha * rC[3][4] +
            beta * C[CIndex + CinitOffset + 4 * TILESIZE * ldc];
      if ((gidx * MICROTILEPROD + idx + CinitOffset) < M &&
          (gidy * MICROTILEPROD + idy + 5 * TILESIZE) < N)
        C[CIndex + CinitOffset + 5 * TILESIZE * ldc] =
            alpha * rC[3][5] +
            beta * C[CIndex + CinitOffset + 5 * TILESIZE * ldc];
      CinitOffset += TILESIZE;
      if ((gidx * MICROTILEPROD + idx + CinitOffset) < M &&
          (gidy * MICROTILEPROD + idy + 0 * TILESIZE) < N)
        C[CIndex + CinitOffset + 0 * ldc] =
            alpha * rC[4][0] + beta * C[CIndex + CinitOffset + 0 * ldc];
      if ((gidx * MICROTILEPROD + idx + CinitOffset) < M &&
          (gidy * MICROTILEPROD + idy + 1 * TILESIZE) < N)
        C[CIndex + CinitOffset + 1 * TILESIZE * ldc] =
            alpha * rC[4][1] +
            beta * C[CIndex + CinitOffset + 1 * TILESIZE * ldc];
      if ((gidx * MICROTILEPROD + idx + CinitOffset) < M &&
          (gidy * MICROTILEPROD + idy + 2 * TILESIZE) < N)
        C[CIndex + CinitOffset + 2 * TILESIZE * ldc] =
            alpha * rC[4][2] +
            beta * C[CIndex + CinitOffset + 2 * TILESIZE * ldc];
      if ((gidx * MICROTILEPROD + idx + CinitOffset) < M &&
          (gidy * MICROTILEPROD + idy + 3 * TILESIZE) < N)
        C[CIndex + CinitOffset + 3 * TILESIZE * ldc] =
            alpha * rC[4][3] +
            beta * C[CIndex + CinitOffset + 3 * TILESIZE * ldc];
      if ((gidx * MICROTILEPROD + idx + CinitOffset) < M &&
          (gidy * MICROTILEPROD + idy + 4 * TILESIZE) < N)
        C[CIndex + CinitOffset + 4 * TILESIZE * ldc] =
            alpha * rC[4][4] +
            beta * C[CIndex + CinitOffset + 4 * TILESIZE * ldc];
      if ((gidx * MICROTILEPROD + idx + CinitOffset) < M &&
          (gidy * MICROTILEPROD + idy + 5 * TILESIZE) < N)
        C[CIndex + CinitOffset + 5 * TILESIZE * ldc] =
            alpha * rC[4][5] +
            beta * C[CIndex + CinitOffset + 5 * TILESIZE * ldc];
      CinitOffset += TILESIZE;
      if ((gidx * MICROTILEPROD + idx + CinitOffset) < M &&
          (gidy * MICROTILEPROD + idy + 0 * TILESIZE) < N)
        C[CIndex + CinitOffset + 0 * ldc] =
            alpha * rC[5][0] + beta * C[CIndex + CinitOffset + 0 * ldc];
      if ((gidx * MICROTILEPROD + idx + CinitOffset) < M &&
          (gidy * MICROTILEPROD + idy + 1 * TILESIZE) < N)
        C[CIndex + CinitOffset + 1 * TILESIZE * ldc] =
            alpha * rC[5][1] +
            beta * C[CIndex + CinitOffset + 1 * TILESIZE * ldc];
      if ((gidx * MICROTILEPROD + idx + CinitOffset) < M &&
          (gidy * MICROTILEPROD + idy + 2 * TILESIZE) < N)
        C[CIndex + CinitOffset + 2 * TILESIZE * ldc] =
            alpha * rC[5][2] +
            beta * C[CIndex + CinitOffset + 2 * TILESIZE * ldc];
      if ((gidx * MICROTILEPROD + idx + CinitOffset) < M &&
          (gidy * MICROTILEPROD + idy + 3 * TILESIZE) < N)
        C[CIndex + CinitOffset + 3 * TILESIZE * ldc] =
            alpha * rC[5][3] +
            beta * C[CIndex + CinitOffset + 3 * TILESIZE * ldc];
      if ((gidx * MICROTILEPROD + idx + CinitOffset) < M &&
          (gidy * MICROTILEPROD + idy + 4 * TILESIZE) < N)
        C[CIndex + CinitOffset + 4 * TILESIZE * ldc] =
            alpha * rC[5][4] +
            beta * C[CIndex + CinitOffset + 4 * TILESIZE * ldc];
      if ((gidx * MICROTILEPROD + idx + CinitOffset) < M &&
          (gidy * MICROTILEPROD + idy + 5 * TILESIZE) < N)
        C[CIndex + CinitOffset + 5 * TILESIZE * ldc] =
            alpha * rC[5][5] +
            beta * C[CIndex + CinitOffset + 5 * TILESIZE * ldc];
    } else {
      C[CIndex + CinitOffset + 0 * ldc] =
          alpha * rC[0][0] + beta * C[CIndex + CinitOffset + 0 * ldc];
      C[CIndex + CinitOffset + 1 * TILESIZE * ldc] =
          alpha * rC[0][1] +
          beta * C[CIndex + CinitOffset + 1 * TILESIZE * ldc];
      C[CIndex + CinitOffset + 2 * TILESIZE * ldc] =
          alpha * rC[0][2] +
          beta * C[CIndex + CinitOffset + 2 * TILESIZE * ldc];
      C[CIndex + CinitOffset + 3 * TILESIZE * ldc] =
          alpha * rC[0][3] +
          beta * C[CIndex + CinitOffset + 3 * TILESIZE * ldc];
      C[CIndex + CinitOffset + 4 * TILESIZE * ldc] =
          alpha * rC[0][4] +
          beta * C[CIndex + CinitOffset + 4 * TILESIZE * ldc];
      C[CIndex + CinitOffset + 5 * TILESIZE * ldc] =
          alpha * rC[0][5] +
          beta * C[CIndex + CinitOffset + 5 * TILESIZE * ldc];
      CinitOffset += TILESIZE;
      C[CIndex + CinitOffset + 0 * ldc] =
          alpha * rC[1][0] + beta * C[CIndex + CinitOffset + 0 * ldc];
      C[CIndex + CinitOffset + 1 * TILESIZE * ldc] =
          alpha * rC[1][1] +
          beta * C[CIndex + CinitOffset + 1 * TILESIZE * ldc];
      C[CIndex + CinitOffset + 2 * TILESIZE * ldc] =
          alpha * rC[1][2] +
          beta * C[CIndex + CinitOffset + 2 * TILESIZE * ldc];
      C[CIndex + CinitOffset + 3 * TILESIZE * ldc] =
          alpha * rC[1][3] +
          beta * C[CIndex + CinitOffset + 3 * TILESIZE * ldc];
      C[CIndex + CinitOffset + 4 * TILESIZE * ldc] =
          alpha * rC[1][4] +
          beta * C[CIndex + CinitOffset + 4 * TILESIZE * ldc];
      C[CIndex + CinitOffset + 5 * TILESIZE * ldc] =
          alpha * rC[1][5] +
          beta * C[CIndex + CinitOffset + 5 * TILESIZE * ldc];
      CinitOffset += TILESIZE;
      C[CIndex + CinitOffset + 0 * ldc] =
          alpha * rC[2][0] + beta * C[CIndex + CinitOffset + 0 * ldc];
      C[CIndex + CinitOffset + 1 * TILESIZE * ldc] =
          alpha * rC[2][1] +
          beta * C[CIndex + CinitOffset + 1 * TILESIZE * ldc];
      C[CIndex + CinitOffset + 2 * TILESIZE * ldc] =
          alpha * rC[2][2] +
          beta * C[CIndex + CinitOffset + 2 * TILESIZE * ldc];
      C[CIndex + CinitOffset + 3 * TILESIZE * ldc] =
          alpha * rC[2][3] +
          beta * C[CIndex + CinitOffset + 3 * TILESIZE * ldc];
      C[CIndex + CinitOffset + 4 * TILESIZE * ldc] =
          alpha * rC[2][4] +
          beta * C[CIndex + CinitOffset + 4 * TILESIZE * ldc];
      C[CIndex + CinitOffset + 5 * TILESIZE * ldc] =
          alpha * rC[2][5] +
          beta * C[CIndex + CinitOffset + 5 * TILESIZE * ldc];
      CinitOffset += TILESIZE;
      C[CIndex + CinitOffset + 0 * ldc] =
          alpha * rC[3][0] + beta * C[CIndex + CinitOffset + 0 * ldc];
      C[CIndex + CinitOffset + 1 * TILESIZE * ldc] =
          alpha * rC[3][1] +
          beta * C[CIndex + CinitOffset + 1 * TILESIZE * ldc];
      C[CIndex + CinitOffset + 2 * TILESIZE * ldc] =
          alpha * rC[3][2] +
          beta * C[CIndex + CinitOffset + 2 * TILESIZE * ldc];
      C[CIndex + CinitOffset + 3 * TILESIZE * ldc] =
          alpha * rC[3][3] +
          beta * C[CIndex + CinitOffset + 3 * TILESIZE * ldc];
      C[CIndex + CinitOffset + 4 * TILESIZE * ldc] =
          alpha * rC[3][4] +
          beta * C[CIndex + CinitOffset + 4 * TILESIZE * ldc];
      C[CIndex + CinitOffset + 5 * TILESIZE * ldc] =
          alpha * rC[3][5] +
          beta * C[CIndex + CinitOffset + 5 * TILESIZE * ldc];
      CinitOffset += TILESIZE;
      C[CIndex + CinitOffset + 0 * ldc] =
          alpha * rC[4][0] + beta * C[CIndex + CinitOffset + 0 * ldc];
      C[CIndex + CinitOffset + 1 * TILESIZE * ldc] =
          alpha * rC[4][1] +
          beta * C[CIndex + CinitOffset + 1 * TILESIZE * ldc];
      C[CIndex + CinitOffset + 2 * TILESIZE * ldc] =
          alpha * rC[4][2] +
          beta * C[CIndex + CinitOffset + 2 * TILESIZE * ldc];
      C[CIndex + CinitOffset + 3 * TILESIZE * ldc] =
          alpha * rC[4][3] +
          beta * C[CIndex + CinitOffset + 3 * TILESIZE * ldc];
      C[CIndex + CinitOffset + 4 * TILESIZE * ldc] =
          alpha * rC[4][4] +
          beta * C[CIndex + CinitOffset + 4 * TILESIZE * ldc];
      C[CIndex + CinitOffset + 5 * TILESIZE * ldc] =
          alpha * rC[4][5] +
          beta * C[CIndex + CinitOffset + 5 * TILESIZE * ldc];
      CinitOffset += TILESIZE;
      C[CIndex + CinitOffset + 0 * ldc] =
          alpha * rC[5][0] + beta * C[CIndex + CinitOffset + 0 * ldc];
      C[CIndex + CinitOffset + 1 * TILESIZE * ldc] =
          alpha * rC[5][1] +
          beta * C[CIndex + CinitOffset + 1 * TILESIZE * ldc];
      C[CIndex + CinitOffset + 2 * TILESIZE * ldc] =
          alpha * rC[5][2] +
          beta * C[CIndex + CinitOffset + 2 * TILESIZE * ldc];
      C[CIndex + CinitOffset + 3 * TILESIZE * ldc] =
          alpha * rC[5][3] +
          beta * C[CIndex + CinitOffset + 3 * TILESIZE * ldc];
      C[CIndex + CinitOffset + 4 * TILESIZE * ldc] =
          alpha * rC[5][4] +
          beta * C[CIndex + CinitOffset + 4 * TILESIZE * ldc];
      C[CIndex + CinitOffset + 5 * TILESIZE * ldc] =
          alpha * rC[5][5] +
          beta * C[CIndex + CinitOffset + 5 * TILESIZE * ldc];
    }
  }) ;
#undef TILESIZE
#undef MICROTILESIZE
  return HCBLAS_SUCCEEDS;
}

