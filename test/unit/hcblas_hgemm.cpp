#include "hcblaslib.h"
#include <cstdlib>
#include "helper_functions.h"
#include "gtest/gtest.h"
#include "hc_am.hpp"
#include "cblas.h"
#include "test_constants.h"

#ifdef HGEMM_TEST_ON

void cblas_hgemm( int,int, int,int, int, int, __half, __half* ,long, __half* , long, __half ,__half*,long  );


void cblas_hgemm( int order ,int transA, int transB, int M, int N, int K, __half alpha  , __half* A, long lda, __half* B,long ldb,  __half beta , __half* C_cblas,long ldc)
{
  if(order == 102 && alpha!= 0)
  {
    if( transA == 111 && transB == 111 )
    {
      for( int i = 0 ; i < M ; i++)
      {
         for( int j = 0 ; j < N ; j++)
         {
          __half temp=0;
             for( int k = 0 ; k < K ; k++)
             { 
                temp += alpha * A[k * M + i] * B[j * K + k]; //CM N,N
             }
             C_cblas[j * M + i] = temp + beta * C_cblas[j * M + i];
         }
      }
    }
    else if ( transA == 112 && transB == 111 )
    {
      for( int i = 0 ; i < M ; i++)
      {
         for( int j = 0 ; j < N ; j++)
         {
         __half temp=0;
             for( int k = 0 ; k < K ; k++)
             {
                temp += alpha * A[k + i * K] * B[j * K + k];
             }
             C_cblas[j * M + i] = temp + beta * C_cblas[j * M + i] ; //CM T,N
         }
      }
    }
    else if ( transA == 111 && transB == 112 )
    {
      for( int i = 0 ; i < M ; i++)
      {
         for( int j = 0 ; j < N ; j++)
         {
         __half temp=0;
             for( int k = 0 ; k < K ; k++)
             {
                temp +=   alpha * A[k * M + i] * B[j + N * k] ; //CM N,T
             }
             C_cblas[j * M + i] = temp + beta * C_cblas[j * M + i] ;
         }
      }
    }
    else if (transA == 112 && transB == 112)
    {
      for( int i = 0 ; i < M ; i++)
      {
         for( int j = 0 ; j < N ; j++)
         {
         __half temp=0;
             for( int k = 0 ; k < K ; k++)
             {
                temp +=  alpha *  A[k + i * K] * B[j + N * k]  ; //CM T,T
             }
             C_cblas[j * M + i] = temp + beta * C_cblas[j * M + i] ;
         }
      }
    }
    else
      cout << "\nERROR !!!";
  }
  else if(order == 101 && alpha!= 0 )
  {
    if( transA == 111 && transB == 111 )
    {
      for( int i = 0 ; i < M ; i++)
      {
         for( int j = 0 ; j < N ; j++)
         {
         __half temp=0;
             for( int k = 0 ; k < K ; k++)
             {
                temp +=  alpha * A[k + i * K] * B[j + k * N] ;  //RM N,N
             }
             C_cblas[j * M + i] = temp + beta * C_cblas[j * M + i] ;
         }
      }
    }
    else if ( transA == 112 && transB == 111 )
    {
      for( int i = 0 ; i < M ; i++)
      {
         for( int j = 0 ; j < N ; j++)
         {
         __half temp=0;
             for( int k = 0 ; k < K ; k++)
             {
               temp +=  alpha * A[k * M + i] * B[j + k * N] ; // RM T,N
             }
             C_cblas[j * M + i] = temp + beta * C_cblas[j * M + i] ;
         }
      }
    }
    else if ( transA == 111 && transB == 112 )
    {
      for( int i = 0 ; i < M ; i++)
      {
         for( int j = 0 ; j < N ; j++)
         {
         __half temp=0;
             for( int k = 0 ; k < K ; k++)
             {
                temp += alpha *  A[k + i * K] * B[j * K + k] ; //RM N,T
             }
             C_cblas[j * M + i] = temp + beta * C_cblas[j * M + i] ;
         }
      }
    }
    else if (transA == 112 && transB == 112)
    {
      for( int i = 0 ; i < M ; i++)
      {
         for( int j = 0 ; j < N ; j++)
         {
         __half temp=0;
             for( int k = 0 ; k < K ; k++)
             {
                temp +=  alpha * A[k * M + i] * B[j * K + k]  ; // RM T,T
             }
             C_cblas[j * M + i] = temp + beta * C_cblas[j * M + i] ;
         }
      }
    }
   }
    else
    {
      if (order == 102 && alpha == 0 )
      {
        for( int i = 0 ; i < M ; i++)
      {
         for( int j = 0 ; j < N ; j++)
         {
         __half temp=0;
             for( int k = 0 ; k < K ; k++)
             {
             temp = 0 ;
             }
             C_cblas[j * M + i] = beta * C_cblas[j * M + i] ;
         }
      }
      }
      else
        cout << "\nERROR !!!";
  }
}

