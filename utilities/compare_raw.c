#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

int sam_read_n_bytes( const char* filename, size_t n_bytes,           /* input  */
                      void*       buffer               )              /* output */
{
    FILE* f = fopen( filename, "r" );
    if( f == NULL )
    {
        fprintf( stderr, "Error! Cannot open input file: %s\n", filename );
        return 1;
    }
    fseek( f, 0, SEEK_END );
    if( ftell(f) < n_bytes )
    {
        fprintf( stderr, "Error! Input file size error: %s\n", filename );
        fprintf( stderr, "  Expecting %ld bytes, got %ld bytes.\n", n_bytes, ftell(f) );
        fclose( f );
        return 1;
    }
    fseek( f, 0, SEEK_SET );

    if( fread( buffer, 1, n_bytes, f ) != n_bytes )
    {
        fprintf( stderr, "Error! Input file read error: %s\n", filename );
        fclose( f );
        return 1;
    }

    fclose( f );
    return 0;
}

int sam_get_statsf( const float* arr1, const float* arr2, size_t len,/* input  */
                    float* rmse,       float* lmax,    float* psnr,  /* output */
                    float* arr1min,    float* arr1max            )   /* output */
{
    *arr1min  = arr1[0];
    *arr1max  = arr1[0];
    float sum = 0.0f, c = 0.0f;
    float diff, y, t;
    size_t  i;
    *lmax = 0.0f;
    for( i = 0; i < len; i++) 
    {
        /* Kahan summation */
        diff = fabsf( arr1[i] - arr2[i] ); /* single precision version of abs() */
        y    = diff * diff - c;
        t    = sum + y;
        c    = t - sum - y;
        sum  = t;

        /* detect the lmax*/
        if( diff > *lmax )
            *lmax = diff;

        /* detect min and max of arr1 */
        if( arr1[i]  < *arr1min )
            *arr1min = arr1[i];
        if( arr1[i]  > *arr1max )
            *arr1max = arr1[i];
    }

    sum  /= (float)len;
    *rmse = sqrtf( sum );   /* single precision version of sqrt() */

    float range2 = (*arr1max - *arr1min) * (*arr1max - *arr1min );
    *psnr = 10.0f * log10f( range2 / sum ); /* single precision version of log10() */
    
    return  0;
}

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
    long n_vals  = n_bytes / sizeof(float);

    float* buf1 = (float*) malloc( n_bytes );
    float* buf2 = (float*) malloc( n_bytes );

    sam_read_n_bytes( file1, n_bytes, buf1 );
    sam_read_n_bytes( file2, n_bytes, buf2 );

    float rmse, lmax, psnr, arr1min, arr1max;
    sam_get_statsf( buf1, buf2, n_vals, &rmse, &lmax, &psnr, &arr1min, &arr1max );
    printf("rmse = %e, lmax = %e, psnr = %f dB, orig_min = %f, orig_max = %f\n", 
            rmse, lmax, psnr, arr1min, arr1max );

    free( buf2 );
    free( buf1 );
}
