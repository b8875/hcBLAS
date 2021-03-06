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
#include <hc_am.hpp>

TEST(hcblasCreateTest, return_Check_hcblasCreate) {
  // Case I: Input to the API is null handle
  hcblasHandle_t handle = NULL;
  hc::accelerator default_acc;
  hc::accelerator_view av = default_acc.get_default_view();
  // Passing a Null handle and default accelerator to the API
  hcblasStatus_t status = hcblasCreate(&handle, &av);
  // Assert if the handle is still NULL after allocation
  EXPECT_TRUE(handle != NULL);
  // If allocation succeeds we must expect a success status
  if (handle != NULL)
    EXPECT_EQ(status, HCBLAS_STATUS_SUCCESS);
  else
    EXPECT_EQ(status, HCBLAS_STATUS_ALLOC_FAILED);
}

TEST(hcblasDestroyTest, return_Check_hcblasDestroy) {
  hcblasHandle_t handle = NULL;
  hc::accelerator default_acc;
  hc::accelerator_view av = default_acc.get_default_view();
  // Passing a Null handle and default accelerator to the API
  hcblasStatus_t status = hcblasCreate(&handle, &av);
  // hcblasDestroy
  status = hcblasDestroy(&handle);
  EXPECT_EQ(status, HCBLAS_STATUS_SUCCESS);
  // Destory again
  status = hcblasDestroy(&handle);
  EXPECT_EQ(status, HCBLAS_STATUS_NOT_INITIALIZED);
}

TEST(hcblasSetGetAcclViewTest, func_and_return_check_hcblasSetGetAcclView) {
  // Case I: Input to the API is null handle
  hcblasHandle_t handle = NULL;
  void **stream = NULL;
  hc::accelerator default_acc;
  hc::accelerator_view av = default_acc.get_default_view();
  hc::accelerator_view default_acc_view = default_acc.get_default_view();
  hc::accelerator_view *accl_view = NULL;
  hcblasStatus_t status = hcblasSetAcclView(handle, default_acc_view);
  EXPECT_EQ(status, HCBLAS_STATUS_NOT_INITIALIZED);
  status = hcblasGetAcclView(handle, accl_view, stream);
  EXPECT_EQ(status, HCBLAS_STATUS_NOT_INITIALIZED);
  // Passing a Null handle and default accelerator to the API
  status = hcblasCreate(&handle, &av);
  // Assert if the handle is still NULL after allocation
  EXPECT_TRUE(handle != NULL);
  // If allocation succeeds we must expect a success status
  if (handle != NULL)
    EXPECT_EQ(status, HCBLAS_STATUS_SUCCESS);
  else
    EXPECT_EQ(status, HCBLAS_STATUS_ALLOC_FAILED);

  status = hcblasSetAcclView(handle, default_acc_view);
  EXPECT_EQ(status, HCBLAS_STATUS_SUCCESS);
  // Now Get the Accl_view
  status = hcblasGetAcclView(handle, accl_view, stream);
  EXPECT_EQ(status, HCBLAS_STATUS_SUCCESS);
  EXPECT_TRUE(accl_view != NULL);
  if (default_acc_view == *accl_view) {
    EXPECT_EQ(status, HCBLAS_STATUS_SUCCESS);
  }
  // We must expect the accl_view obtained is what that's being set
  EXPECT_EQ(default_acc_view, *accl_view);
}

TEST(hcblasSetVectorTest, return_Check_hcblasSetVector) {
  int n = 10;
  int incx = 1, incy = 1;
  float *x1 = (float *)calloc(n, sizeof(float));
  double *x2 = (double *)calloc(n, sizeof(double));
  hcblasStatus_t status;
  hcblasHandle_t handle = NULL;
  hc::accelerator default_acc;
  hc::accelerator_view av = default_acc.get_default_view();

  // Passing a Null handle and default accelerator to the API
  status = hcblasCreate(&handle, &av);
  float *y1 = (float *)am_alloc(n * sizeof(float), handle->currentAccl, 0);
  double *y2 = (double *)am_alloc(n * sizeof(double), handle->currentAccl, 0);
  // HCBLAS_STATUS_SUCCESS
  // float type memory transfer from host to device
  status = hcblasSetVector(handle, n, sizeof(float), x1, incx, y1, incy);
  EXPECT_EQ(status, HCBLAS_STATUS_SUCCESS);
  // double type memory transfer from host to device
  status = hcblasSetVector(handle, n, sizeof(double), x2, incx, y2, incy);
  EXPECT_EQ(status, HCBLAS_STATUS_SUCCESS);

  // HCBLAS_STATUS_INVALID_VALUE
  // incx is 0
  status = hcblasSetVector(handle, n, sizeof(float), x1, 0, y1, incy);
  EXPECT_EQ(status, HCBLAS_STATUS_INVALID_VALUE);
  // incy is 0
  status = hcblasSetVector(handle, n, sizeof(float), x1, incx, y1, 0);
  EXPECT_EQ(status, HCBLAS_STATUS_INVALID_VALUE);
  // elemSize is 0
  status = hcblasSetVector(handle, n, 0, x1, incx, y1, incy);
  EXPECT_EQ(status, HCBLAS_STATUS_INVALID_VALUE);

  // HCBLAS_STATUS_MAPPING_ERROR
  status = hcblasSetVector(handle, n, sizeof(double), x1, incx, y1, incy);
  EXPECT_EQ(status, HCBLAS_STATUS_MAPPING_ERROR);

  // HCBLAS_STATUS_NOT_INITIALIZED
  hcblasDestroy(&handle);
  status = hcblasSetVector(handle, n, sizeof(float), x1, incx, y1, incy);
  EXPECT_EQ(status, HCBLAS_STATUS_NOT_INITIALIZED);

  free(x1);
  free(x2);
  hc::am_free(y1);
  hc::am_free(y2);
}