TEST(hcblas_hgemm, return_correct_hgemm_Implementation_type_1) {
  hc::accelerator accl;
  hc::accelerator_view av = accl.get_default_view();
  Hcblaslibrary hc(&av);
  int M = 189, N = 9, K = 19;
  __half alpha = 1, beta = 1;
  long lda, ldb, ldc;
  int incX = 1, incY = 1;
  long aOffset = 0;
  long bOffset = 0;
  long cOffset = 0;
  hcblasOrder hcOrder;
  hcblasTranspose typeA, typeB;
  hcblasStatus status;
  accelerator_view accl_view = hc.currentAcclView;
  accelerator acc = hc.currentAccl;
  // Implementation type I - Inputs and Outputs are HCC device pointers
  __half* A = (__half*) calloc(M * K, sizeof(__half));
  __half* B = (__half*) calloc(K * N, sizeof(__half));
  __half* C = (__half*) calloc(M * N, sizeof(__half));
  __half* devA = hc::am_alloc(sizeof(__half) * M * K, acc, 0);
  __half* devB = hc::am_alloc(sizeof(__half) * K * N, acc, 0);
  __half* devC = hc::am_alloc(sizeof(__half) * M * N, acc, 0);

  for(int i = 0; i < M * K; i++) {
    A[i] = rand() % 100;
  }

  for(int i = 0; i < K * N; i++) {
    B[i] = rand() % 15;
  }

  for(int i = 0; i < M * N; i++) {
    C[i] = rand() % 25;
  }

  accl_view.copy(A, devA, M * K * sizeof(__half));
  accl_view.copy(B, devB, K * N * sizeof(__half));
  accl_view.copy(C, devC, M * N * sizeof(__half));
  // NoTransA and NoTransB
  typeA = NoTrans;
  typeB = NoTrans;
  // Column major
  lda = M;
  ldb = K ;
  ldc = M;
  status = hc.hcblas_hgemm(accl_view, ColMajor, typeA, typeB, M, N, K, alpha, devA, lda, devB, ldb, beta, devC, ldc, aOffset, bOffset, cOffset);
  EXPECT_EQ(status, HCBLAS_SUCCEEDS);
  // Row Major
  lda = K;
  ldb = N ;
  ldc = N;
  status = hc.hcblas_hgemm(accl_view, RowMajor, typeA, typeB, M, N, K, alpha, devA, lda, devB, ldb, beta, devC, ldc, aOffset, bOffset, cOffset);
  EXPECT_EQ(status, HCBLAS_SUCCEEDS);
  // NoTransA TransB
  typeA = NoTrans;
  typeB = Trans;
  // Column major
  lda = M;
  ldb = N ;
  ldc = M;
  status = hc.hcblas_hgemm(accl_view, ColMajor, typeA, typeB, M, N, K, alpha, devA, lda, devB, ldb, beta, devC, ldc, aOffset, bOffset, cOffset);
  EXPECT_EQ(status, HCBLAS_SUCCEEDS);
  // Row Major
  lda = K;
  ldb = K ;
  ldc = N;
  status = hc.hcblas_hgemm(accl_view, RowMajor, typeA, typeB, M, N, K, alpha, devA, lda, devB, ldb, beta, devC, ldc, aOffset, bOffset, cOffset);
  EXPECT_EQ(status, HCBLAS_SUCCEEDS);
  // TransA NoTransB
  typeA = Trans;
  typeB = NoTrans;
  // Column major
  lda = K;
  ldb = K ;
  ldc = M;
  status = hc.hcblas_hgemm(accl_view, ColMajor, typeA, typeB, M, N, K, alpha, devA, lda, devB, ldb, beta, devC, ldc, aOffset, bOffset, cOffset);
  EXPECT_EQ(status, HCBLAS_SUCCEEDS);
  // Row Major
  lda = M;
  ldb = N ;
  ldc = N;
  status = hc.hcblas_hgemm(accl_view, RowMajor, typeA, typeB, M, N, K, alpha, devA, lda, devB, ldb, beta, devC, ldc, aOffset, bOffset, cOffset);
  EXPECT_EQ(status, HCBLAS_SUCCEEDS);
  // TransA TransB
  typeA = Trans;
  typeB = Trans;
  // Column major
  lda = K;
  ldb = N ;
  ldc = M;
  status = hc.hcblas_hgemm(accl_view, ColMajor, typeA, typeB, M, N, K, alpha, devA, lda, devB, ldb, beta, devC, ldc, aOffset, bOffset, cOffset);
  EXPECT_EQ(status, HCBLAS_SUCCEEDS);
  // Row Major
  lda = M;
  ldb = K ;
  ldc = N;
  status = hc.hcblas_hgemm(accl_view, RowMajor, typeA, typeB, M, N, K, alpha, devA, lda, devB, ldb, beta, devC, ldc, aOffset, bOffset, cOffset);
  EXPECT_EQ(status, HCBLAS_SUCCEEDS);
  typeA = NoTrans;
  typeB = NoTrans;
  lda = M;
  ldb = K ;
  ldc = M;
  hcOrder = ColMajor;
  __half* devA1 = NULL;
  __half* devB1 = NULL;
  __half* devC1 = NULL;
  //A, B, C device pointers are not allocated properly
  status = hc.hcblas_hgemm(accl_view, hcOrder, typeA, typeB, M, N, K, alpha, devA1, lda, devB, ldb, beta, devC, ldc, aOffset, bOffset, cOffset);
  EXPECT_EQ(status, HCBLAS_INVALID);
  status = hc.hcblas_hgemm(accl_view, hcOrder, typeA, typeB, M, N, K, alpha, devA, lda, devB1, ldb, beta, devC, ldc, aOffset, bOffset, cOffset);
  EXPECT_EQ(status, HCBLAS_INVALID);
  status = hc.hcblas_hgemm(accl_view, hcOrder, typeA, typeB, M, N, K, alpha, devA, lda, devB, ldb, beta, devC1, ldc, aOffset, bOffset, cOffset);
  EXPECT_EQ(status, HCBLAS_INVALID);
  // M is 0
  status = hc.hcblas_hgemm(accl_view, hcOrder, typeA, typeB, 0, N, K, alpha, devA, lda, devB, ldb, beta, devC, ldc, aOffset, bOffset, cOffset);
  EXPECT_EQ(status, HCBLAS_INVALID);
   // N is 0
  status = hc.hcblas_hgemm(accl_view, hcOrder, typeA, typeB, M, 0, K, alpha, devA, lda, devB, ldb, beta, devC, ldc, aOffset, bOffset, cOffset);
  EXPECT_EQ(status, HCBLAS_INVALID);
  // K is 0
  status = hc.hcblas_hgemm(accl_view, hcOrder, typeA, typeB, M, N, 0, alpha, devA, lda, devB, ldb, beta, devC, ldc, aOffset, bOffset, cOffset);
  EXPECT_EQ(status, HCBLAS_INVALID);
  free(A);
  free(B);
  free(C);
  hc::am_free(devA);
  hc::am_free(devB);
  hc::am_free(devC);
}

// Function to check Hgemm NoTransAB Column Major
void func_check_hgemmNN_Col_type_1(int M, int N, int K, __half alpha, __half beta, float tolerance) {
  hc::accelerator accl;
  hc::accelerator_view av = accl.get_default_view();
  Hcblaslibrary hc(&av);
  long lda, ldb, ldc;
  int incX = 1, incY = 1;
  long aOffset = 0;
  long bOffset = 0;
  long cOffset = 0;
  hcblasOrder hcOrder;
  hcblasTranspose typeA, typeB;
  hcblasStatus status;
  accelerator_view accl_view = hc.currentAcclView;
  accelerator acc = hc.currentAccl;
  CBLAS_TRANSPOSE Transa, Transb;
  
  // Implementation type I - Inputs and Outputs are HCC device pointers
  __half* A = (__half*) calloc(M * K, sizeof(__half));
  __half* B = (__half*) calloc(K * N, sizeof(__half));
  __half* C = (__half*) calloc(M * N, sizeof(__half));
  __half* C_hcblas = (__half*) calloc(M * N, sizeof(__half));
  __half* C_cblas = (__half*) calloc(M * N, sizeof(__half));
  __half* devA = hc::am_alloc(sizeof(__half) * M * K, acc, 0);
  __half* devB = hc::am_alloc(sizeof(__half) * K * N, acc, 0);
  __half* devC = hc::am_alloc(sizeof(__half) * M * N, acc, 0);
  float* C_hcblas_float = (float*) calloc(M * N, sizeof(float));
  float* C_cblas_float = (float*) calloc(M * N, sizeof(float));
  float X = 2;
  for(int i = 0; i < M * K; i++) {
    A[i] = __hc_float2half (static_cast <float> (rand()) / (static_cast <float> (RAND_MAX/X)));
   }

  for(int i = 0; i < K * N; i++) {
    B[i] = __hc_float2half (static_cast <float> (rand()) / (static_cast <float> (RAND_MAX/X)));
   }

  for(int i = 0; i < M * N; i++) {
    C[i] = __hc_float2half (static_cast <float> (rand()) / (static_cast <float> (RAND_MAX/X)));
     C_cblas[i] = C[i];
  }
   accl_view.copy(A, devA, M * K * sizeof(__half));
  accl_view.copy(B, devB, K * N * sizeof(__half));
  accl_view.copy(C, devC, M * N * sizeof(__half));
  
  // NoTransA and NoTransB
  typeA = NoTrans;
  typeB = NoTrans;
  Transa = CblasNoTrans;
  Transb = CblasNoTrans;
  
  // Column major
  lda = M;
  ldb = K ;
  ldc = M;
  status = hc.hcblas_hgemm(accl_view, ColMajor, typeA, typeB, M, N, K, alpha, devA, lda, devB, ldb, beta, devC, ldc, aOffset, bOffset, cOffset);
  EXPECT_EQ(status, HCBLAS_SUCCEEDS);
  accl_view.copy(devC, C_hcblas, M * N * sizeof(__half));
  cblas_hgemm( CblasColMajor, Transa, Transb, M, N, K, alpha, A,lda, B,ldb, beta, C_cblas,ldc );
  float result = hgemmCompareL2fe(C_cblas, C_hcblas, M*N, tolerance);
  EXPECT_LE(result, tolerance);
  
  free(A);
  free(B);
  free(C);
  free(C_cblas);
  free(C_hcblas);
  hc::am_free(devA);
  hc::am_free(devB);
  hc::am_free(devC);
}

