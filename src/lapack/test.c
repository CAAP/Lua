#include <stdio.h>
#include <lapacke.h>

void print_matrix_rowmajor(char *desc, lapack_int m, lapack_int n, double *mat, lapack_int ldm) {
    lapack_int i,j;
    printf( "\n %s\n", desc );

    for (i = 0; i<m; i++) {
	for (j = 0; j<n; j++)
	    printf( " %6.2f", mat[i*ldm+j] );
	printf( "\n" );
    }
}

int main (int argc, const char * argv[]) {
    double a[5*3] = {1,1,1,2,3,4,3,5,2,4,2,5,5,4,3};
    double b[5*2] = {-10, -3, 12, 14, 14, 12, 16, 16, 18, 16};
    lapack_int info, m, n, lda, ldb, nrhs;

    m = 5;
    n = 3;
    nrhs = 2;
    lda = 3;
    ldb = 2;

    info = LAPACKE_dgels(LAPACK_ROW_MAJOR, 'N', m, n, nrhs, a, lda, b, ldb);

    print_matrix_rowmajor( "Solution", n, nrhs, b, ldb );

    return info;
}
