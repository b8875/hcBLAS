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

#include "include/hcblas.h"
#include "include/hcblaslib.h"
#include "gtest/gtest.h"
#include <cblas.h>
#include <cstdlib>
#include <hc_am.hpp>

unsigned int global_seed = 100;

TEST(hcblas_saxpy, return_correct_saxpy_Implementation_type_1) {
  hc::accelerator accl;
  hc::accelerator_view av = accl.get_default_view();
  Hcblaslibrary hc(&av);
  int N = 279;
  int incX = 1;
  int incY = 1;
  __int64_t yOffset = 0;
  __int64_t xOffset = 0;
  hcblasStatus status;
  float alpha = 1;
  __int64_t lenx = 1 + (N - 1) * abs(incX);
  __int64_t leny = 1 + (N - 1) * abs(incY);
  float *X = (float *)calloc(lenx, sizeof(float));
  float *Y = (float *)calloc(leny, sizeof(float));
  hc::accelerator_view accl_view = hc.currentAcclView;
  hc::accelerator acc = hc.currentAccl;
  /* Implementation type I - Inputs and Outputs are HCC device pointers */
  float *devX = hc::am_alloc(sizeof(float) * lenx, acc, 0);
  float *devY = hc::am_alloc(sizeof(float) * leny, acc, 0);
  for (int i = 0; i < lenx; i++) {
    X[i] = rand_r(&global_seed) % 10;
  }
  for (int i = 0; i < leny; i++) {
    Y[i] = rand_r(&global_seed) % 15;
  }
  accl_view.copy(X, devX, lenx * sizeof(float));
  accl_view.copy(Y, devY, leny * sizeof(float));
  /* Proper call */
  status = hc.hcblas_saxpy(accl_view, N, alpha, devX, incX, devY, incY, xOffset,
                           yOffset);
  EXPECT_EQ(status, HCBLAS_SUCCEEDS);
  /* X and Y are not properly allocated */
  float *devX1 = NULL;
  float *devY1 = NULL;
  status = hc.hcblas_saxpy(accl_view, N, alpha, devX1, incX, devY, incY,
                           xOffset, yOffset);
  EXPECT_EQ(status, HCBLAS_INVALID);
  status = hc.hcblas_saxpy(accl_view, N, alpha, devX, incX, devY1, incY,
                           xOffset, yOffset);
  EXPECT_EQ(status, HCBLAS_INVALID);
  /* alpha is 0 */
  alpha = 0;
  status = hc.hcblas_saxpy(accl_view, N, alpha, devX, incX, devY, incY, xOffset,
                           yOffset);
  EXPECT_EQ(status, HCBLAS_SUCCEEDS);
  /* N is 0 */
  N = 0;
  status = hc.hcblas_saxpy(accl_view, N, alpha, devX, incX, devY, incY, xOffset,
                           yOffset);
  EXPECT_EQ(status, HCBLAS_INVALID);
  /* incX is 0 */
  incX = 0;
  status = hc.hcblas_saxpy(accl_view, N, alpha, devX, incX, devY, incY, xOffset,
                           yOffset);
  EXPECT_EQ(status, HCBLAS_INVALID);
  /* incY is 0 */
  incX = 1;
  incY = 0;
  status = hc.hcblas_saxpy(accl_view, N, alpha, devX, incX, devY, incY, xOffset,
                           yOffset);
  EXPECT_EQ(status, HCBLAS_INVALID);
  // Mandatory wait after kernel invocations when no copy to host happens
  accl_view.wait();
  free(X);
  free(Y);
  hc::am_free(devX);
  hc::am_free(devY);
}

