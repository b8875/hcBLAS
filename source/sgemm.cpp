#include "ampblaslib.h"
#include <amp.h>
#include <amp_math.h>
using namespace Concurrency;

#define REGISTER 0
#define REGISTER_EXTN 1
#define STEP 0
#define SUBMICROTILE 0

#if SUBMICROTILE
#define NOTRANSAB 0
#define NOTRANSA 1
#define NOTRANSB 0
#define TRANSAB 0
#endif

#if REGISTER_EXTN
#define TSM 16 // tile-size in M
#define TSN 16 // tile-size in N
#define TSK 16 // tile-size in K
#define WPTM 2 // work-per-thread in M
#define WPTN 2 // work-per-thread in  N
#define RTSM (TSM/WPTM) // reduced tile-size in M
#define RTSN (TSN/WPTN) // reduced tile-size in N
#define LPTA ((TSK*TSM)/(RTSM*RTSN)) // Loads-per-thread for A
#define LPTB ((TSK*TSN)/(RTSM*RTSN)) // Loads-per-thread for B
#endif

#define THREADS    16
#define GEMM_BLOCK 256
#define TILE_DIM  16

#define TILE_SZ_A 32
#define TILE_SZ_B 16
#define TILE_SZ_RATIO (TILE_SZ_A/TILE_SZ_B)
#define TILESIZE 16
#define STEPSIZE 128 
#define MICROTILESIZE 2

#define  M1x1(offset)			\
            rA[0][0] = lA[offA + 0];	\
            rB[0][0] = lB[offB + 0];	\
            offA += offset;			\
            offB += offset;			\
            rC[0][0]=rA[0][0] *rB[0][0] + rC[0][0]; \

#define  MS1x1(offsetA, offsetB)			\
            for(int iter = 0; iter < STEPSIZE/TILESIZE; ++iter) \
            {\
              rA[0][iter] = lA[offA + (TILESIZE * TILESIZE) * iter];	\
              rB[0][iter] = lB[offB + (TILESIZE * TILESIZE) * iter];	\
              rC[0][0] +=rA[0][iter] *rB[0][iter]; \
            }\
            offA += offsetA;			\
            offB += offsetB;			\

#define  MTS                                                                \
           for(int iter = 0; iter < MICROTILESIZE ; iter++)                 \
           {                                                                \
             rA[0][iter] = lA[offA + (iter * TILESIZE)];                    \
             rB[0][iter] = lB[offB + (iter * TILESIZE)];                    \
           }                                                                \
           for(int rowIndex = 0; rowIndex < MICROTILESIZE ; rowIndex++)     \
           {                                                                \
           for(int colIndex = 0; colIndex < MICROTILESIZE ; colIndex++)     \
           {                                                                \
           rC[rowIndex][colIndex] = rA[0][rowIndex] * rB[0][colIndex] +     \
                                    rC[rowIndex][colIndex];                 \
           }                                                                \
           }                                                                \
           offA += (MICROTILESIZE * TILESIZE);                              \
           offB += (MICROTILESIZE * TILESIZE);                              \

#if REGISTER_EXTN

static void gemm_NoTransA_extend(Concurrency::array_view<float, 1> &A, long aOffset,
                                Concurrency::array_view<float, 1> &B, long bOffset,
                                Concurrency::array_view<float, 1> &C, long cOffset,
                                int M, int N, int K, int lda, int ldb, int ldc,
                                float alpha, float beta)
{
    Concurrency::extent<2> grdExt((((M - 1)/WPTM + 1) + (RTSM -1)) & ~(RTSM - 1), (((N -1) /WPTN + 1) + (RTSN - 1)) & ~(RTSN - 1));//((M - 1) / WPTM + 1) * WPTM, ((N - 1) / WPTN + 1) *WPTN);
    Concurrency::tiled_extent < RTSM, RTSN > t_ext(grdExt);

    Concurrency::parallel_for_each(t_ext,
                                   [=] (Concurrency::tiled_index<RTSM,RTSN> tidx)
                                   restrict(amp) {

    // Thread identifiers
    const int tidm = tidx.local[0]; // Local row ID (max: TSM/WPTM)
    const int tidn = tidx.local[1]; // Local col ID (max: TSN/WPTN)
    const int offsetM = TSM*tidx.tile[0]; // Work-group offset
    const int offsetN = TSN*tidx.tile[1]; // Work-group offset

    // Local memory to fit a tile of A and B
    tile_static float Asub[TSK][TSM];
    tile_static float Bsub[TSN][TSK+2];

    // Allocate register space
    float Areg;
    float Breg[WPTN];
    float acc[WPTM][WPTN] = {{(float)0.0}};

    // Loop over all tiles
    int numTiles = (K - 1)/TSK + 1 ;
    for (int t=0; t<numTiles; t++) {

        // Load one tile of A and B into local memory
        for (int la=0; la<LPTA; la++) {
            int tid = tidn*RTSM + tidm;
            int id = la*RTSN*RTSM + tid;
            int row = id % TSM;
            int col = id / TSM;
            int tiledIndex = TSK*t + col;
            if ( tiledIndex < K && offsetM +row < M) {
                Asub[col][row] = A[aOffset + tiledIndex * M + offsetM + row];
            }
            else Asub[col][row] = 0.0;
            if ( tiledIndex < K && offsetN +row < N) {
               Bsub[row][col] = B[bOffset + tiledIndex * N + offsetN + row];
            }
            else Bsub[row][col] = 0.0;
        }

        // Synchronise to make sure the tile is loaded
        tidx.barrier.wait();

        // Loop over the values of a single tile
        for (int k=0; k<TSK; k++) {

            // Cache the values of Bsub in registers
            for (int wn=0; wn<WPTN; wn++) {
                int col = tidn + wn*RTSN;
                Breg[wn] = Bsub[col][k];
            }

            // Perform the computation
            for (int wm=0; wm<WPTM; wm++) {
                int row = tidm + wm*RTSM;
                Areg = Asub[k][row];
                for (int wn=0; wn<WPTN; wn++) {
                    acc[wm][wn] += Areg * Breg[wn];
                }
            }
        }

        // Synchronise before loading the next tile
        tidx.barrier.wait();
     }

     // Store the final results in C
     for (int wm=0; wm<WPTM; wm++) {
        int globalRow = offsetM + tidm + wm*RTSM;
        for (int wn=0; wn<WPTN; wn++) {
            int globalCol = offsetN + tidn + wn*RTSN;
            if (  globalCol < N  && globalRow < M ) {
                C[cOffset + globalCol * M + globalRow ] = acc[wm][wn];
            }
        }
    }

});

}