TEST(hcblasGetVectorTest, return_Check_hcblasGetVector) {
  int n = 10;
  int incx = 1, incy = 1;
  float *y1 = (float *)calloc(n, sizeof(float));
  double *y2 = (double *)calloc(n, sizeof(double));
  hcblasStatus_t status;
  hcblasHandle_t handle = NULL;
  hc::accelerator default_acc;
  hc::accelerator_view av = default_acc.get_default_view();
  // Passing a Null handle and default accelerator to the API
  status = hcblasCreate(&handle, &av);
  float *x1 = (float *)am_alloc(n * sizeof(float), handle->currentAccl, 0);
  double *x2 = (double *)am_alloc(n * sizeof(double), handle->currentAccl, 0);

  // HCBLAS_STATUS_SUCCESS
  // float type memory transfer from device to host
  status = hcblasGetVector(handle, n, sizeof(float), x1, incx, y1, incy);
  EXPECT_EQ(status, HCBLAS_STATUS_SUCCESS);
  // double type memory transfer from device to host
  status = hcblasGetVector(handle, n, sizeof(double), x2, incx, y2, incy);
  EXPECT_EQ(status, HCBLAS_STATUS_SUCCESS);

  // HCBLAS_STATUS_INVALID_VALUE
  // incx is 0
  status = hcblasGetVector(handle, n, sizeof(float), x1, 0, y1, incy);
  EXPECT_EQ(status, HCBLAS_STATUS_INVALID_VALUE);
  // incy is 0
  status = hcblasGetVector(handle, n, sizeof(float), x1, incx, y1, 0);
  EXPECT_EQ(status, HCBLAS_STATUS_INVALID_VALUE);
  // elemSize is 0
  status = hcblasGetVector(handle, n, 0, x1, incx, y1, incy);
  EXPECT_EQ(status, HCBLAS_STATUS_INVALID_VALUE);

  // HCBLAS_STATUS_MAPPING_ERROR
  status = hcblasGetVector(handle, n, sizeof(double), x1, incx, y1, incy);
  EXPECT_EQ(status, HCBLAS_STATUS_MAPPING_ERROR);

  // HCBLAS_STATUS_NOT_INITIALIZED
  hcblasDestroy(&handle);
  status = hcblasGetVector(handle, n, sizeof(float), x1, incx, y1, incy);
  EXPECT_EQ(status, HCBLAS_STATUS_NOT_INITIALIZED);

  free(y1);
  free(y2);
  hc::am_free(x1);
  hc::am_free(x2);
}