// Function to check Hgemm NoTransAB row Major
void func_check_hgemmNN_Row_type_1(int M, int N, int K, __half alpha, __half beta, float tolerance) {
  hc::accelerator accl;
  hc::accelerator_view av = accl.get_default_view();
  Hcblaslibrary hc(&av);
  long lda, ldb, ldc;
  int incX = 1, incY = 1;
  long aOffset = 0;
  long bOffset = 0;
  long cOffset = 0;
  hcblasOrder hcOrder;
  hcblasTranspose typeA, typeB;
  hcblasStatus status;
  accelerator_view accl_view = hc.currentAcclView;
  accelerator acc = hc.currentAccl;
  CBLAS_TRANSPOSE Transa, Transb;
  // Implementation type I - Inputs and Outputs are HCC device pointers
  __half* A = (__half*) calloc(M * K, sizeof(__half));
  __half* B = (__half*) calloc(K * N, sizeof(__half));
  __half* C = (__half*) calloc(M * N, sizeof(__half));
  __half* C_hcblas = (__half*) calloc(M * N, sizeof(__half));
  __half* C_cblas = (__half*) calloc(M * N, sizeof(__half));
  __half* devA = hc::am_alloc(sizeof(__half) * M * K, acc, 0);
  __half* devB = hc::am_alloc(sizeof(__half) * K * N, acc, 0);
  __half* devC = hc::am_alloc(sizeof(__half) * M * N, acc, 0);
  float X = 2;

  for(int i = 0; i < M * K; i++) {
    A[i] = __hc_float2half (static_cast <float> (rand()) / (static_cast <float> (RAND_MAX/X)));
  }

  for(int i = 0; i < K * N; i++) {
    B[i] = __hc_float2half (static_cast <float> (rand()) / (static_cast <float> (RAND_MAX/X)));
  }

  for(int i = 0; i < M * N; i++) {
    C[i] = __hc_float2half (static_cast <float> (rand()) / (static_cast <float> (RAND_MAX/X)));
    C_cblas[i] = C[i];
  }

  accl_view.copy(A, devA, M * K * sizeof(__half));
  accl_view.copy(B, devB, K * N * sizeof(__half));
  accl_view.copy(C, devC, M * N * sizeof(__half));
  // NoTransA and NoTransB
  typeA = NoTrans;
  typeB = NoTrans;
  Transa = CblasNoTrans;
  Transb = CblasNoTrans;

  // Row Major
  lda = K;
  ldb = N ;
  ldc = N;
  status = hc.hcblas_hgemm(accl_view, RowMajor, typeA, typeB, M, N, K, alpha, devA, lda, devB, ldb, beta, devC, ldc, aOffset, bOffset, cOffset);
  EXPECT_EQ(status, HCBLAS_SUCCEEDS);
  accl_view.copy(devC, C_hcblas, M * N * sizeof(__half));
  cblas_hgemm( CblasRowMajor, Transa, Transb, M, N, K, alpha, A, lda, B, ldb, beta, C_cblas, ldc);

  float result = hgemmCompareL2fe(C_cblas, C_hcblas, M*N, tolerance);
  EXPECT_LE(result, tolerance);

  free(A);
  free(B);
  free(C);
  free(C_cblas);
  free(C_hcblas);
  hc::am_free(devA);
  hc::am_free(devB);
  hc::am_free(devC);
}



// Function to check Hgemm NoTransA Col Major
void func_check_hgemmNT_Col_type_1(int M, int N, int K, __half alpha, __half beta, float tolerance) {
  hc::accelerator accl;
  hc::accelerator_view av = accl.get_default_view();
  Hcblaslibrary hc(&av);
  long lda, ldb, ldc;
  int incX = 1, incY = 1;
  long aOffset = 0;
  long bOffset = 0;
  long cOffset = 0;
  hcblasOrder hcOrder;
  hcblasTranspose typeA, typeB;
  hcblasStatus status;
  accelerator_view accl_view = hc.currentAcclView;
  accelerator acc = hc.currentAccl;
  CBLAS_TRANSPOSE Transa, Transb;
  // Implementation type I - Inputs and Outputs are HCC device pointers
  __half* A = (__half*) calloc(M * K, sizeof(__half));
  __half* B = (__half*) calloc(K * N, sizeof(__half));
  __half* C = (__half*) calloc(M * N, sizeof(__half));
  __half* C_hcblas = (__half*) calloc(M * N, sizeof(__half));
  __half* C_cblas = (__half*) calloc(M * N, sizeof(__half));
  __half* devA = hc::am_alloc(sizeof(__half) * M * K, acc, 0);
  __half* devB = hc::am_alloc(sizeof(__half) * K * N, acc, 0);
  __half* devC = hc::am_alloc(sizeof(__half) * M * N, acc, 0);
  float X = 2;
   for(int i = 0; i < M * K; i++) {
    A[i] = __hc_float2half (static_cast <float> (rand()) / (static_cast <float> (RAND_MAX/X)));
  }

  for(int i = 0; i < K * N; i++) {
    B[i] = __hc_float2half (static_cast <float> (rand()) / (static_cast <float> (RAND_MAX/X)));
  }

  for(int i = 0; i < M * N; i++) {
    C[i] = __hc_float2half (static_cast <float> (rand()) / (static_cast <float> (RAND_MAX/X)));
    C_cblas[i] = C[i];
  }
 
  accl_view.copy(A, devA, M * K * sizeof(__half));
  accl_view.copy(B, devB, K * N * sizeof(__half));
  accl_view.copy(C, devC, M * N * sizeof(__half));

  // NoTransA TransB
  typeA = NoTrans;
  typeB = Trans;
  Transa = CblasNoTrans;
  Transb = CblasTrans;
  // Column major
  lda = M;
  ldb = N ;
  ldc = M;
  status = hc.hcblas_hgemm(accl_view, ColMajor, typeA, typeB, M, N, K, alpha, devA, lda, devB, ldb, beta, devC, ldc, aOffset, bOffset, cOffset);
  EXPECT_EQ(status, HCBLAS_SUCCEEDS);
  accl_view.copy(devC, C_hcblas, M * N * sizeof(__half));
  cblas_hgemm( CblasColMajor, Transa, Transb, M, N, K, alpha, A, lda, B, ldb, beta, C_cblas, ldc);

  float result = hgemmCompareL2fe(C_cblas, C_hcblas, M*N, tolerance);
  EXPECT_LE(result, tolerance);

  lda = M;
  ldb = N ;
  ldc = M;
  status = hc.hcblas_hgemm(accl_view, ColMajor, typeA, typeB, M, N, K, 0, devA, lda, devB, ldb, beta, devC, ldc, aOffset, bOffset, cOffset);
  EXPECT_EQ(status, HCBLAS_SUCCEEDS);
  accl_view.copy(devC, C_hcblas, M * N * sizeof(__half));
  cblas_hgemm( CblasColMajor, Transa, Transb, M, N, K, 0, A, lda, B, ldb, beta, C_cblas, ldc);

  result = hgemmCompareL2fe(C_cblas, C_hcblas, M*N, tolerance);
  EXPECT_LE(result, tolerance);
  
  lda = M;
  ldb = N ;
  ldc = M;
  status = hc.hcblas_hgemm(accl_view, ColMajor, typeA, typeB, M, N, K, 0, devA, lda, devB, ldb, 0, devC, ldc, aOffset, bOffset, cOffset);
  EXPECT_EQ(status, HCBLAS_SUCCEEDS);
  accl_view.copy(devC, C_hcblas, M * N * sizeof(__half));
  cblas_hgemm( CblasColMajor, Transa, Transb, M, N, K, 0, A, lda, B, ldb, 0, C_cblas, ldc);

  result = hgemmCompareL2fe(C_cblas, C_hcblas, M*N, tolerance);
  EXPECT_LE(result, tolerance);

  free(A);
  free(B);
  free(C);
  free(C_cblas);
  free(C_hcblas);
  hc::am_free(devA);
  hc::am_free(devB);
  hc::am_free(devC);
}