#endif

#if REGISTER

static void gemm_NoTransAB(Concurrency::array_view<float, 1> &A, long aOffset,
                          Concurrency::array_view<float, 1> &B, long bOffset,
                          Concurrency::array_view<float, 1> &C, long cOffset,
                          int M, int N, int K, int lda, int ldb, int ldc,
                          float alpha, float beta)
{
    Concurrency::extent<2> grdExt(((M - 1) / TILE_SZ_A + 1) * TILE_SZ_A, (N - 1) / TILE_SZ_B + 1);
    Concurrency::tiled_extent <TILE_SZ_A, 1> t_ext(grdExt);
    Concurrency::parallel_for_each(t_ext,
                                   [=] (Concurrency::tiled_index<TILE_SZ_A,1> tidx)
                                   restrict(amp) {

    // Shared memory for tiling input B array
    tile_static float B_s [TILE_SZ_RATIO][TILE_SZ_B];

    // Macros for accessing flattened matrices
    #define A(row,col) A[(row) + (col) * M]
    #define B(row,col) B[(row) + (col) * K]
    #define C(row,col) C[(row) + (col) * M]

    // Index variables
    const unsigned int row = tidx.global[0];
    const unsigned int col = tidx.tile[1] * TILE_SZ_B;

    // Privatization of output variables
    float c_reg[TILE_SZ_B] = {(float)0};

    // Loop over the input tiles
    for(unsigned int tileIdx = 0; tileIdx < (K - 1)/TILE_SZ_RATIO + 1; ++tileIdx) {
        // Load the tile of B into shared memory
        const unsigned int i = tidx.local[0]/TILE_SZ_B;
        const unsigned int j = tidx.local[0]%TILE_SZ_B;

        if (tileIdx*TILE_SZ_RATIO + i < K && col + j < N) {
            B_s[i][j] = B(tileIdx*TILE_SZ_RATIO + i, col + j);
        }
        else {
            B_s[i][j] = 0;
        }
        tidx.barrier.wait();

        // Loop over elements inside the tile
        for (unsigned int idx = 0; idx < TILE_SZ_RATIO; ++idx) {
            // Load tile of A matrix into register
            float a_reg;
            if(row < M && tileIdx*TILE_SZ_RATIO + idx < K) {
                a_reg = A(row, tileIdx*TILE_SZ_RATIO + idx);
            }
            else {
                a_reg = 0;
            }

            // Loop over and update the output elements assigned to the thread
            for(unsigned int outIdx = 0; outIdx < TILE_SZ_B; ++outIdx) {
                c_reg[outIdx] += a_reg*B_s[idx][outIdx];
            }
        }
        tidx.barrier.wait();
    }
    for (unsigned int outIdx = 0; outIdx < TILE_SZ_B; ++outIdx) {
        if (row < M && col + outIdx < N) {
            (C(row, col + outIdx) *= beta) += (c_reg[outIdx] *= alpha);
        }
    }
});
   

}


static void gemm_NoTransA(Concurrency::array_view<float, 1> &A, long aOffset,
                          Concurrency::array_view<float, 1> &B, long bOffset,
                          Concurrency::array_view<float, 1> &C, long cOffset,
                          int M, int N, int K, int lda, int ldb, int ldc,
                          float alpha, float beta)
{
    Concurrency::extent<2> grdExt(((M - 1) / TILE_SZ_A + 1) * TILE_SZ_A, (N - 1) / TILE_SZ_B + 1);
    Concurrency::tiled_extent <TILE_SZ_A, 1> t_ext(grdExt);
    Concurrency::parallel_for_each(t_ext,
                                   [=] (Concurrency::tiled_index<TILE_SZ_A,1> tidx)
                                   restrict(amp) {

    // Shared memory for tiling input B array
    tile_static float B_s [TILE_SZ_RATIO][TILE_SZ_B];

    // Macros for accessing flattened matrices
    #define A(row,col) A[(row) + (col) * M]
    #define B(row,col) B[(row) * N + (col)]
    #define C(row,col) C[(row) + (col) * M]

    // Index variables
    const unsigned int row = tidx.global[0];
    const unsigned int col = tidx.tile[1] * TILE_SZ_B;

    // Privatization of output variables
    float c_reg[TILE_SZ_B] = {(float)0};

    // Loop over the input tiles
    for(unsigned int tileIdx = 0; tileIdx < (K - 1)/TILE_SZ_RATIO + 1; ++tileIdx) {
        // Load the tile of B into shared memory
        const unsigned int i = tidx.local[0]/TILE_SZ_B;
        const unsigned int j = tidx.local[0]%TILE_SZ_B;

        if (tileIdx*TILE_SZ_RATIO + i < K && col + j < N) {
            B_s[i][j] = B(tileIdx*TILE_SZ_RATIO + i, col + j);
        }
        else {
            B_s[i][j] = 0;
        }
        tidx.barrier.wait();

        // Loop over elements inside the tile
        for (unsigned int idx = 0; idx < TILE_SZ_RATIO; ++idx) {
            // Load tile of A matrix into register
            float a_reg;
            if(row < M && tileIdx*TILE_SZ_RATIO + idx < K) {
                a_reg = A(row, tileIdx*TILE_SZ_RATIO + idx);
            }
            else {
                a_reg = 0;
            }

            // Loop over and update the output elements assigned to the thread
            for(unsigned int outIdx = 0; outIdx < TILE_SZ_B; ++outIdx) {
                c_reg[outIdx] += a_reg*B_s[idx][outIdx];
            }
        }
        tidx.barrier.wait();
    }
    for (unsigned int outIdx = 0; outIdx < TILE_SZ_B; ++outIdx) {
        if (row < M && col + outIdx < N) {
            (C(row, col + outIdx) *= beta) += (c_reg[outIdx] *= alpha);
        }
    }
});
}