TEST(hcblasSetMatrixTest, return_Check_hcblasSetMatrix) {
  int rows = 10;
  int cols = 10;
  int lda = 10, ldb = 10;
  float *x1 = (float *)calloc(rows * cols, sizeof(float));
  double *x2 = (double *)calloc(rows * cols, sizeof(double));
  hcblasStatus_t status;
  hcblasHandle_t handle = NULL;
  hc::accelerator default_acc;
  hc::accelerator_view av = default_acc.get_default_view();
  // Passing a Null handle and default accelerator to the API
  status = hcblasCreate(&handle, &av);
  float *y1 =
      (float *)am_alloc(rows * cols * sizeof(float), handle->currentAccl, 0);
  double *y2 =
      (double *)am_alloc(rows * cols * sizeof(double), handle->currentAccl, 0);

  // HCBLAS_STATUS_SUCCESS
  // float type memory transfer from host to device
  status = hcblasSetMatrix(handle, rows, cols, sizeof(float), x1, lda, y1, ldb);
  EXPECT_EQ(status, HCBLAS_STATUS_SUCCESS);
  // double type memory transfer from host to device
  status =
      hcblasSetMatrix(handle, rows, cols, sizeof(double), x2, lda, y2, ldb);
  EXPECT_EQ(status, HCBLAS_STATUS_SUCCESS);

  // HCBLAS_STATUS_INVALID_VALUE
  // lda is 0
  status = hcblasSetMatrix(handle, rows, cols, sizeof(float), x1, 0, y1, ldb);
  EXPECT_EQ(status, HCBLAS_STATUS_INVALID_VALUE);
  // ldb is 0
  status = hcblasSetMatrix(handle, rows, cols, sizeof(float), x1, lda, y1, 0);
  EXPECT_EQ(status, HCBLAS_STATUS_INVALID_VALUE);
  // elemSize is 0
  status = hcblasSetMatrix(handle, rows, cols, 0, x1, lda, y1, ldb);
  EXPECT_EQ(status, HCBLAS_STATUS_INVALID_VALUE);

  // HCBLAS_STATUS_MAPPING_ERROR
  status =
      hcblasSetMatrix(handle, rows, cols, sizeof(double), x1, lda, y1, ldb);
  EXPECT_EQ(status, HCBLAS_STATUS_MAPPING_ERROR);

  // HCBLAS_STATUS_NOT_INITIALIZED
  hcblasDestroy(&handle);
  status = hcblasSetMatrix(handle, rows, cols, sizeof(float), x1, lda, y1, ldb);
  EXPECT_EQ(status, HCBLAS_STATUS_NOT_INITIALIZED);

  free(x1);
  free(x2);
  hc::am_free(y1);
  hc::am_free(y2);
}

TEST(hcblasGetMatrixTest, return_Check_hcblasGetMatrix) {
  int rows = 10;
  int cols = 10;
  int lda = 10, ldb = 10;
  float *y1 = (float *)calloc(cols * rows, sizeof(float));
  double *y2 = (double *)calloc(cols * rows, sizeof(double));
  hcblasStatus_t status;
  hcblasHandle_t handle = NULL;
  hc::accelerator default_acc;
  hc::accelerator_view av = default_acc.get_default_view();
  // Passing a Null handle and default accelerator to the API
  status = hcblasCreate(&handle, &av);
  float *x1 =
      (float *)am_alloc(rows * cols * sizeof(float), handle->currentAccl, 0);
  double *x2 =
      (double *)am_alloc(rows * cols * sizeof(double), handle->currentAccl, 0);

  // HCBLAS_STATUS_SUCCESS
  // float type memory transfer from device to host
  status = hcblasGetMatrix(handle, rows, cols, sizeof(float), x1, lda, y1, ldb);
  EXPECT_EQ(status, HCBLAS_STATUS_SUCCESS);
  // double type memory transfer from device to host
  status =
      hcblasGetMatrix(handle, rows, cols, sizeof(double), x2, lda, y2, ldb);
  EXPECT_EQ(status, HCBLAS_STATUS_SUCCESS);

  // HCBLAS_STATUS_INVALID_VALUE
  // lda is 0
  status = hcblasGetMatrix(handle, rows, cols, sizeof(float), x1, 0, y1, ldb);
  EXPECT_EQ(status, HCBLAS_STATUS_INVALID_VALUE);
  // ldb is 0
  status = hcblasGetMatrix(handle, rows, cols, sizeof(float), x1, lda, y1, 0);
  EXPECT_EQ(status, HCBLAS_STATUS_INVALID_VALUE);
  // elemSize is 0
  status = hcblasGetMatrix(handle, rows, cols, 0, x1, lda, y1, ldb);
  EXPECT_EQ(status, HCBLAS_STATUS_INVALID_VALUE);

  // HCBLAS_STATUS_MAPPING_ERROR
  status =
      hcblasGetMatrix(handle, rows, cols, sizeof(double), x1, lda, y1, ldb);
  EXPECT_EQ(status, HCBLAS_STATUS_MAPPING_ERROR);

  // HCBLAS_STATUS_NOT_INITIALIZED
  hcblasDestroy(&handle);
  status = hcblasGetMatrix(handle, rows, cols, sizeof(float), x1, lda, y1, ldb);
  EXPECT_EQ(status, HCBLAS_STATUS_NOT_INITIALIZED);

  free(y1);
  free(y2);
  hc::am_free(x1);
  hc::am_free(x2);
}