// Function to check Hgemm NoTransA Row Major
void func_check_hgemmNT_Row_type_1(int M, int N, int K, __half alpha, __half beta, float tolerance) {
  hc::accelerator accl;
  hc::accelerator_view av = accl.get_default_view();
  Hcblaslibrary hc(&av);
  long lda, ldb, ldc;
  int incX = 1, incY = 1;
  long aOffset = 0;
  long bOffset = 0;
  long cOffset = 0;
  hcblasOrder hcOrder;
  hcblasTranspose typeA, typeB;
  hcblasStatus status;
  accelerator_view accl_view = hc.currentAcclView;
  accelerator acc = hc.currentAccl;
  CBLAS_TRANSPOSE Transa, Transb;
  // Implementation type I - Inputs and Outputs are HCC device pointers
  __half* A = (__half*) calloc(M * K, sizeof(__half));
  __half* B = (__half*) calloc(K * N, sizeof(__half));
  __half* C = (__half*) calloc(M * N, sizeof(__half));
  __half* C_hcblas = (__half*) calloc(M * N, sizeof(__half));
  __half* C_cblas = (__half*) calloc(M * N, sizeof(__half));
  __half* devA = hc::am_alloc(sizeof(__half) * M * K, acc, 0);
  __half* devB = hc::am_alloc(sizeof(__half) * K * N, acc, 0);
  __half* devC = hc::am_alloc(sizeof(__half) * M * N, acc, 0);
  float X = 2;

  for(int i = 0; i < M * K; i++) {
    A[i] = __hc_float2half (static_cast <float> (rand()) / (static_cast <float> (RAND_MAX/X)));
  }

  for(int i = 0; i < K * N; i++) {
    B[i] = __hc_float2half (static_cast <float> (rand()) / (static_cast <float> (RAND_MAX/X)));
  }

  for(int i = 0; i < M * N; i++) {
    C[i] = __hc_float2half (static_cast <float> (rand()) / (static_cast <float> (RAND_MAX/X)));
    C_cblas[i] = C[i];
  }

  accl_view.copy(A, devA, M * K * sizeof(__half));
  accl_view.copy(B, devB, K * N * sizeof(__half));
  accl_view.copy(C, devC, M * N * sizeof(__half));

  // NoTransA TransB
  typeA = NoTrans;
  typeB = Trans;
  Transa = CblasNoTrans;
  Transb = CblasTrans;
  
  // Row Major
  lda = K;
  ldb = K ;
  ldc = N;
  status = hc.hcblas_hgemm(accl_view, RowMajor, typeA, typeB, M, N, K, alpha, devA, lda, devB, ldb, beta, devC, ldc, aOffset, bOffset, cOffset);
  EXPECT_EQ(status, HCBLAS_SUCCEEDS);
  accl_view.copy(devC, C_hcblas, M * N * sizeof(__half));
  cblas_hgemm(CblasRowMajor, Transa, Transb, M, N, K, alpha, A, lda, B, ldb, beta, C_cblas, ldc);

  float result = hgemmCompareL2fe(C_cblas, C_hcblas, M*N, tolerance);
  EXPECT_LE(result, tolerance);

  // alpha and beta are zeroes
  lda = K;
  ldb = K ;
  ldc = N;
  status = hc.hcblas_hgemm(accl_view, RowMajor, typeA, typeB, M, N, K, 0, devA, lda, devB, ldb, beta, devC, ldc, aOffset, bOffset, cOffset);
  EXPECT_EQ(status, HCBLAS_SUCCEEDS);
  accl_view.copy(devC, C_hcblas, M * N * sizeof(__half));
  cblas_hgemm(CblasRowMajor, Transa, Transb, M, N, K, 0, A, lda, B, ldb, beta, C_cblas, ldc);

   result = hgemmCompareL2fe(C_cblas, C_hcblas, M*N, tolerance);
  EXPECT_LE(result, tolerance);

  // alpha = 0, beta = 0
  lda = K;
  ldb = K ;
  ldc = N;
  status = hc.hcblas_hgemm(accl_view, RowMajor, typeA, typeB, M, N, K, 0, devA, lda, devB, ldb, 0, devC, ldc, aOffset, bOffset, cOffset);
  EXPECT_EQ(status, HCBLAS_SUCCEEDS);
  accl_view.copy(devC, C_hcblas, M * N * sizeof(__half));
  cblas_hgemm(CblasRowMajor, Transa, Transb, M, N, K, 0, A, lda, B, ldb, 0, C_cblas, ldc);

  result = hgemmCompareL2fe(C_cblas, C_hcblas, M*N, tolerance);
  EXPECT_LE(result, tolerance);

  free(A);
  free(B);
  free(C);
  free(C_cblas);
  free(C_hcblas);
  hc::am_free(devA);
  hc::am_free(devB);
  hc::am_free(devC);
}

// Function to check Hgemm NoTransB Col Major
void func_check_hgemmTN_Col_type_1(int M, int N, int K, __half alpha, __half beta, float tolerance) {
  hc::accelerator accl;
  hc::accelerator_view av = accl.get_default_view();
  Hcblaslibrary hc(&av);
  long lda, ldb, ldc;
  int incX = 1, incY = 1;
  long aOffset = 0;
  long bOffset = 0;
  long cOffset = 0;
  hcblasOrder hcOrder;
  hcblasTranspose typeA, typeB;
  hcblasStatus status;
  accelerator_view accl_view = hc.currentAcclView;
  accelerator acc = hc.currentAccl;
  CBLAS_TRANSPOSE Transa, Transb;
  // Implementation type I - Inputs and Outputs are HCC device pointers
  __half* A = (__half*) calloc(M * K, sizeof(__half));
  __half* B = (__half*) calloc(K * N, sizeof(__half));
  __half* C = (__half*) calloc(M * N, sizeof(__half));
  __half* C_hcblas = (__half*) calloc(M * N, sizeof(__half));
  __half* C_cblas = (__half*) calloc(M * N, sizeof(__half));
  __half* devA = hc::am_alloc(sizeof(__half) * M * K, acc, 0);
  __half* devB = hc::am_alloc(sizeof(__half) * K * N, acc, 0);
  __half* devC = hc::am_alloc(sizeof(__half) * M * N, acc, 0);
  float X = 2;

  for(int i = 0; i < M * K; i++) {
    A[i] = __hc_float2half (static_cast <float> (rand()) / (static_cast <float> (RAND_MAX/X)));
  }

  for(int i = 0; i < K * N; i++) {
    B[i] = __hc_float2half (static_cast <float> (rand()) / (static_cast <float> (RAND_MAX/X)));
  }

  for(int i = 0; i < M * N; i++) {
    C[i] = __hc_float2half (static_cast <float> (rand()) / (static_cast <float> (RAND_MAX/X)));
    C_cblas[i] = C[i];
  }

  accl_view.copy(A, devA, M * K * sizeof(__half));
  accl_view.copy(B, devB, K * N * sizeof(__half));
  accl_view.copy(C, devC, M * N * sizeof(__half));
  
  // TransA NoTransB
  typeA = Trans;
  typeB = NoTrans;
  Transa = CblasTrans;
  Transb = CblasNoTrans;
  // Column major
  lda = K;
  ldb = K ;
  ldc = M;
  status = hc.hcblas_hgemm(accl_view, ColMajor, typeA, typeB, M, N, K, alpha, devA, lda, devB, ldb, beta, devC, ldc, aOffset, bOffset, cOffset);
  EXPECT_EQ(status, HCBLAS_SUCCEEDS);
  accl_view.copy(devC, C_hcblas, M * N * sizeof(__half));
  cblas_hgemm( CblasColMajor, Transa, Transb, M, N, K, alpha, A, lda, B, ldb, beta, C_cblas, ldc);

 float result = hgemmCompareL2fe(C_cblas, C_hcblas, M*N, tolerance);
  EXPECT_LE(result, tolerance);

  free(A);
  free(B);
  free(C);
  free(C_cblas);
  free(C_hcblas);
  hc::am_free(devA);
  hc::am_free(devB);
  hc::am_free(devC);
}