TEST(hcblas_saxpy, func_correct_saxpy_Implementation_type_1) {
  hc::accelerator accl;
  hc::accelerator_view av = accl.get_default_view();
  Hcblaslibrary hc(&av);
  int N = 279;
  int incX = 1;
  int incY = 1;
  __int64_t yOffset = 0;
  __int64_t xOffset = 0;
  hcblasStatus status;
  float alpha = 1;
  __int64_t lenx = 1 + (N - 1) * abs(incX);
  __int64_t leny = 1 + (N - 1) * abs(incY);
  float *X = (float *)calloc(lenx, sizeof(float));
  float *Y = (float *)calloc(leny, sizeof(float));
  float *Ycblas = (float *)calloc(N, sizeof(float));
  hc::accelerator_view accl_view = hc.currentAcclView;
  hc::accelerator acc = hc.currentAccl;
  /* Implementation type I - Inputs and Outputs are HCC device pointers */
  float *devX = hc::am_alloc(sizeof(float) * lenx, acc, 0);
  float *devY = hc::am_alloc(sizeof(float) * leny, acc, 0);
  for (int i = 0; i < lenx; i++) {
    X[i] = rand_r(&global_seed) % 10;
  }
  for (int i = 0; i < leny; i++) {
    Y[i] = rand_r(&global_seed) % 15;
    Ycblas[i] = Y[i];
  }
  accl_view.copy(X, devX, lenx * sizeof(float));
  accl_view.copy(Y, devY, leny * sizeof(float));
  /* Proper call */
  status = hc.hcblas_saxpy(accl_view, N, alpha, devX, incX, devY, incY, xOffset,
                           yOffset);
  EXPECT_EQ(status, HCBLAS_SUCCEEDS);
  accl_view.copy(devY, Y, leny * sizeof(float));
  cblas_saxpy(N, alpha, X, incX, Ycblas, incY);
  for (int i = 0; i < leny; i++) EXPECT_EQ(Y[i], Ycblas[i]);
  free(X);
  free(Y);
  free(Ycblas);
  hc::am_free(devX);
  hc::am_free(devY);
}

TEST(hcblas_saxpy, return_correct_saxpy_Implementation_type_2) {
  hc::accelerator accl;
  hc::accelerator_view av = accl.get_default_view();
  Hcblaslibrary hc(&av);
  int N = 279;
  int incX = 1;
  int incY = 1;
  __int64_t yOffset = 0;
  int batchSize = 128;
  __int64_t xOffset = 0;
  float alpha = 1;
  hcblasStatus status;
  __int64_t X_batchOffset = N;
  __int64_t Y_batchOffset = N;
  __int64_t lenx = 1 + (N - 1) * abs(incX);
  __int64_t leny = 1 + (N - 1) * abs(incY);
  float *Xbatch = (float *)calloc(lenx * batchSize, sizeof(float));
  float *Ybatch = (float *)calloc(leny * batchSize, sizeof(float));
  hc::accelerator_view accl_view = hc.currentAcclView;
  hc::accelerator acc = hc.currentAccl;
  float *devXbatch = hc::am_alloc(sizeof(float) * lenx * batchSize, acc, 0);
  float *devYbatch = hc::am_alloc(sizeof(float) * leny * batchSize, acc, 0);
  /* Implementation type II - Inputs and Outputs are HCC float array containers
   * with batch processing */
  for (int i = 0; i < lenx * batchSize; i++) {
    Xbatch[i] = rand_r(&global_seed) % 10;
  }
  for (int i = 0; i < leny * batchSize; i++) {
    Ybatch[i] = rand_r(&global_seed) % 15;
  }
  accl_view.copy(Xbatch, devXbatch, lenx * batchSize * sizeof(float));
  accl_view.copy(Ybatch, devYbatch, leny * batchSize * sizeof(float));
  /* Proper call */
  status = hc.hcblas_saxpy(accl_view, N, alpha, devXbatch, incX, X_batchOffset,
                           devYbatch, incY, Y_batchOffset, xOffset, yOffset,
                           batchSize);
  EXPECT_EQ(status, HCBLAS_SUCCEEDS);
  /* alpha is 0*/
  alpha = 0;
  status = hc.hcblas_saxpy(accl_view, N, alpha, devXbatch, incX, X_batchOffset,
                           devYbatch, incY, Y_batchOffset, xOffset, yOffset,
                           batchSize);
  EXPECT_EQ(status, HCBLAS_SUCCEEDS);
  /* X and Y are not properly allocated */
  float *devX1 = NULL;
  float *devY1 = NULL;
  status = hc.hcblas_saxpy(accl_view, N, alpha, devX1, incX, X_batchOffset,
                           devYbatch, incY, Y_batchOffset, xOffset, yOffset,
                           batchSize);
  EXPECT_EQ(status, HCBLAS_INVALID);
  status =
      hc.hcblas_saxpy(accl_view, N, alpha, devXbatch, incX, X_batchOffset,
                      devY1, incY, Y_batchOffset, xOffset, yOffset, batchSize);
  EXPECT_EQ(status, HCBLAS_INVALID);
  /* N is 0 */
  N = 0;
  status = hc.hcblas_saxpy(accl_view, N, alpha, devXbatch, incX, X_batchOffset,
                           devYbatch, incY, Y_batchOffset, xOffset, yOffset,
                           batchSize);
  EXPECT_EQ(status, HCBLAS_INVALID);
  /* incX is 0 */
  incX = 0;
  status = hc.hcblas_saxpy(accl_view, N, alpha, devXbatch, incX, X_batchOffset,
                           devYbatch, incY, Y_batchOffset, xOffset, yOffset,
                           batchSize);
  EXPECT_EQ(status, HCBLAS_INVALID);
  /* incY is 0 */
  incX = 1;
  incY = 0;
  status = hc.hcblas_saxpy(accl_view, N, alpha, devXbatch, incX, X_batchOffset,
                           devYbatch, incY, Y_batchOffset, xOffset, yOffset,
                           batchSize);
  EXPECT_EQ(status, HCBLAS_INVALID);
  // Mandatory wait after kernel invocations when no copy to host happens
  accl_view.wait();
  free(Xbatch);
  free(Ybatch);
  hc::am_free(devXbatch);
  hc::am_free(devYbatch);
}