static void gemm_NoTransB(Concurrency::array_view<float, 1> &A, long aOffset,
                          Concurrency::array_view<float, 1> &B, long bOffset,
                          Concurrency::array_view<float, 1> &C, long cOffset,
                          int M, int N, int K, int lda, int ldb, int ldc,
                          float alpha, float beta,
                          Concurrency::array_view<float,1> &temp_buf)
{
    Concurrency::extent<2> grdExt(((M - 1) / TILE_SZ_A + 1) * TILE_SZ_A, (N - 1) / TILE_SZ_B + 1);
    Concurrency::tiled_extent <TILE_SZ_A, 1> t_ext(grdExt);

    Concurrency::parallel_for_each(t_ext,
                                   [=] (Concurrency::tiled_index<TILE_SZ_A,1> tidx)
                                   restrict(amp) {

    // Shared memory for tiling input B array
    tile_static float B_s [TILE_SZ_RATIO][TILE_SZ_B];

    // Macros for accessing flattened matrices
    #define A1(row,col) A[(row) * K + (col)]
    #define B1(row,col) B[(row) + (col) * K]
    #define C1(row,col) C[(row) + (col) * M]

    // Index variables
   const unsigned int row = tidx.global[0];
    const unsigned int col = tidx.tile[1] * TILE_SZ_B;

    // Privatization of output variables
    float c_reg[TILE_SZ_B] = {(float)0};

    // Loop over the input tiles
    for(unsigned int tileIdx = 0; tileIdx < (K - 1)/TILE_SZ_RATIO + 1; ++tileIdx) {
        // Load the tile of B into shared memory
        const unsigned int i = tidx.local[0]/TILE_SZ_B;
        const unsigned int j = tidx.local[0]%TILE_SZ_B;

        if (tileIdx*TILE_SZ_RATIO + i < K && col + j < N) {
            B_s[i][j] = B1(tileIdx*TILE_SZ_RATIO + i, col + j);
        }
        else {
            B_s[i][j] = 0;
        }
        tidx.barrier.wait();

        // Loop over elements inside the tile
        for (unsigned int idx = 0; idx < TILE_SZ_RATIO; ++idx) {
            // Load tile of A matrix into register
            float a_reg;
            if(row < M && tileIdx*TILE_SZ_RATIO + idx < K) {
                a_reg = A1(row, tileIdx*TILE_SZ_RATIO + idx);
            }
            else {
                a_reg = 0;
            }

            // Loop over and update the output elements assigned to the thread
            for(unsigned int outIdx = 0; outIdx < TILE_SZ_B; ++outIdx) {
                c_reg[outIdx] += a_reg*B_s[idx][outIdx];
            }
        }
        tidx.barrier.wait();
    }
    for (unsigned int outIdx = 0; outIdx < TILE_SZ_B; ++outIdx) {
        if (row < M && col + outIdx < N) {
            (C1(row, col + outIdx) *= beta) += (c_reg[outIdx] *= alpha);
        }
    }
});
}

static void gemm_TransAB(Concurrency::array_view<float, 1> &A, long aOffset,
                         Concurrency::array_view<float, 1> &B, long bOffset,
                         Concurrency::array_view<float, 1> &C, long cOffset,
                         int M, int N, int K, int lda, int ldb, int ldc,
                         float alpha, float beta)
{
    Concurrency::extent<2> grdExt(((M - 1) / TILE_SZ_A + 1) * TILE_SZ_A, (N - 1) / TILE_SZ_B + 1);
    Concurrency::tiled_extent <TILE_SZ_A, 1> t_ext(grdExt);

    Concurrency::parallel_for_each(t_ext,
                                   [=] (Concurrency::tiled_index<TILE_SZ_A,1> tidx)
                                   restrict(amp) {

    // Shared memory for tiling input B array
    tile_static float B_s [TILE_SZ_RATIO][TILE_SZ_B];

    // Macros for accessing flattened matrices
    #define A1(row,col) A[(row) * K + (col)]
    #define B1(row,col) B[(row) * N + (col)]
    #define C1(row,col) C[(row) + (col) * M]

    // Index variables
    const unsigned int row = tidx.global[0];
    const unsigned int col = tidx.tile[1] * TILE_SZ_B;

    // Privatization of output variables
    float c_reg[TILE_SZ_B] = {(float)0};

    // Loop over the input tiles
    for(unsigned int tileIdx = 0; tileIdx < (K - 1)/TILE_SZ_RATIO + 1; ++tileIdx) {
        // Load the tile of B into shared memory
        const unsigned int i = tidx.local[0]/TILE_SZ_B;
        const unsigned int j = tidx.local[0]%TILE_SZ_B;

        if (tileIdx*TILE_SZ_RATIO + i < K && col + j < N) {
            B_s[i][j] = B1(tileIdx*TILE_SZ_RATIO + i, col + j);
        }
        else {
            B_s[i][j] = 0;
        }
        tidx.barrier.wait();

        // Loop over elements inside the tile
        for (unsigned int idx = 0; idx < TILE_SZ_RATIO; ++idx) {
            // Load tile of A matrix into register
            float a_reg;
            if(row < M && tileIdx*TILE_SZ_RATIO + idx < K) {
                a_reg = A1(row, tileIdx*TILE_SZ_RATIO + idx);
            }
            else {
                a_reg = 0;
            }

            // Loop over and update the output elements assigned to the thread
            for(unsigned int outIdx = 0; outIdx < TILE_SZ_B; ++outIdx) {
                c_reg[outIdx] += a_reg*B_s[idx][outIdx];
            }
        }
        tidx.barrier.wait();
    }
    for (unsigned int outIdx = 0; outIdx < TILE_SZ_B; ++outIdx) {
        if (row < M && col + outIdx < N) {
            (C1(row, col + outIdx) *= beta) += (c_reg[outIdx] *= alpha);
        }
    }
});
}
#endif