// Function to check Hgemm NoTransB Row Major
void func_check_hgemmTN_Row_type_1(int M, int N, int K, __half alpha, __half beta, float tolerance) {
  hc::accelerator accl;
  hc::accelerator_view av = accl.get_default_view();
  Hcblaslibrary hc(&av);
  long lda, ldb, ldc;
  int incX = 1, incY = 1;
  long aOffset = 0;
  long bOffset = 0;
  long cOffset = 0;
  hcblasOrder hcOrder;
  hcblasTranspose typeA, typeB;
  hcblasStatus status;
  accelerator_view accl_view = hc.currentAcclView;
  accelerator acc = hc.currentAccl;
  CBLAS_TRANSPOSE Transa, Transb;
  // Implementation type I - Inputs and Outputs are HCC device pointers
  __half* A = (__half*) calloc(M * K, sizeof(__half));
  __half* B = (__half*) calloc(K * N, sizeof(__half));
  __half* C = (__half*) calloc(M * N, sizeof(__half));
  __half* C_hcblas = (__half*) calloc(M * N, sizeof(__half));
  __half* C_cblas = (__half*) calloc(M * N, sizeof(__half));
  __half* devA = hc::am_alloc(sizeof(__half) * M * K, acc, 0);
  __half* devB = hc::am_alloc(sizeof(__half) * K * N, acc, 0);
  __half* devC = hc::am_alloc(sizeof(__half) * M * N, acc, 0);
  float X = 2;

  for(int i = 0; i < M * K; i++) {
    A[i] = __hc_float2half (static_cast <float> (rand()) / (static_cast <float> (RAND_MAX/X)));
  }

  for(int i = 0; i < K * N; i++) {
    B[i] = __hc_float2half (static_cast <float> (rand()) / (static_cast <float> (RAND_MAX/X)));
  }

  for(int i = 0; i < M * N; i++) {
    C[i] = __hc_float2half (static_cast <float> (rand()) / (static_cast <float> (RAND_MAX/X)));
    C_cblas[i] = C[i];
  }

  accl_view.copy(A, devA, M * K * sizeof(__half));
  accl_view.copy(B, devB, K * N * sizeof(__half));
  accl_view.copy(C, devC, M * N * sizeof(__half));
  
  // TransA NoTransB
  typeA = Trans;
  typeB = NoTrans;
  Transa = CblasTrans;
  Transb = CblasNoTrans;

  // Row Major
  lda = M;
  ldb = N ;
  ldc = N;
  status = hc.hcblas_hgemm(accl_view, RowMajor, typeA, typeB, M, N, K, alpha, devA, lda, devB, ldb, beta, devC, ldc, aOffset, bOffset, cOffset);
  EXPECT_EQ(status, HCBLAS_SUCCEEDS);
  accl_view.copy(devC, C_hcblas, M * N * sizeof(__half));
  cblas_hgemm( CblasRowMajor, Transa, Transb, M, N, K, alpha, A, lda, B, ldb, beta, C_cblas, ldc);

  for(int i = 0 ; i < M * N ; i++)
    EXPECT_EQ(C_hcblas[i], C_cblas[i]);

  free(A);
  free(B);
  free(C);
  free(C_cblas);
  free(C_hcblas);
  hc::am_free(devA);
  hc::am_free(devB);
  hc::am_free(devC);
}

// Function to check Hgemm TransAB Col Major
void func_check_hgemmTT_Col_type_1(int M, int N, int K, __half alpha, __half beta, float tolerance) {
  hc::accelerator accl;
  hc::accelerator_view av = accl.get_default_view();
  Hcblaslibrary hc(&av);
  long lda, ldb, ldc;
  int incX = 1, incY = 1;
  long aOffset = 0;
  long bOffset = 0;
  long cOffset = 0;
  hcblasOrder hcOrder;
  hcblasTranspose typeA, typeB;
  hcblasStatus status;
  accelerator_view accl_view = hc.currentAcclView;
  accelerator acc = hc.currentAccl;
  CBLAS_TRANSPOSE Transa, Transb;
  // Implementation type I - Inputs and Outputs are HCC device pointers
  __half* A = (__half*) calloc(M * K, sizeof(__half));
  __half* B = (__half*) calloc(K * N, sizeof(__half));
  __half* C = (__half*) calloc(M * N, sizeof(__half));
  __half* C_hcblas = (__half*) calloc(M * N, sizeof(__half));
  __half* C_cblas = (__half*) calloc(M * N, sizeof(__half));
  __half* devA = hc::am_alloc(sizeof(__half) * M * K, acc, 0);
  __half* devB = hc::am_alloc(sizeof(__half) * K * N, acc, 0);
  __half* devC = hc::am_alloc(sizeof(__half) * M * N, acc, 0);
  float X = 2;

  for(int i = 0; i < M * K; i++) {
    A[i] = __hc_float2half (static_cast <float> (rand()) / (static_cast <float> (RAND_MAX/X)));
  }

  for(int i = 0; i < K * N; i++) {
    B[i] = __hc_float2half (static_cast <float> (rand()) / (static_cast <float> (RAND_MAX/X)));
  }

  for(int i = 0; i < M * N; i++) {
    C[i] = __hc_float2half (static_cast <float> (rand()) / (static_cast <float> (RAND_MAX/X)));
    C_cblas[i] = C[i];
  }

  accl_view.copy(A, devA, M * K * sizeof(__half));
  accl_view.copy(B, devB, K * N * sizeof(__half));
  accl_view.copy(C, devC, M * N * sizeof(__half));

  // TransA TransB
  typeA = Trans;
  typeB = Trans;
  Transa = CblasTrans;
  Transb = CblasTrans;

  
  // Column major 
  lda = K;
  ldb = N ;
  ldc = M;
  status = hc.hcblas_hgemm(accl_view, ColMajor, typeA, typeB, M, N, K, alpha, devA, lda, devB, ldb, beta, devC, ldc, aOffset, bOffset, cOffset);
  EXPECT_EQ(status, HCBLAS_SUCCEEDS);
  accl_view.copy(devC, C_hcblas, M * N * sizeof(__half));
  cblas_hgemm( CblasColMajor, Transa, Transb, M, N, K, alpha, A, lda, B, ldb, beta, C_cblas, ldc);

  for(int i = 0 ; i < M * N ; i++)
    EXPECT_EQ(C_hcblas[i], C_cblas[i]);

  free(A);
  free(B);
  free(C);
  free(C_cblas);
  free(C_hcblas);
  hc::am_free(devA);
  hc::am_free(devB);
  hc::am_free(devC);
}

