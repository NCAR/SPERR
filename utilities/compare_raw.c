#include "helper.h"

#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>

typedef double FLOAT;

int main( int argc, char* argv[] )
{
    if( argc != 3 )
    {
        printf("Usage: ./a.out file1 file2\n");
        return 1;
    }

    const char* file1 = argv[1];
    const char* file2 = argv[2];

    struct stat st1, st2;
    stat( file1, &st1 );
    stat( file2, &st2 );
    if( st1.st_size != st2.st_size )
    {
        printf("Two files have different sizes!\n");
        return 1;
    } 

    long n_bytes = st1.st_size;
    long n_vals  = n_bytes / sizeof(FLOAT);

    FLOAT* buf1 = (FLOAT*) malloc( n_bytes );
    FLOAT* buf2 = (FLOAT*) malloc( n_bytes );

    sam_read_n_bytes( file1, n_bytes, buf1 );
    sam_read_n_bytes( file2, n_bytes, buf2 );

    FLOAT rmse, lmax, psnr, arr1min, arr1max;
    /*sam_get_statsf( buf1, buf2, n_vals, &rmse, &lmax, &psnr, &arr1min, &arr1max );*/
    sam_get_statsd( buf1, buf2, n_vals, &rmse, &lmax, &psnr, &arr1min, &arr1max );
    printf("rmse = %f, lmax = %f, psnr = %fdB, orig_min = %f, orig_max = %f\n", 
            rmse, lmax, psnr, arr1min, arr1max );

    free( buf2 );
    free( buf1 );
}
