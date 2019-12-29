
#include "helper.h"

#include <math.h>
#include <stdio.h>

int sam_get_stats( const float* arr1, const float* arr2, long len, /* input  */
                   float* rmse,       float* lmax,    float* psnr, /* output */
                   float* arr1min,    float* arr1max            )  /* output */
{
    *arr1min  = arr1[0];
    *arr1max  = arr1[0];
    float sum = 0.0f, c = 0.0f;
    float diff, y, t;
    long  i;
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
    *psnr = -10.0f * log10f( sum / range2 ); /* single precision version of log10() */
    
    return  0;
}


int sam_read_n_bytes( const char* filename, long n_bytes,             /* input  */
                      void*       buffer               )              /* output */
{
    FILE* f = fopen( filename, "r" );
    if( f == NULL )
    {
        fprintf( stderr, "Error! Cannot open input file: %s\n", filename );
        return 1;
    }
    fseek( f, 0, SEEK_END );
    if( ftell(f) != n_bytes )
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

int sam_write_n_doubles( const char*   filename, long n_vals,
                         const double* buffer              )
{
    FILE* f = fopen( filename, "w" );
    if( f == NULL )
    {
        fprintf( stderr, "Error! Cannot open output file: %s\n", filename );
        return 1;
    }
    if( fwrite(buffer, sizeof(double), n_vals, f) != n_vals )
    {
        fprintf( stderr, "Error! Output file write error: %s\n", filename );
        fclose( f );
        return 1;
    }
    fclose( f );
    return 0;
}