// Function to check Hgemm TransAB Row Major
void func_check_hgemmTT_Row_type_1(int M, int N, int K, __half alpha, __half beta, float tolerance) {
  hc::accelerator accl;
  hc::accelerator_view av = accl.get_default_view();
  Hcblaslibrary hc(&av);
  
  long lda, ldb, ldc;
  int incX = 1, incY = 1;
  long aOffset = 0;
  long bOffset = 0;
  long cOffset = 0;
  hcblasOrder hcOrder;
  hcblasTranspose typeA, typeB;
  hcblasStatus status;
  accelerator_view accl_view = hc.currentAcclView;
  accelerator acc = hc.currentAccl;
  CBLAS_TRANSPOSE Transa, Transb;
  // Implementation type I - Inputs and Outputs are HCC device pointers
  __half* A = (__half*) calloc(M * K, sizeof(__half));
  __half* B = (__half*) calloc(K * N, sizeof(__half));
  __half* C = (__half*) calloc(M * N, sizeof(__half));
  __half* C_hcblas = (__half*) calloc(M * N, sizeof(__half));
  __half* C_cblas = (__half*) calloc(M * N, sizeof(__half));
  __half* devA = hc::am_alloc(sizeof(__half) * M * K, acc, 0);
  __half* devB = hc::am_alloc(sizeof(__half) * K * N, acc, 0);
  __half* devC = hc::am_alloc(sizeof(__half) * M * N, acc, 0);
  float X = 2;

  for(int i = 0; i < M * K; i++) {
    A[i] = __hc_float2half (static_cast <float> (rand()) / (static_cast <float> (RAND_MAX/X)));
  }

  for(int i = 0; i < K * N; i++) {
    B[i] = __hc_float2half (static_cast <float> (rand()) / (static_cast <float> (RAND_MAX/X)));
  }

  for(int i = 0; i < M * N; i++) {
    C[i] = __hc_float2half (static_cast <float> (rand()) / (static_cast <float> (RAND_MAX/X)));
    C_cblas[i] = C[i];
  }

  accl_view.copy(A, devA, M * K * sizeof(__half));
  accl_view.copy(B, devB, K * N * sizeof(__half));
  accl_view.copy(C, devC, M * N * sizeof(__half));

  // TransA TransB
  typeA = Trans;
  typeB = Trans;
  Transa = CblasTrans;
  Transb = CblasTrans;

  // Row Major
  lda = M;
  ldb = K ;
  ldc = N;
  status = hc.hcblas_hgemm(accl_view, RowMajor, typeA, typeB, M, N, K, alpha, devA, lda, devB, ldb, beta, devC, ldc, aOffset, bOffset, cOffset);
  EXPECT_EQ(status, HCBLAS_SUCCEEDS);
  accl_view.copy(devC, C_hcblas, M * N * sizeof(__half));
  cblas_hgemm( CblasRowMajor, Transa, Transb, M, N, K, alpha, A, lda, B, ldb, beta, C_cblas, ldc);

  float result = hgemmCompareL2fe(C_cblas, C_hcblas, M*N, tolerance);
  EXPECT_LE(result, tolerance);

  free(A);
  free(B);
  free(C);
  free(C_cblas);
  free(C_hcblas);
  hc::am_free(devA);
  hc::am_free(devB);
  hc::am_free(devC);
}

// Case A:  Square Cases tests
// Order : Column 

// Type NoTransAB
// check square matrices of VVSmall input sizes
TEST(hcblas_hgemm, func_correct_hgemmNN_Col_square_vvsmall_Implementation_type_1) {
 int M, N, K;
 M = N = K = gen_vvsmall();
 __half alpha = __hc_float2half((float)rand()/(float)(RAND_MAX) * 1.172); 
 __half beta = __hc_float2half((float)rand()/(float)(RAND_MAX) * 3.414);
 func_check_hgemmNN_Col_type_1(M, N, K, alpha, beta, 1.0e-5f);
}

// check square matrices of VSmall input sizes
TEST(hcblas_hgemm, func_correct_hgemmNN_Col_square_vsmall_Implementation_type_1) {
 int M, N, K;
 M = N = K = gen_vsmall();
 __half alpha = __hc_float2half((float)rand()/(float)(RAND_MAX) * 1.172);
 __half beta = __hc_float2half((float)rand()/(float)(RAND_MAX) * 3.414);
 func_check_hgemmNN_Col_type_1(M, N, K, alpha, beta, 1.0e-5f);
}

// check square matrices of small input sizes
TEST(hcblas_hgemm, func_correct_hgemmNN_Col_square_small_Implementation_type_1) {
 int M, N, K;
 M = N = K = gen_small();
 __half alpha = __hc_float2half((float)rand()/(float)(RAND_MAX) * 1.172); 
 __half beta =__hc_float2half((float)rand()/(float)(RAND_MAX) * 3.414);
 func_check_hgemmNN_Col_type_1(M, N, K, alpha, beta, 1.0e-5f);
}


// check square matrices of regular input sizes
TEST(hcblas_hgemm, func_correct_hgemmNN_Col_square_regular_Implementation_type_1) {
 int M, N, K;
 M = N = K = gen_regular();
 __half alpha = __hc_float2half((float)rand()/(float)(RAND_MAX) * 1.172); 
 __half beta = __hc_float2half((float)rand()/(float)(RAND_MAX) * 3.414); 
 func_check_hgemmNN_Col_type_1(M, N, K, alpha, beta, 1.0e-5f);
}

// Type NoTransA
// check square matrices of VVSmall input sizes
TEST(hcblas_hgemm, func_correct_hgemmNT_Col_square_vvsmall_Implementation_type_1) {
 int M, N, K;
 M = N = K = gen_vvsmall();
 __half alpha = __hc_float2half((float)rand()/(float)(RAND_MAX) * 1.172);
 __half beta = __hc_float2half((float)rand()/(float)(RAND_MAX) * 3.414);
 func_check_hgemmNT_Col_type_1(M, N, K, alpha, beta, 1.0e-5f);
}

// check square matrices of VSmall input sizes
TEST(hcblas_hgemm, func_correct_hgemmNT_Col_square_vsmall_Implementation_type_1) {
 int M, N, K;
 M = N = K = gen_vsmall();
 __half alpha = __hc_float2half((float)rand()/(float)(RAND_MAX) * 1.172);
 __half beta = __hc_float2half((float)rand()/(float)(RAND_MAX) * 3.414);
 func_check_hgemmNT_Col_type_1(M, N, K, alpha, beta, 1.0e-5f);
}

// check square matrices of small input sizes
TEST(hcblas_hgemm, func_correct_hgemmNT_Col_square_small_Implementation_type_1) {
 int M, N, K;
 M = N = K = gen_small();
 __half alpha = __hc_float2half((float)rand()/(float)(RAND_MAX) * 1.172);
 __half beta = __hc_float2half((float)rand()/(float)(RAND_MAX) * 3.414);
 func_check_hgemmNT_Col_type_1(M, N, K, alpha, beta, 1.0e-5f);
}


// check square matrices of regular input sizes
TEST(hcblas_hgemm, func_correct_hgemmNT_Col_square_regular_Implementation_type_1) {
 int M, N, K;
 M = N = K = gen_regular();
 __half alpha = __hc_float2half((float)rand()/(float)(RAND_MAX) * 1.172);
 __half beta = __hc_float2half((float)rand()/(float)(RAND_MAX) * 3.414);
 func_check_hgemmNT_Col_type_1(M, N, K, alpha, beta, 1.0e-5f);

}

//TODO: tasks longtime uncomment when needed to be tested
// check square matrices of large  input sizes
/*TEST(hcblas_hgemm, func_correct_hgemmNT_Col_square_large_Implementation_type_1) {
 int M, N, K;
 M = N = K = gen_large();
  
 __half alpha = __hc_float2half((float)rand()/(float)(RAND_MAX) * 1.172);
 __half beta =  __hc_float2half((float)rand()/(float)(RAND_MAX) * 3.414); 
 func_check_hgemmNT_Col_type_1(M, N, K, alpha, beta, 1.0e-5f);
}*/

