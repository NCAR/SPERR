
#include "helper.h"

#include <math.h>
#include <stdio.h>

int get_rmse_lmax( const float* arr1, const float* arr2, long len, /* input  */
                   float* rmse,       float* lmax,                 /* output */
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
        diff = fabsf( arr1[i] - arr2[i] ); /* single precision version */
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
    *rmse = sqrtf( sum );   /* single precision version */
    
    return  0;
}


int read_n_bytes( const char* filename, long n_bytes,             /* input  */
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