#if STEP
static void gemm_NoTransAB_batch(Concurrency::array_view<float, 1> &A, long aOffset,
                                 Concurrency::array_view<float, 1> &B, long bOffset,
                                 Concurrency::array_view<float, 1> &C, long cOffset,
                                 int M, int N, int K, int lda, int ldb, int ldc,
                                 float alpha, float beta)
{
  Concurrency::extent<2> grdExt((N + (TILESIZE - 1)) & ~(TILESIZE - 1), (M + (TILESIZE - 1)) & ~(TILESIZE - 1));
  Concurrency::tiled_extent<TILESIZE, TILESIZE> t_ext(grdExt);

  Concurrency::parallel_for_each(t_ext, [=] (Concurrency::tiled_index<TILESIZE, TILESIZE> tidx) restrict(amp)
  {
    int shiftFactor = Concurrency::fast_math::log2(STEPSIZE);
    float rC[1][1];
    float rA[1][STEPSIZE/TILESIZE];
    float rB[1][STEPSIZE/TILESIZE];
    tile_static float lA[TILESIZE * MICROTILESIZE * STEPSIZE];//8*8+8
    tile_static float lB[TILESIZE * MICROTILESIZE * STEPSIZE];
    rC[0][0] = 0;
    int gidx = tidx.tile[1];
    int gidy = tidx.tile[0];
    int idx = tidx.local[1];
    int idy = tidx.local[0];
    int idt = TILESIZE * idy + idx;
    int idxT = idt % TILESIZE;
    int idyT = idt / TILESIZE;
    int block_k = ((K + (STEPSIZE - 1)) & ~(STEPSIZE - 1)) >> shiftFactor;
    int i = 0;
    do
    {
      tidx.barrier.wait();

      // Load Sections of A and B into respective shared memory slots
      for (int sec =0; sec < STEPSIZE/TILESIZE; ++sec)
      {
        // Load Section 'sec' from global memory B onto shared lB
        if(gidy*TILESIZE+idxT  < N && (idyT + i * STEPSIZE + (TILESIZE * sec)) < K) 
          lB[idxT*TILESIZE+idyT + (TILESIZE * TILESIZE * sec)] = B[bOffset + (gidy*TILESIZE+ idxT) * ldb + idyT + i * STEPSIZE + (TILESIZE * sec)];
        else
          lB[idxT*TILESIZE+idyT + (TILESIZE * TILESIZE * sec)] = 0;

        // Load Section 'sec' from global memory A onto shared lA
        if(gidx * TILESIZE + idxT < M && (i * STEPSIZE + idyT + (TILESIZE * sec)) < K)
           lA[idxT*TILESIZE+idyT + (TILESIZE * TILESIZE * sec)] = A[aOffset  + gidx*TILESIZE+ idxT + idyT*lda + i * (lda << shiftFactor) + (TILESIZE * sec) * lda];
        else
           lA[idxT*TILESIZE+idyT + (TILESIZE * TILESIZE * sec)] = 0;
      }
      tidx.barrier.wait();

      int offA = idx * TILESIZE;
      int offB = idy * TILESIZE;
      int offset = 1;

      for (int iter=0; iter < TILESIZE; ++iter)
      {
         MS1x1(offset, offset);
      }

      i++;
    } while (--block_k > 0);


    tidx.barrier.wait();
    if(gidx*TILESIZE+idx < M && gidy*TILESIZE+idy < N)
        C[cOffset + gidx*TILESIZE +idx + (gidy*TILESIZE + idy)*ldc] = alpha * rC[0][0] + beta * C[cOffset + gidx*TILESIZE+idx + (gidy*TILESIZE + idy)*ldc];
  });

}

static void gemm_NoTransA_batch(Concurrency::array_view<float, 1> &A, long aOffset,
                                Concurrency::array_view<float, 1> &B, long bOffset,
                                Concurrency::array_view<float, 1> &C, long cOffset,
                                int M, int N, int K, int lda, int ldb, int ldc,
                                float alpha, float beta)
{
  Concurrency::extent<2> grdExt((N + (TILESIZE - 1)) & ~(TILESIZE - 1), (M + (TILESIZE - 1)) & ~(TILESIZE - 1));
  Concurrency::tiled_extent<TILESIZE, TILESIZE> t_ext(grdExt);

  Concurrency::parallel_for_each(t_ext, [=] (Concurrency::tiled_index<TILESIZE, TILESIZE> tidx) restrict(amp)
  { 
    int shiftFactor = Concurrency::fast_math::log2(STEPSIZE);
    float rC[1][1] = {(float)0};
    float rA[1][STEPSIZE / TILESIZE];
    float rB[1][STEPSIZE / TILESIZE];
    tile_static float lA[TILESIZE * MICROTILESIZE * STEPSIZE];
    tile_static float lB[TILESIZE * MICROTILESIZE * STEPSIZE];
    int gidx = tidx.tile[1];
    int gidy = tidx.tile[0];
    int idx = tidx.local[1];
    int idy = tidx.local[0];
    int idt = TILESIZE * idy + idx;
    int idxT = idt % TILESIZE;
    int idyT = idt / TILESIZE;
    int block_k = ((K + (STEPSIZE - 1)) & ~(STEPSIZE - 1)) >> shiftFactor;
    int i = 0;
    do
    {
      tidx.barrier.wait();
      for(int sec = 0; sec < STEPSIZE / TILESIZE; ++sec)
      {
        if(gidy * TILESIZE + idxT < N && i * STEPSIZE + idyT + (sec * TILESIZE) < K)
        {
          lB[(idyT + (sec * TILESIZE)) * TILESIZE + idxT] = B[bOffset + gidy * TILESIZE + idxT + (idyT + (sec * TILESIZE)) * ldb + i * (ldb << shiftFactor)];
        }
        else
        {
          lB[(idyT + (sec * TILESIZE )) * TILESIZE + idxT] = 0;
	}

        if(gidx * TILESIZE + idxT < M && i * STEPSIZE + idyT + (sec * TILESIZE) < K)
        {
          lA[(idyT + (sec * TILESIZE)) * TILESIZE + idxT] = A[aOffset + gidx * TILESIZE + idxT + (idyT + (sec * TILESIZE)) * lda + i * (lda << shiftFactor)];
        }
        else
        {
          lA[(idyT + (sec * TILESIZE)) * TILESIZE + idxT] = 0;
        }
      }
      tidx.barrier.wait();

      int offA = idx;
      int offB = idy;
      int offset = TILESIZE;

      for (int iter=0; iter < TILESIZE; ++iter)
      {
        MS1x1(offset, offset);
      }

      i++;
    } while (--block_k > 0);

    tidx.barrier.wait();
    if(gidx * TILESIZE + idx < M && gidy * TILESIZE + idy < N)
      C[cOffset + gidx * TILESIZE + idx + (gidy * TILESIZE + idy) * ldc] =  alpha * rC[0][0] + beta * C[cOffset + gidx * TILESIZE + idx + (gidy * TILESIZE + idy) * ldc];
  });

}

