#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <float.h>
#include <math.h>
#include <sys/time.h>
#include <pthread.h>
#include <unistd.h>
#include <mkl.h>
double *A;
double *B;
double *C;
int thread_nums; 
int n;
/* Helper functions */

double read_timer( )
{
    static bool initialized = false;
    static struct timeval start;
    struct timeval end;
    if( !initialized )
    {
        gettimeofday( &start, NULL );
        initialized = true;
    }

    gettimeofday( &end, NULL );

    return (end.tv_sec - start.tv_sec) + 1.0e-6 * (end.tv_usec - start.tv_usec);
}

void fill( double *p, int n )
{
    for( int i = 0; i < n; i++ )
        p[i] = 2 * drand48( ) - 1;
}

void absolute_value( double *p, int n )
{
    for( int i = 0; i < n; i++ )
        p[i] = fabs( p[i] );
}

void *worker(void *args) {
    double sum;
    int  i, j, tId, portion, rowS, rowE;
    tId = *(int *)(args); //get the thread ID
    portion  = n / thread_nums;
    rowS = tId * portion;
    rowE = (tId + 1) * portion;

    for ( i = rowS; i < rowE; i++ ) {
        for ( j = 0; j < n; j++ ) {
            sum = C[i + j * n];
	    for (int k = 0; k < n; k++) {
		sum += A[i + k * n] * B[k + j * n];
	    }
     	    C[i + j * n] = sum;
	}
    }
}    

pthread_t * threads;

void square_dgemm( int n, double *A, double *B, double *C ) {
	for (int i = 0; i < thread_nums; i++) {
		int *tId;
		tId = (int *)malloc(sizeof(int));
		*tId = i;
    		pthread_create(&threads[i], NULL, worker, (void*)tId);
	}
	for (int i = 0; i < thread_nums; i++) {
		pthread_join(threads[i], NULL);
	}
}

/* The benchmarking program */

int main( int argc, char **argv )
{   
    thread_nums = atoi(argv[1]);
    threads = (pthread_t *) malloc( thread_nums * sizeof(pthread_t) );

    int test_sizes[] = {
	1600    
    };
 
	/*For each test size*/
    for( int isize = 0; isize < sizeof(test_sizes)/sizeof(test_sizes[0]); isize++ )
    {
	/*Craete and fill 3 random matrices A,B,C*/
        n = test_sizes[isize];

        A = (double*) malloc( n * n * sizeof(double) );
        B = (double*) malloc( n * n * sizeof(double) );
        C = (double*) malloc( n * n * sizeof(double) );

        fill( A, n * n );
        fill( B, n * n );
        fill( C, n * n );
        
        /*  measure Mflop/s rate; time a sufficiently long sequence of calls to eliminate noise*/
        double Mflop_s, seconds = -1.0;
        for( int n_iterations = 1; seconds < 0.1; n_iterations *= 2 ) 
        {
            /* warm-up */
            square_dgemm( n, A, B, C );
            
            /*  measure time */
            seconds = read_timer( );
            for( int i = 0; i < n_iterations; i++ )
                square_dgemm( n, A, B, C );
            seconds = read_timer( ) - seconds;
           
            /*  compute Mflop/s rate */
            Mflop_s = 2e-6 * n_iterations * n * n * n / seconds;
        }
        printf ("Size: %d\tMflop/s: %g\n", n, Mflop_s);
        
        /*  Ensure that error does not exceed the theoretical error bound */
		
		/* Set initial C to 0 and do matrix multiply of A*B */
        memset( C, 0, sizeof( double ) * n * n );
        square_dgemm( n, A, B, C );
		/*Subtract A*B from C using standard dgemm (note that this should be 0 to within machine roundoff)*/
	const char *ntran = "N";
        double one = 1.0;	
	double negone = -1.0;
	double dbl_eps = -3.0*DBL_EPSILON*n;

 
        dgemm( ntran, ntran, &n,&n,&n, &negone, A, &n, B, &n, &one, C, &n );
		/*Subtract the maximum allowed roundoff from each element of C*/
        absolute_value( A, n * n );
        absolute_value( B, n * n );
        absolute_value( C, n * n );
        dgemm( ntran, ntran, &n,&n,&n, &dbl_eps, A, &n, B, &n, &one, C, &n );
		/*After this test if any element in C is still positive something went wrong in square_dgemm*/
        for( int i = 0; i < n * n; i++ )
            if( C[i] > 0 )
            {
                printf( "FAILURE: error in matrix multiply exceeds an acceptable margin\n" );
                exit(-1);
            }

		/*Deallocate memory*/
        free( C );
        free( B );
        free( A );
    }
    
    return 0;
}
