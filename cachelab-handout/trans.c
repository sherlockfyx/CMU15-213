/* 
 * trans.c - Matrix transpose B = A^T
 *
 * Each transpose function must have a prototype of the form:
 * void trans(int M, int N, int A[N][M], int B[M][N]);
 *
 * A transpose function is evaluated by counting the number of misses
 * on a 1KB direct mapped cache with a block size of 32 bytes.
 */ 
#include <stdio.h>
#include <stdlib.h>
#include "cachelab.h"

int is_transpose(int M, int N, int A[N][M], int B[M][N]);
void transpose11(int M, int N, int A[N][M], int B[M][N]);
void transpose22(int M, int N, int A[N][M], int B[M][N]);
void transpose33(int M, int N, int A[N][M], int B[M][N]);

/* 
 * transpose_submit - This is the solution transpose function that you
 *     will be graded on for Part B of the assignment. Do not change
 *     the description string "Transpose submission", as the driver
 *     searches for that string to identify the transpose function to
 *     be graded. 
 */
char transpose_submit_desc[] = "Transpose submission";
void transpose_submit(int M, int N, int A[N][M], int B[M][N])
{
    if(M == 32)
		transpose11(32, 32, A, B);
	else if(M == 64)
		transpose22(64, 64, A, B);
	else if(M == 61)
		transpose33(M, N, A, B);
    return;
}

/* 
 * You can define additional transpose functions below. We've defined
 * a simple one below to help you get started. 
 */ 
char transpose1[] = "Transpose 1";
void transpose11(int M, int N, int A[N][M], int B[M][N])
{
	int i, j, ii, jj;
	int val1, val2, val3, val4, val5, val6, val7, val8;
	for(i = 0; i < 32; i += 8){
		for(j = 0; j < 32; j += 8){
			// for(ii = i; ii < i + 8; ii++){
			// 		val1 = A[ii][j+0];
			// 		val2 = A[ii][j+1];
			// 		val3 = A[ii][j+2];
			// 		val4 = A[ii][j+3];
			// 		val5 = A[ii][j+4];
			// 		val6 = A[ii][j+5];
			// 		val7 = A[ii][j+6];
			// 		val8 = A[ii][j+7];
			// 		B[j+0][ii] = val1;
			// 		B[j+1][ii] = val2;
			// 		B[j+2][ii] = val3;
			// 		B[j+3][ii] = val4;
			// 		B[j+4][ii] = val5;
			// 		B[j+5][ii] = val6;
			// 		B[j+6][ii] = val7;
			// 		B[j+7][ii] = val8;	
			// }

			for (ii = i, jj = j; ii < i + 8; ii++, jj++) {
				val1 = A[ii][j + 0];
				val2 = A[ii][j + 1];
				val3 = A[ii][j + 2];
				val4 = A[ii][j + 3];
				val5 = A[ii][j + 4];
				val6 = A[ii][j + 5];
				val7 = A[ii][j + 6];
				val8 = A[ii][j + 7];
				B[jj][i + 0] = val1;
				B[jj][i + 1] = val2;
				B[jj][i + 2] = val3;
				B[jj][i + 3] = val4;
				B[jj][i + 4] = val5;
				B[jj][i + 5] = val6;
				B[jj][i + 6] = val7;
				B[jj][i + 7] = val8;
			}
			// transpose
			for (ii = 0; ii < 8; ii++) {
				for (jj = ii + 1; jj < 8; jj++) {
					val1 = B[j + ii][i + jj];
					B[j + ii][i + jj] = B[j + jj][i + ii];
					B[j + jj][i + ii] = val1;
				}
			}
		}
	}
}