static void gemm_NoTransB_batch(Concurrency::array_view<float, 1> &A, long aOffset,
                                Concurrency::array_view<float, 1> &B, long bOffset,
                                Concurrency::array_view<float, 1> &C, long cOffset,
                                int M, int N, int K, int lda, int ldb, int ldc,
                                float alpha, float beta)
{
  Concurrency::extent<2> grdExt((N + (TILESIZE - 1)) & ~(TILESIZE - 1), (M + (TILESIZE - 1)) & ~(TILESIZE - 1));
  Concurrency::tiled_extent<TILESIZE, TILESIZE> t_ext(grdExt);

  Concurrency::parallel_for_each(t_ext, [=] (Concurrency::tiled_index<TILESIZE, TILESIZE> tidx) restrict(amp)
  {
    int shiftFactor = Concurrency::fast_math::log2(STEPSIZE);
    float rC[1][1] = {(float)0};
    float rA[1][STEPSIZE / TILESIZE];
    float rB[1][STEPSIZE / TILESIZE];
    tile_static float lA[TILESIZE * MICROTILESIZE * STEPSIZE];
    tile_static float lB[TILESIZE * MICROTILESIZE * STEPSIZE];
    int gidx = tidx.tile[1];
    int gidy = tidx.tile[0];
    int idx = tidx.local[1];
    int idy = tidx.local[0];
    int idt = TILESIZE * idy + idx;
    int idxT = idt % TILESIZE;
    int idyT = idt / TILESIZE;
    int block_k =((K + (STEPSIZE - 1)) & ~(STEPSIZE - 1)) >> shiftFactor;

    int i = 0;
    do
    {
      tidx.barrier.wait();
      for(int sec = 0; sec < STEPSIZE / TILESIZE; ++sec)
      {
        if(gidy * TILESIZE + idxT < N && i * STEPSIZE + idyT + (sec * TILESIZE) < K)
        {
          lB[(sec * TILESIZE * TILESIZE) + idyT + idxT * TILESIZE] = B[bOffset + (gidy * TILESIZE + idxT) * ldb + idyT + i * STEPSIZE + (sec * TILESIZE)];
        }
        else
        {
          lB[(sec * TILESIZE * TILESIZE ) + idyT + idxT * TILESIZE] = 0;
	}

        if(gidx * TILESIZE + idxT < M && i * STEPSIZE + idyT + (sec * TILESIZE ) < K)
        {
          lA[(sec * TILESIZE * TILESIZE) + idyT + idxT * TILESIZE] = A[aOffset + (gidx * TILESIZE + idxT) * lda + idyT + i * STEPSIZE + (sec * TILESIZE)];
        }
        else
        {
          lA[(sec * TILESIZE * TILESIZE ) + idyT + idxT * TILESIZE] = 0;
        }
      }

    tidx.barrier.wait();

    int offA = idx * TILESIZE;
    int offB = idy * TILESIZE;
    int offset = 1;

    for (int iter=0; iter < TILESIZE; ++iter)
    {
      MS1x1(offset, offset);
    }

    i++;
    }while (--block_k > 0);

    tidx.barrier.wait();
    if(gidx * TILESIZE + idx < M && gidy * TILESIZE + idy < N)
        C[cOffset + (gidx * TILESIZE + idx) + (gidy * TILESIZE + idy) * ldc] = alpha * rC[0][0] + beta * C[cOffset + (gidx * TILESIZE + idx) + (gidy * TILESIZE + idy) * ldc];
  });
}

static void gemm_TransAB_batch(Concurrency::array_view<float, 1> &A, long aOffset,
                               Concurrency::array_view<float, 1> &B, long bOffset,
                               Concurrency::array_view<float, 1> &C, long cOffset,
                               int M, int N, int K, int lda, int ldb, int ldc,
                               float alpha, float beta)

{
  Concurrency::extent<2> grdExt((N + (TILESIZE - 1)) & ~(TILESIZE - 1), (M + (TILESIZE - 1)) & ~(TILESIZE - 1));
  Concurrency::tiled_extent<TILESIZE, TILESIZE> t_ext(grdExt);

  Concurrency::parallel_for_each(t_ext, [=] (Concurrency::tiled_index<TILESIZE, TILESIZE> tidx) restrict(amp)
  {
    int shiftFactor = Concurrency::fast_math::log2(STEPSIZE);
    float rC[1][1] = {(float)0};
    float rA[1][STEPSIZE/TILESIZE];
    float rB[1][STEPSIZE/TILESIZE];
    tile_static float lA[TILESIZE * MICROTILESIZE * STEPSIZE];
    tile_static float lB[TILESIZE * MICROTILESIZE * STEPSIZE];
    int gidx = tidx.tile[1];
    int gidy = tidx.tile[0];
    int idx = tidx.local[1];
    int idy = tidx.local[0];
    int idt = TILESIZE * idy + idx;
    int idxT = idt % TILESIZE;
    int idyT = idt / TILESIZE;
    int block_k = ((K + (STEPSIZE - 1)) & ~(STEPSIZE - 1)) >> shiftFactor;
    int i = 0;
    do
    {
      tidx.barrier.wait();
      for(int sec = 0; sec < STEPSIZE / TILESIZE; sec++ ) {

        if(gidy * TILESIZE + idxT < N && i * STEPSIZE + idyT +(sec * TILESIZE) < K)
        {
          lB[((idxT + sec * TILESIZE) * TILESIZE) + idyT] = B[bOffset + gidy * TILESIZE + idxT + ((idyT + (sec * TILESIZE)) *ldb) + i * (ldb << shiftFactor)];
        }
        else
        {
          lB[((idxT + sec * TILESIZE) * TILESIZE) + idyT] = 0;
        }

        if(gidx * TILESIZE + idxT < M && i * STEPSIZE + idyT + (sec * TILESIZE) < K)
        {
          lA[(sec * TILESIZE * TILESIZE) + idyT + idxT * TILESIZE] = A[aOffset  + (gidx * TILESIZE + idxT) * lda + idyT + i * STEPSIZE + (sec * TILESIZE)];
        }
        else
        {
          lA[(sec * TILESIZE * TILESIZE ) + idyT + idxT * TILESIZE] = 0;
        }
      }
      tidx.barrier.wait();

      int offA = idx * TILESIZE;
      int offB = idy * TILESIZE;
      int offset = 1;
      for (int iter = 0; iter < TILESIZE; ++iter)
      {
        MS1x1(offset, offset);
      }
      i++;
    } while (--block_k > 0);


    tidx.barrier.wait();
    if(gidx * TILESIZE + idx < M && gidy * TILESIZE + idy < N)
        C[cOffset + gidx * TILESIZE + idx + (gidy * TILESIZE + idy) * ldc] = alpha * rC[0][0] + beta * C[cOffset + gidx * TILESIZE + idx + (gidy * TILESIZE + idy) * ldc];
  });
}
#endif

