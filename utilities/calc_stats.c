
#include "calc_stats.h"

#include "math.h"

int get_rmse_max( const float* arr1, const float* arr2, long len, /* input */
                  float* rmse,       float* max               )   /* output */
{
    float sum = 0.0f, c = 0.0f;
    float diff, y, t;
    long  i;
    *max  = 0.0f;
    for( i = 0; i < len; i++) 
    {
        /* Kahan summation */
        diff = fabsf( arr1[i] - arr2[i] ); /* single precision version */
        y    = diff * diff - c;
        t    = sum + y;
        c    = t - sum - y;
        sum  = t;

        /* detect the max */
        if( diff > *max )
            *max = diff;
    }

    sum  /= (float)len;
    *rmse = sqrtf( sum );   /* single precision version */
    
    return  0;
}