char transpose2[] = "Transpose 2";
void transpose22(int M, int N, int A[N][M], int B[M][N]) {
	int i, j, ii, tmp;
	int a0, a1, a2, a3, a4, a5, a6, a7;
	for (i = 0; i < N; i += 8) {
		for (j = 0; j < M; j += 8) {	//分块
			for (ii = 0; ii < 8 / 2; ii++) {	//line
				// A top left
				a0 = A[ii + i][j + 0];
				a1 = A[ii + i][j + 1];
				a2 = A[ii + i][j + 2];
				a3 = A[ii + i][j + 3];

				// copy
				// A top right
				a4 = A[ii + i][j + 4];
				a5 = A[ii + i][j + 5];
				a6 = A[ii + i][j + 6];
				a7 = A[ii + i][j + 7];

				// B top left
				B[j + 0][ii + i] = a0;
				B[j + 1][ii + i] = a1;
				B[j + 2][ii + i] = a2;
				B[j + 3][ii + i] = a3;

				// copy
				// B top right
				B[j + 0][ii + 4 + i] = a4;
				B[j + 1][ii + 4 + i] = a5;
				B[j + 2][ii + 4 + i] = a6;
				B[j + 3][ii + 4 + i] = a7;
			}

			for (ii = 0; ii < 8 / 2; ii++) {
				// step 1 2
				a0 = A[i + 4][j + ii], a4 = A[i + 4][j + ii + 4];
				a1 = A[i + 5][j + ii], a5 = A[i + 5][j + ii + 4];
				a2 = A[i + 6][j + ii], a6 = A[i + 6][j + ii + 4];
				a3 = A[i + 7][j + ii], a7 = A[i + 7][j + ii + 4];
				// step 3
				tmp = B[j + ii][i + 4], B[j + ii][i + 4] = a0, a0 = tmp;
				tmp = B[j + ii][i + 5], B[j + ii][i + 5] = a1, a1 = tmp;
				tmp = B[j + ii][i + 6], B[j + ii][i + 6] = a2, a2 = tmp;
				tmp = B[j + ii][i + 7], B[j + ii][i + 7] = a3, a3 = tmp;
				// step 4
				B[j + ii + 4][i + 0] = a0, B[j + ii + 4][i + 4 + 0] = a4;
				B[j + ii + 4][i + 1] = a1, B[j + ii + 4][i + 4 + 1] = a5;
				B[j + ii + 4][i + 2] = a2, B[j + ii + 4][i + 4 + 2] = a6;
				B[j + ii + 4][i + 3] = a3, B[j + ii + 4][i + 4 + 3] = a7;
			}
		}
	}
}

char transpose3[] = "Transpose 3";
void transpose33(int M, int N, int A[N][M], int B[M][N]) {
	int i, j, jj, ii;
	int a0, a1, a2, a3, a4, a5, a6, a7;
	for (i = 0; i < N; i += 8) {
		for (j = 0; j < M; j += 23) {
			if (i + 8 <= N && j + 23 <= M) {
				for (jj = j; jj < j + 23; jj++) {
					a0 = A[i + 0][jj];
					a1 = A[i + 1][jj];
					a2 = A[i + 2][jj];
					a3 = A[i + 3][jj];
					a4 = A[i + 4][jj];
					a5 = A[i + 5][jj];
					a6 = A[i + 6][jj];
					a7 = A[i + 7][jj];
					B[jj][i + 0] = a0;
					B[jj][i + 1] = a1;
					B[jj][i + 2] = a2;
					B[jj][i + 3] = a3;
					B[jj][i + 4] = a4;
					B[jj][i + 5] = a5;
					B[jj][i + 6] = a6;
					B[jj][i + 7] = a7;
				}
			} else { //不规则部分
				for (ii = i; ii < i + 8 && ii < N; ii++) {
					for (jj = j; jj < j + 23 && jj < M; jj++) {
						B[jj][ii] = A[ii][jj];
						// a0 = A[ii][jj];
						// B[jj][ii] = a0;
					}
				}
			}
		}
	}
}

/* 
 * trans - A simple baseline transpose function, not optimized for the cache.
 */
char trans_desc[] = "Simple row-wise scan transpose";
void trans(int M, int N, int A[N][M], int B[M][N])
{
    int i, j, tmp;

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; j++) {
            tmp = A[i][j];
            B[j][i] = tmp;
        }
    }    

}

/*
 * registerFunctions - This function registers your transpose
 *     functions with the driver.  At runtime, the driver will
 *     evaluate each of the registered functions and summarize their
 *     performance. This is a handy way to experiment with different
 *     transpose strategies.
 */
void registerFunctions()
{
    /* Register your solution function */
    registerTransFunction(transpose_submit, transpose_submit_desc); 

    /* Register any additional transpose functions */
    registerTransFunction(trans, trans_desc); 
    registerTransFunction(transpose11, transpose1);
	registerTransFunction(transpose22, transpose2);
	registerTransFunction(transpose33, transpose3);

}

/* 
 * is_transpose - This helper function checks if B is the transpose of
 *     A. You can check the correctness of your transpose by calling
 *     it before returning from the transpose function.
 */
int is_transpose(int M, int N, int A[N][M], int B[M][N])
{
    int i, j;

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; ++j) {
            if (A[i][j] != B[j][i]) {
                return 0;
            }
        }
    }
    return 1;
}