#if SUBMICROTILE
#if NOTRANSAB
static void gemm_NoTransAB_subMicroTile(Concurrency::array_view<float, 1> &A, long aOffset,
                                        Concurrency::array_view<float, 1> &B, long bOffset,
                                        Concurrency::array_view<float, 1> &C, long cOffset,
                                        int M, int N, int K, int lda, int ldb, int ldc,
                                        float alpha, float beta)
{
  Concurrency::extent<2> grdExt(((N / 2) + (TILESIZE - 1)) & ~(TILESIZE - 1), ((M / 2) + (TILESIZE - 1)) & ~(TILESIZE - 1));
  Concurrency::tiled_extent<TILESIZE, TILESIZE> t_ext(grdExt);

  Concurrency::parallel_for_each(t_ext, [=] (Concurrency::tiled_index<TILESIZE, TILESIZE> tidx) restrict(amp)
  {
    float rC[MICROTILESIZE][MICROTILESIZE] = {(float)0};
    float rA[1][MICROTILESIZE];
    float rB[1][MICROTILESIZE];
    tile_static float lA[TILESIZE * TILESIZE * MICROTILESIZE];
    tile_static float lB[TILESIZE * TILESIZE * MICROTILESIZE];
    int gidx = tidx.tile[1];
    int gidy = tidx.tile[0];
    int idx = tidx.local[1];
    int idy = tidx.local[0];
    int idt = TILESIZE * idy + idx;
    int idxT = idt % TILESIZE;
    int idyT = idt / TILESIZE;
    int block_k = 0;
    do
    {
      tidx.barrier.wait();
      for(int sec = 0; sec < MICROTILESIZE; ++sec)
      {
        if(gidy * TILESIZE * MICROTILESIZE + idxT + (sec * TILESIZE) < N && block_k * TILESIZE + idyT < K)
        {
          lB[(idyT * TILESIZE * MICROTILESIZE) + idxT + (sec * TILESIZE)] = B[bOffset + (gidy * TILESIZE * MICROTILESIZE + idxT + sec * TILESIZE) * ldb + idyT + block_k * TILESIZE];
        }
        else
        {
          lB[(idyT * TILESIZE * MICROTILESIZE) + idxT + (sec * TILESIZE)] = 0;
	}

        if(gidx * TILESIZE * MICROTILESIZE + idxT + (sec * TILESIZE) < M && block_k * TILESIZE + idyT < K)
        {
          lA[(idyT * TILESIZE * MICROTILESIZE) + idxT + (sec * TILESIZE)] = A[aOffset + (gidx * TILESIZE * MICROTILESIZE) + idxT + (sec * TILESIZE) +  idyT * lda + block_k * (lda * TILESIZE)];
        }
        else
        {
          lA[(idyT * TILESIZE * MICROTILESIZE) + idxT + (sec * TILESIZE)] = 0;
        }
      }
      tidx.barrier.wait();

      int offA = idx;
      int offB = idy;
      for (int iter=0; iter < TILESIZE; ++iter)
      {
        MTS;
      }
      tidx.barrier.wait();
    } while (++block_k < (((K + TILESIZE - 1) & ~(TILESIZE - 1))/TILESIZE));

    int xIndex = gidx * TILESIZE * MICROTILESIZE + idx;
    int yIndex = (gidy * TILESIZE * MICROTILESIZE + idy) * ldc;
    for( int row = 0; row < MICROTILESIZE; row++)
    {
      for( int col = 0; col < MICROTILESIZE ; col++)
      {
      if(xIndex + (TILESIZE * col) < M && (yIndex / ldc) + (TILESIZE * row) < N)
        C[cOffset + (xIndex + TILESIZE * col) + yIndex + (TILESIZE * row) * ldc] = alpha * rC[col][row] + beta * C[cOffset + (xIndex + TILESIZE * col) + yIndex + (TILESIZE * row) * ldc];
      }
    }
 });
}
#endif
#if NOTRANSA
static void gemm_NoTransA_subMicroTile(Concurrency::array_view<float, 1> &A, long aOffset,
                                       Concurrency::array_view<float, 1> &B, long bOffset,
                                       Concurrency::array_view<float, 1> &C, long cOffset,
                                       int M, int N, int K, int lda, int ldb, int ldc,
                                       float alpha, float beta)
{
  Concurrency::extent<2> grdExt(((N / 2) + (TILESIZE - 1)) & ~(TILESIZE - 1), ((M / 2) + (TILESIZE - 1)) & ~(TILESIZE - 1));
  Concurrency::tiled_extent<TILESIZE, TILESIZE> t_ext(grdExt);

  Concurrency::parallel_for_each(t_ext, [=] (Concurrency::tiled_index<TILESIZE, TILESIZE> tidx) restrict(amp)
  {
    float rC[MICROTILESIZE][MICROTILESIZE] = {(float)0};
    float rA[1][MICROTILESIZE];
    float rB[1][MICROTILESIZE];
    tile_static float lA[TILESIZE * TILESIZE * MICROTILESIZE];
    tile_static float lB[TILESIZE * TILESIZE * MICROTILESIZE];
    int gidx = tidx.tile[1];
    int gidy = tidx.tile[0];
    int idx = tidx.local[1];
    int idy = tidx.local[0];
    int idt = TILESIZE * idy + idx;
    int idxT = idt % TILESIZE;
    int idyT = idt / TILESIZE;
    int block_k = 0;
    do
    {
      tidx.barrier.wait();
      for(int sec = 0; sec < MICROTILESIZE; ++sec)
      {
        if(gidy * TILESIZE * MICROTILESIZE + idxT + (sec * TILESIZE) < N && block_k * TILESIZE + idyT < K)
        {
          lB[(idyT * TILESIZE * MICROTILESIZE) + idxT + (sec * TILESIZE)] = B[bOffset + (gidy * TILESIZE * MICROTILESIZE) + idxT + (sec * TILESIZE) + idyT * ldb + block_k * (ldb * TILESIZE)];
        }
        else
        {
          lB[(idyT * TILESIZE * MICROTILESIZE) + idxT + (sec * TILESIZE)] = 0;
	}

        if(gidx * TILESIZE * MICROTILESIZE + idxT + (sec * TILESIZE) < M && block_k * TILESIZE + idyT < K)
        {
          lA[(idyT * TILESIZE * MICROTILESIZE) + idxT + (sec * TILESIZE)] = A[aOffset + (gidx * TILESIZE * MICROTILESIZE) + idxT + (sec * TILESIZE) +  idyT * lda + block_k * (lda * TILESIZE)];
        }
        else
        {
          lA[(idyT * TILESIZE * MICROTILESIZE) + idxT + (sec * TILESIZE)] = 0;
        }
      }
      tidx.barrier.wait();

      int offA = idx;
      int offB = idy;
      for (int iter=0; iter < TILESIZE; ++iter)
      {
        MTS;
      }
      tidx.barrier.wait();
    } while (++block_k < (((K + TILESIZE - 1) & ~(TILESIZE - 1))/TILESIZE));

    int xIndex = gidx * TILESIZE * MICROTILESIZE + idx;
    int yIndex = (gidy * TILESIZE * MICROTILESIZE + idy) * ldc;
    for( int row = 0; row < MICROTILESIZE; row++)
    {
      for( int col = 0; col < MICROTILESIZE ; col++)
      {
      if(xIndex + (TILESIZE * col) < M && (yIndex / ldc) + (TILESIZE * row) < N)
        C[cOffset + (xIndex + TILESIZE * col) + yIndex + (TILESIZE * row) * ldc] = alpha * rC[col][row] + beta * C[cOffset + (xIndex + TILESIZE * col) + yIndex + (TILESIZE * row) * ldc];
      }
    }
 });
}
#endif