TEST(hcblas_saxpy, func_correct_saxpy_Implementation_type_2) {
  hc::accelerator accl;
  hc::accelerator_view av = accl.get_default_view();
  Hcblaslibrary hc(&av);
  int N = 279;
  int incX = 1;
  int incY = 1;
  __int64_t yOffset = 0;
  int batchSize = 128;
  __int64_t xOffset = 0;
  float alpha = 1;
  hcblasStatus status;
  __int64_t X_batchOffset = N;
  __int64_t Y_batchOffset = N;
  __int64_t lenx = 1 + (N - 1) * abs(incX);
  __int64_t leny = 1 + (N - 1) * abs(incY);
  float *Xbatch = (float *)calloc(lenx * batchSize, sizeof(float));
  float *Ybatch = (float *)calloc(leny * batchSize, sizeof(float));
  float *Ycblasbatch = (float *)calloc(N * batchSize, sizeof(float));
  hc::accelerator_view accl_view = hc.currentAcclView;
  hc::accelerator acc = hc.currentAccl;
  float *devXbatch = hc::am_alloc(sizeof(float) * lenx * batchSize, acc, 0);
  float *devYbatch = hc::am_alloc(sizeof(float) * leny * batchSize, acc, 0);
  /* Implementation type II - Inputs and Outputs are HCC float array containers
   * with batch processing */
  for (int i = 0; i < lenx * batchSize; i++) {
    Xbatch[i] = rand_r(&global_seed) % 10;
  }
  for (int i = 0; i < leny * batchSize; i++) {
    Ybatch[i] = rand_r(&global_seed) % 15;
    Ycblasbatch[i] = Ybatch[i];
  }
  accl_view.copy(Xbatch, devXbatch, lenx * batchSize * sizeof(float));
  accl_view.copy(Ybatch, devYbatch, leny * batchSize * sizeof(float));
  /* Proper call */
  status = hc.hcblas_saxpy(accl_view, N, alpha, devXbatch, incX, X_batchOffset,
                           devYbatch, incY, Y_batchOffset, xOffset, yOffset,
                           batchSize);
  EXPECT_EQ(status, HCBLAS_SUCCEEDS);
  accl_view.copy(devYbatch, Ybatch, leny * batchSize * sizeof(float));
  for (int i = 0; i < batchSize; i++)
    cblas_saxpy(N, alpha, Xbatch + i * N, incX, Ycblasbatch + i * N, incY);
  for (int i = 0; i < leny * batchSize; i++)
    EXPECT_EQ(Ybatch[i], Ycblasbatch[i]);
  free(Xbatch);
  free(Ybatch);
  free(Ycblasbatch);
  hc::am_free(devXbatch);
  hc::am_free(devYbatch);
}