// Type NoTransB
// check square matrices of VVSmall input sizes
TEST(hcblas_hgemm, func_correct_hgemmTN_Col_square_vvsmall_Implementation_type_1) {
 int M, N, K;
 M = N = K = gen_vvsmall();
 __half alpha = __hc_float2half((float)rand()/(float)(RAND_MAX) * 1.172);
 __half beta =  __hc_float2half((float)rand()/(float)(RAND_MAX) * 3.414);
 func_check_hgemmTN_Col_type_1(M, N, K, alpha, beta, 1.0e-5f);
}

// check square matrices of VSmall input sizes
TEST(hcblas_hgemm, func_correct_hgemmTN_Col_square_vsmall_Implementation_type_1) {
 int M, N, K;
 M = N = K = gen_vsmall();
 __half alpha = __hc_float2half((float)rand()/(float)(RAND_MAX) * 1.172);
 __half beta =  __hc_float2half((float)rand()/(float)(RAND_MAX) * 3.414);
 func_check_hgemmTN_Col_type_1(M, N, K, alpha, beta, 1.0e-5f);
}

// check square matrices of small input sizes
TEST(hcblas_hgemm, func_correct_hgemmTN_Col_square_small_Implementation_type_1) {
 int M, N, K;
 M = N = K = gen_small();
 __half alpha = __hc_float2half((float)rand()/(float)(RAND_MAX) * 1.172);
 __half beta =  __hc_float2half((float)rand()/(float)(RAND_MAX) * 3.414); 
 func_check_hgemmTN_Col_type_1(M, N, K, alpha, beta, 1.0e-5f);
}


// check square matrices of regular input sizes
TEST(hcblas_hgemm, func_correct_hgemmTN_Col_square_regular_Implementation_type_1) {
 int M, N, K;
 M = N = K = gen_regular();
 __half alpha =__hc_float2half((float)rand()/(float)(RAND_MAX) * 1.172); 
 __half beta =  __hc_float2half((float)rand()/(float)(RAND_MAX) * 3.414);
 func_check_hgemmTN_Col_type_1(M, N, K, alpha, beta, 1.0e-5f);
}

//TODO: tasks longtime uncomment when needed to be tested
// check square matrices of large  input sizes
/*TEST(hcblas_hgemm, func_correct_hgemmTN_Col_square_large_Implementation_type_1) {
 int M, N, K;
 M = N = K = gen_large();
 __half alpha = __hc_float2half((float)rand()/(float)(RAND_MAX) * 1.172);
 __half beta = __hc_float2half((float)rand()/(float)(RAND_MAX) * 3.414); 
 func_check_hgemmTN_Col_type_1(M, N, K, alpha, beta, 1.0e-5f);
}*/


// Type TransAB
// check square matrices of VVSmall input sizes
TEST(hcblas_hgemm, func_correct_hgemmTT_Col_square_vvsmall_Implementation_type_1) {
 int M, N, K;
 M = N = K = gen_vvsmall();
 __half alpha = __hc_float2half((float)rand()/(float)(RAND_MAX) * 1.172);
 __half beta = __hc_float2half((float)rand()/(float)(RAND_MAX) * 3.414); 
 func_check_hgemmTT_Col_type_1(M, N, K, alpha, beta, 1.0e-5f);
}

// check square matrices of VSmall input sizes
TEST(hcblas_hgemm, func_correct_hgemmTT_Col_square_vsmall_Implementation_type_1) {
 int M, N, K;
 M = N = K = gen_vsmall();
 __half alpha = __hc_float2half((float)rand()/(float)(RAND_MAX) * 1.172);
 __half beta =  __hc_float2half((float)rand()/(float)(RAND_MAX) * 3.414);
 func_check_hgemmTT_Col_type_1(M, N, K, alpha, beta, 1.0e-5f);
}

//TODO:check mismatch result
// check square matrices of small input sizes
/*TEST(hcblas_hgemm, func_correct_hgemmTT_Col_square_small_Implementation_type_1) {
 int M, N, K;
 M = N = K = gen_small();
 __half alpha = __hc_float2half((float)rand()/(float)(RAND_MAX) * 1.172);
 __half beta =  __hc_float2half((float)rand()/(float)(RAND_MAX) * 3.414); 
 func_check_hgemmTT_Col_type_1(M, N, K, alpha, beta, 1.0e-5f);
}*/

// check square matrices of regular input sizes
TEST(hcblas_hgemm, func_correct_hgemmTT_Col_square_regular_Implementation_type_1) {
 int M, N, K;
 M = N = K = gen_regular();
 __half alpha = __hc_float2half((float)rand()/(float)(RAND_MAX) * 1.172);
 __half beta = __hc_float2half((float)rand()/(float)(RAND_MAX) * 3.414);
 func_check_hgemmTT_Col_type_1(M, N, K, alpha, beta, 1.0e-5f);
}

// CASE 2: Slim A Fat B
// HGEMM NN Case

// check slim A with large M and Vsmall K
TEST(hcblas_hgemm, func_correct_hgemmNN_Col_slimA_vsmallK_Implementation_type_1) {
 int M, N, K;
 M = N = gen_vlarge();
 K = gen_vsmall();
 __half alpha = __hc_float2half((float)rand()/(float)(RAND_MAX) * 1.172);
 __half beta = __hc_float2half((float)rand()/(float)(RAND_MAX) * 3.414); 
 func_check_hgemmNN_Col_type_1(M, N, K, alpha, beta, 1.0e-5f);
}

// check slim A with large M and small K
TEST(hcblas_hgemm, func_correct_hgemmNN_Col_slimA_smallK_Implementation_type_1) {
 int M, N, K;
 M = N = gen_vlarge();
 K = gen_small();
 __half alpha =__hc_float2half((float)rand()/(float)(RAND_MAX) * 1.172); 
 __half beta = __hc_float2half((float)rand()/(float)(RAND_MAX) * 3.414);
 func_check_hgemmNN_Col_type_1(M, N, K, alpha, beta, 1.0e-5f);
}

// HGEMM NT Case

// check slim A with large M and Vsmall K
TEST(hcblas_hgemm, func_correct_hgemmNT_Col_slimA_vsmallK_Implementation_type_1) {
 int M, N, K;
 M = N = gen_vlarge();
 K = gen_vsmall();
 __half alpha = __hc_float2half((float)rand()/(float)(RAND_MAX) * 1.172);
 __half beta = __hc_float2half((float)rand()/(float)(RAND_MAX) * 3.414);
 func_check_hgemmNT_Col_type_1(M, N, K, alpha, beta, 1.0e-5f);
}

// check slim A with large M and small K
TEST(hcblas_hgemm, func_correct_hgemmNT_Col_slimA_smallK_Implementation_type_1) {
 int M, N, K;
 M = N = gen_vlarge();
 K = gen_small();
 __half alpha = __hc_float2half((float)rand()/(float)(RAND_MAX) * 1.172);
 __half beta = __hc_float2half((float)rand()/(float)(RAND_MAX) * 3.414);
 func_check_hgemmNT_Col_type_1(M, N, K, alpha, beta, 1.0e-5f);
}

// HGEMM TN Case

// check slim A with large M and Vsmall K
TEST(hcblas_hgemm, func_correct_hgemmTN_Col_slimA_vsmallK_Implementation_type_1) {
 int M, N, K;
 M = N = gen_vlarge();
 K = gen_vsmall();
 __half alpha = __hc_float2half((float)rand()/(float)(RAND_MAX) * 1.172);
 __half beta = __hc_float2half((float)rand()/(float)(RAND_MAX) * 3.414);
 func_check_hgemmTN_Col_type_1(M, N, K, alpha, beta, 1.0e-5f);
}