#if NOTRANSB
static void gemm_NoTransB_subMicroTile(Concurrency::array_view<float, 1> &A, long aOffset,
                                       Concurrency::array_view<float, 1> &B, long bOffset,
                                       Concurrency::array_view<float, 1> &C, long cOffset,
                                       int M, int N, int K, int lda, int ldb, int ldc,
                                       float alpha, float beta)
{
  Concurrency::extent<2> grdExt(((N / 2) + (TILESIZE - 1)) & ~(TILESIZE - 1), ((M / 2) + (TILESIZE - 1)) & ~(TILESIZE - 1));
  Concurrency::tiled_extent<TILESIZE, TILESIZE> t_ext(grdExt);

  Concurrency::parallel_for_each(t_ext, [=] (Concurrency::tiled_index<TILESIZE, TILESIZE> tidx) restrict(amp)
  {
    float rC[MICROTILESIZE][MICROTILESIZE] = {(float)0};
    float rA[1][MICROTILESIZE];
    float rB[1][MICROTILESIZE];
    tile_static float lA[TILESIZE * TILESIZE * MICROTILESIZE];
    tile_static float lB[TILESIZE * TILESIZE * MICROTILESIZE];
    int gidx = tidx.tile[1];
    int gidy = tidx.tile[0];
    int idx = tidx.local[1];
    int idy = tidx.local[0];
    int idt = TILESIZE * idy + idx;
    int idxT = idt % TILESIZE;
    int idyT = idt / TILESIZE;
    int block_k = 0;
    do
    {
      tidx.barrier.wait();
      for(int sec = 0; sec < MICROTILESIZE; ++sec)
      {
        if(gidy * TILESIZE * MICROTILESIZE + idxT + (sec * TILESIZE) < N && block_k * TILESIZE + idyT < K)
        {
          lB[(idyT * TILESIZE * MICROTILESIZE) + idxT + (sec * TILESIZE)] = B[bOffset + (gidy * TILESIZE * MICROTILESIZE + idxT + sec * TILESIZE) * ldb + idyT + block_k * TILESIZE];
        }
        else
        {
          lB[(idyT * TILESIZE * MICROTILESIZE) + idxT + (sec * TILESIZE)] = 0;
	}

        if(gidx * TILESIZE * MICROTILESIZE + idxT + (sec * TILESIZE) < M && block_k * TILESIZE + idyT < K)
        {
          lA[(idyT * TILESIZE * MICROTILESIZE) + idxT + (sec * TILESIZE)] = A[aOffset + (gidx * TILESIZE * MICROTILESIZE + idxT + sec * TILESIZE) * lda +  idyT + block_k * TILESIZE];
        }
        else
        {
          lA[(idyT * TILESIZE * MICROTILESIZE) + idxT + (sec * TILESIZE)] = 0;
        }
      }
      tidx.barrier.wait();

      int offA = idx;
      int offB = idy;
      for (int iter=0; iter < TILESIZE; ++iter)
      {
        MTS;
      }
      tidx.barrier.wait();
    } while (++block_k < (((K + TILESIZE - 1) & ~(TILESIZE - 1))/TILESIZE));

    int xIndex = gidx * TILESIZE * MICROTILESIZE + idx;
    int yIndex = (gidy * TILESIZE * MICROTILESIZE + idy) * ldc;
    for( int row = 0; row < MICROTILESIZE; row++)
    {
      for( int col = 0; col < MICROTILESIZE ; col++)
      {
      if(xIndex + (TILESIZE * col) < M && (yIndex / ldc) + (TILESIZE * row) < N)
        C[cOffset + (xIndex + TILESIZE * col) + yIndex + (TILESIZE * row) * ldc] = alpha * rC[col][row] + beta * C[cOffset + (xIndex + TILESIZE * col) + yIndex + (TILESIZE * row) * ldc];
      }
    }
 });
}
#endif

#if TRANSAB
static void gemm_TransAB_subMicroTile(Concurrency::array_view<float, 1> &A, long aOffset,
                                      Concurrency::array_view<float, 1> &B, long bOffset,
                                      Concurrency::array_view<float, 1> &C, long cOffset,
                                      int M, int N, int K, int lda, int ldb, int ldc,
                                      float alpha, float beta)
{
  Concurrency::extent<2> grdExt(((N / 2) + (TILESIZE - 1)) & ~(TILESIZE - 1), ((M / 2) + (TILESIZE - 1)) & ~(TILESIZE - 1));
  Concurrency::tiled_extent<TILESIZE, TILESIZE> t_ext(grdExt);

  Concurrency::parallel_for_each(t_ext, [=] (Concurrency::tiled_index<TILESIZE, TILESIZE> tidx) restrict(amp)
  {
    float rC[MICROTILESIZE][MICROTILESIZE] = {(float)0};
    float rA[1][MICROTILESIZE];
    float rB[1][MICROTILESIZE];
    tile_static float lA[TILESIZE * TILESIZE * MICROTILESIZE];
    tile_static float lB[TILESIZE * TILESIZE * MICROTILESIZE];
    int gidx = tidx.tile[1];
    int gidy = tidx.tile[0];
    int idx = tidx.local[1];
    int idy = tidx.local[0];
    int idt = TILESIZE * idy + idx;
    int idxT = idt % TILESIZE;
    int idyT = idt / TILESIZE;
    int block_k = 0;
    do
    {
      tidx.barrier.wait();
      for(int sec = 0; sec < MICROTILESIZE; ++sec)
      {
        if(gidy * TILESIZE * MICROTILESIZE + idxT + (sec * TILESIZE) < N && block_k * TILESIZE + idyT < K)
        {
          lB[(idyT * TILESIZE * MICROTILESIZE) + idxT + (sec * TILESIZE)] = B[bOffset + (gidy * TILESIZE * MICROTILESIZE) + idxT + (sec * TILESIZE) + idyT * ldb + block_k * (ldb * TILESIZE)];
        }
        else
        {
          lB[(idyT * TILESIZE * MICROTILESIZE) + idxT + (sec * TILESIZE)] = 0;
	}

        if(gidx * TILESIZE * MICROTILESIZE + idxT + (sec * TILESIZE) < M && block_k * TILESIZE + idyT < K)
        {
          lA[(idyT * TILESIZE * MICROTILESIZE) + idxT + (sec * TILESIZE)] = A[aOffset + (gidx * TILESIZE * MICROTILESIZE + idxT + sec * TILESIZE) * lda +  idyT + block_k * TILESIZE];
        }
        else
        {
          lA[(idyT * TILESIZE * MICROTILESIZE) + idxT + (sec * TILESIZE)] = 0;
        }
      }
      tidx.barrier.wait();

      int offA = idx;
      int offB = idy;
      for (int iter=0; iter < TILESIZE; ++iter)
      {
        MTS;
      }
      tidx.barrier.wait();
    } while (++block_k < (((K + TILESIZE - 1) & ~(TILESIZE - 1))/TILESIZE));

    int xIndex = gidx * TILESIZE * MICROTILESIZE + idx;
    int yIndex = (gidy * TILESIZE * MICROTILESIZE + idy) * ldc;
    for( int row = 0; row < MICROTILESIZE; row++)
    {
      for( int col = 0; col < MICROTILESIZE ; col++)
      {
      if(xIndex + (TILESIZE * col) < M && (yIndex / ldc) + (TILESIZE * row) < N)
        C[cOffset + (xIndex + TILESIZE * col) + yIndex + (TILESIZE * row) * ldc] = alpha * rC[col][row] + beta * C[cOffset + (xIndex + TILESIZE * col) + yIndex + (TILESIZE * row) * ldc];
      }
    }
 });
}
#endif
#endif

