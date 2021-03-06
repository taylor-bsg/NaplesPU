#ifndef NPU_ACCELERATOR
    #include <strings.h>
    #include <stdio.h>
#else
    #define CORE_ID    __builtin_npu_read_control_reg(0)
    #define THREAD_ID  __builtin_npu_read_control_reg(2)
#endif

#define LOOP 1
#define N 16

static int A[N][N] = {{0}};
static int B[N][N] = {{0}};
static int C[N][N] __attribute__((aligned(64)))  = {{0}};

void init_matrix(int a[N][N]);
void matrix_mult (const int a[N][N], const int b[N][N], int mult[N][N], int core_id, int thread_id);

int main(){
    init_matrix(A);
    init_matrix(B);
#ifdef NPU_ACCELERATOR
  // Setting a non-coherent memory space
  // start addr = 0x800000, end_addr = 0xFFFFFFFF, valid = 1
  // int mmap = (2 << 11) | (0x3FF << 1) | 1;
  // __builtin_npu_write_control_reg(mmap | (THREAD_ID << 21), 19);

  for (int i = 0; i < LOOP; i++) {
    matrix_mult(A, B, C, CORE_ID, THREAD_ID);
  }

  __builtin_npu_barrier(42, CORE_NUMB * THREAD_NUMB - 1);

  if (CORE_ID == 0 && THREAD_ID == 0) {
    for (int i = 0; i < N*N; i += 64 / sizeof(int)) {
      __builtin_npu_flush((int) &C[i / N][i % N]);
    }
    __builtin_npu_write_control_reg(N*N, 12); // For cosimulation purpose
  }

  __builtin_npu_barrier(43, CORE_NUMB * THREAD_NUMB - 1);

  return (int)&C;
#else
  bzero((void *) &C[0][0], N*N*sizeof(int));

  for (int i = 0; i < CORE_NUMB; i++) {
    for (int j = 0; j < THREAD_NUMB; j++) {
      matrix_mult(A, B, C, i, j);
    }
  }

  int pcount = 0;
  
  for (int i = 0; i < N; i++) {
    for (int j = 0; j < N; j++) {
      printf(" %7d", C[i][j]);
      pcount++;
      if (pcount % 16 == 0)
        printf("\n");
    }
  }

  return 0;
#endif
}

void matrix_mult(const int a[N][N], const int b[N][N], int mult[N][N], int core_id, int thread_id) {
  int start_loop = (core_id * N / CORE_NUMB) + thread_id;
  int end_loop = N / CORE_NUMB * (core_id + 1);
  
  for (int i = start_loop; i < end_loop; i += THREAD_NUMB){
    for (int j = 0; j < N; j++)
      for (int k = 0; k < N; k++)
        mult[i][j] += a[i][k] * b[k][j];
   }
};

void init_matrix(int a[N][N]){
    for (int i = 0; i < N; i++){
        for (int j = 0; j < N; j++){
            a[i][j] = (i*N + j) % N;
        }
    }
}