// check slim A with large M and small K
TEST(hcblas_hgemm, func_correct_hgemmTN_Col_slimA_smallK_Implementation_type_1) {
 int M, N, K;
 M = N = gen_vlarge();
 K = gen_small();
 __half alpha = __hc_float2half((float)rand()/(float)(RAND_MAX) * 1.172);
 __half beta = __hc_float2half((float)rand()/(float)(RAND_MAX) * 3.414);
 func_check_hgemmTN_Col_type_1(M, N, K, alpha, beta, 1.0e-5f);
}

// HGEMM TT Case

// check slim A with large M and Vsmall K
TEST(hcblas_hgemm, func_correct_hgemmTT_Col_slimA_vsmallK_Implementation_type_1) {
 int M, N, K;
 M = N = gen_vlarge();
 K = gen_vsmall();
 __half alpha = __hc_float2half((float)rand()/(float)(RAND_MAX) * 1.172);
 __half beta = __hc_float2half((float)rand()/(float)(RAND_MAX) * 3.414);
 func_check_hgemmTT_Col_type_1(M, N, K, alpha, beta, 1.0e-5f);
}

// check slim A with large M and small K
TEST(hcblas_hgemm, func_correct_hgemmTT_Col_slimA_smallK_Implementation_type_1) {
 int M, N, K;
 M = N = gen_vlarge();
 K = gen_small();
 __half alpha = __hc_float2half((float)rand()/(float)(RAND_MAX) * 1.172);
 __half beta = __hc_float2half((float)rand()/(float)(RAND_MAX) * 3.414);
 func_check_hgemmTT_Col_type_1(M, N, K, alpha, beta, 1.0e-5f);
}

// check slim A with large M and regular K
TEST(hcblas_hgemm, func_correct_hgemmTT_Col_slimA_regularK_Implementation_type_1) {
 int M, N, K;
 M = N = gen_vlarge();
 K = gen_regular();
 __half alpha = __hc_float2half((float)rand()/(float)(RAND_MAX) * 1.172);
 __half beta = __hc_float2half((float)rand()/(float)(RAND_MAX) * 3.414);
 func_check_hgemmTT_Col_type_1(M, N, K, alpha, beta, 1.0e-5f);
}


// CASE 2: Slim C Fat B Square A
// HGEMM NN Case

// check slim C with large M and Vsmall N
TEST(hcblas_hgemm, func_correct_hgemmNN_Col_slimC_vsmallN_Implementation_type_1) {
 int M, N, K;
 M = K = gen_vlarge();
 N = gen_vsmall();
 __half alpha = __hc_float2half((float)rand()/(float)(RAND_MAX) * 1.172);
 __half beta = __hc_float2half((float)rand()/(float)(RAND_MAX) * 3.414);
 func_check_hgemmNN_Col_type_1(M, N, K, alpha, beta, 1.0e-5f);
}

// check slim C with large M and small N
TEST(hcblas_hgemm, func_correct_hgemmNN_Col_slimC_smallN_Implementation_type_1) {
 int M, N, K;
 M = K = gen_vlarge();
 N = gen_small();
 __half alpha = __hc_float2half((float)rand()/(float)(RAND_MAX) * 1.172);
 __half beta = __hc_float2half((float)rand()/(float)(RAND_MAX) * 3.414);
 func_check_hgemmNN_Col_type_1(M, N, K, alpha, beta, 1.0e-5f);
}

// check slim C with large M and regular N
TEST(hcblas_hgemm, func_correct_hgemmNN_Col_slimC_regularN_Implementation_type_1) {
 int M, N, K;
 M = K = gen_vlarge();
 N = gen_regular();
 __half alpha = __hc_float2half((float)rand()/(float)(RAND_MAX) * 1.172);
 __half beta = __hc_float2half((float)rand()/(float)(RAND_MAX) * 3.414);
 func_check_hgemmNN_Col_type_1(M, N, K, alpha, beta, 1.0e-5f);
}

// HGEMM NT Case

// check slim C with large M and small N
TEST(hcblas_hgemm, func_correct_hgemmNT_Col_slimC_smallN_Implementation_type_1) {
 int M, N, K;
 M = K = gen_vlarge();
 N = gen_small();
 __half alpha = __hc_float2half((float)rand()/(float)(RAND_MAX) * 1.172);
 __half beta = __hc_float2half((float)rand()/(float)(RAND_MAX) * 3.414);
 func_check_hgemmNT_Col_type_1(M, N, K, alpha, beta, 1.0e-5f);
}

// check slim C with large M and regular N
TEST(hcblas_hgemm, func_correct_hgemmNT_Col_slimC_regularN_Implementation_type_1) {
 int M, N, K;
 M = K = gen_vlarge();
 N = gen_regular();
 __half alpha = __hc_float2half((float)rand()/(float)(RAND_MAX) * 1.172);
 __half beta = __hc_float2half((float)rand()/(float)(RAND_MAX) * 3.414);
 func_check_hgemmNT_Col_type_1(M, N, K, alpha, beta, 1.0e-5f);
}

// HGEMM TN Case
// check slim C with large M and Vsmall N
TEST(hcblas_hgemm, func_correct_hgemmTN_Col_slimC_vsmallN_Implementation_type_1) {
 int M, N, K;
 M = K = gen_vlarge();
 N = gen_vsmall();
 __half alpha = __hc_float2half((float)rand()/(float)(RAND_MAX) * 1.172);
 __half beta = __hc_float2half((float)rand()/(float)(RAND_MAX) * 3.414); 
 func_check_hgemmTN_Col_type_1(M, N, K, alpha, beta, 1.0e-5f);
}

// check slim C with large M and small N
TEST(hcblas_hgemm, func_correct_hgemmTN_Col_slimC_smallN_Implementation_type_1) {
 int M, N, K;
 M = K = gen_vlarge();
 N = gen_small();
 __half alpha = __hc_float2half((float)rand()/(float)(RAND_MAX) * 1.172);
 __half beta = __hc_float2half((float)rand()/(float)(RAND_MAX) * 3.414);
 func_check_hgemmTN_Col_type_1(M, N, K, alpha, beta, 1.0e-5f);
}

// check slim C with large M and regular N
TEST(hcblas_hgemm, func_correct_hgemmTN_Col_slimC_regularN_Implementation_type_1) {
 int M, N, K;
 M = K = gen_vlarge();
 N = gen_regular();
 __half alpha = __hc_float2half((float)rand()/(float)(RAND_MAX) * 1.172);
 __half beta = __hc_float2half((float)rand()/(float)(RAND_MAX) * 3.414);
 func_check_hgemmTN_Col_type_1(M, N, K, alpha, beta, 1.0e-5f);
}
// HGEMM TT Case

// check slim C with large M and small N
TEST(hcblas_hgemm, func_correct_hgemmTT_Col_slimC_smallN_Implementation_type_1) {
 int M, N, K;
 M = K = gen_vlarge();
 N = gen_small();
 __half alpha = __hc_float2half((float)rand()/(float)(RAND_MAX) * 1.172);
 __half beta = __hc_float2half((float)rand()/(float)(RAND_MAX) * 3.414);
 func_check_hgemmTT_Col_type_1(M, N, K, alpha, beta, 1.0e-5f);
}

// check slim C with large M and regular N
TEST(hcblas_hgemm, func_correct_hgemmTT_Col_slimC_regularN_Implementation_type_1) {
 int M, N, K;
 M = K = gen_vlarge();
 N = gen_regular();
 __half alpha = __hc_float2half((float)rand()/(float)(RAND_MAX) * 1.172);
 __half beta = __hc_float2half((float)rand()/(float)(RAND_MAX) * 3.414);
 func_check_hgemmTT_Col_type_1(M, N, K, alpha, beta, 1.0e-5f);
}

#endif