int gemm_AMP(char TransA, char TransB, const int M, const int N, const int K,
             const float alpha, Concurrency::array_view<float> &A_mat, long aOffset, long lda,
             Concurrency::array_view<float> &B_mat, long bOffset, long ldb, const float beta,
             Concurrency::array_view<float> &C_mat, long cOffset, long ldc,
             Concurrency::array_view<float> &temp_buf)
{
  int i, j;

  // Quick return if possible
  if (!M || !N || ((alpha == 0 || !K) && beta == 1))
    return 0;
  // For alpha = 0
  if (alpha == 0)
  {
    if (beta == 0)
    {
      for (j = 0; j < N; ++j)
        for (i = 0; i < M; ++i)
          C_mat[i + j * ldc] = 0;
    }
    else
    {
      for (j = 0; j < N; ++j)
        for (i = 0; i < M; ++i)
          C_mat[i + j * ldc] *= beta;
    }
    return 0;
  }
  // Start the operations
#if REGISTER
  {
    if (TransB == 'n')
    {
      if (TransA == 'n')
        gemm_NoTransAB(A_mat, aOffset, B_mat, bOffset, C_mat, cOffset, M, N, K, lda, ldb, ldc, alpha, beta);
      else
        gemm_NoTransB(A_mat, aOffset, B_mat, bOffset, C_mat, cOffset, M, N, K, lda, ldb, ldc, alpha, beta, temp_buf);
    }
    else if (TransA == 'n')
      gemm_NoTransA(A_mat, aOffset, B_mat, bOffset, C_mat, cOffset, M, N, K, lda, ldb, ldc, alpha, beta);
    else
      gemm_TransAB(A_mat, aOffset, B_mat, bOffset, C_mat, cOffset, M, N, K, lda, ldb, ldc, alpha, beta);
  }
#endif
#if REGISTER_EXTN
 {
     if (TransA == 'n')
      gemm_NoTransA_extend(A_mat, aOffset, B_mat, bOffset, C_mat, cOffset, M, N, K, lda, ldb, ldc, alpha, beta);
 }
#endif
#if STEP 
  {
    if (TransB == 'n')
    {
      if (TransA == 'n')
        gemm_NoTransAB_batch(A_mat, aOffset, B_mat, bOffset, C_mat, cOffset, M, N, K, lda, ldb, ldc, alpha, beta);
      else
        gemm_NoTransB_batch(A_mat, aOffset, B_mat, bOffset, C_mat, cOffset, M, N, K, lda, ldb, ldc, alpha, beta);
    }
    else if (TransA == 'n')
      gemm_NoTransA_batch(A_mat, aOffset, B_mat, bOffset, C_mat, cOffset, M, N, K, lda, ldb, ldc, alpha, beta);
    else
      gemm_TransAB_batch(A_mat, aOffset, B_mat, bOffset, C_mat, cOffset, M, N, K, lda, ldb, ldc, alpha, beta);
  }
#endif
#if SUBMICROTILE
  {
    if (TransB == 'n')
    {
      if (TransA == 'n')
      {
#if NOTRANSAB
       gemm_NoTransAB_subMicroTile(A_mat, aOffset, B_mat, bOffset, C_mat, cOffset, M, N, K, lda, ldb, ldc, alpha, beta);
#endif
      }
      else
      {
#if NOTRANSB
        gemm_NoTransB_subMicroTile(A_mat, aOffset, B_mat, bOffset, C_mat, cOffset, M, N, K, lda, ldb, ldc, alpha, beta);
#endif
      }
    }
    else if (TransA == 'n')
    {
#if NOTRANSA
      gemm_NoTransA_subMicroTile(A_mat, aOffset, B_mat, bOffset, C_mat, cOffset, M, N, K, lda, ldb, ldc, alpha, beta);
#endif
    }
    else
    {
#if TRANSAB
     gemm_TransAB_subMicroTile(A_mat, aOffset, B_mat, bOffset, C_mat, cOffset, M, N, K, lda, ldb, ldc, alpha, beta);
#endif
    }
  }
#endif

 return 0;
}

ampblasStatus Ampblaslibrary :: ampblas_sgemm(const enum AMPBLAS_TRANS typeA,
                                              const enum AMPBLAS_TRANS typeB,
                                              const int M, const int N,
                                              const int K, const float *alpha,
                                              float *A, const long lda,
                                              float *B, const long ldb,
                                              const float *beta, float *C,
                                              const long ldc, const long aOffset,
                                              const long bOffset,
                                              const long cOffset)
{
    Concurrency::array_view<float> A_mat(K * M, A);
    Concurrency::array_view<float> B_mat(N * K, B);
    Concurrency::array_view<float> C_mat(M * N, C);
    Concurrency::array_view<float> *temp_buf = NULL;

    gemm_AMP(typeA, typeB, M, N, K, *alpha, A_mat, aOffset, lda, B_mat,
             bOffset, ldb, *beta, C_mat, cOffset, ldc, *temp_buf);

    C_mat.synchronize();

    /* Print Output */
    /*for (int i = 0; i < M * N; i++)
        cout<<" C_mat["<<i<<"] "<<C_mat[i]<<endl;*/

    return AMPBLAS_SUCCESS;
}
