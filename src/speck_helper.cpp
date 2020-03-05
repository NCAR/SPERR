#include "speck_helper.h"

#include <cmath>
#include <cassert>


size_t speck::calc_num_of_xforms( size_t len )
{
    assert( len > 0 );
    // I decide 8 is the minimal length to do one level of xform.
    float f      = std::log2( float(len) / 8.0f ); 
    size_t num_of_xforms = f < 0.0f ? 0 : size_t(f) + 1;

    return num_of_xforms;
}
    

void speck::calc_approx_detail_len( size_t orig_len,  size_t lev,
                         std::array<size_t, 2>& approx_detail_len )
{
    size_t low_len  = orig_len; 
    size_t high_len = 0;
    size_t new_low;
    for( size_t i = 0; i < lev; i++ )
    {
        new_low  = low_len % 2 == 0 ? low_len / 2 : (low_len + 1) / 2;
        high_len = low_len - new_low;
        low_len  = new_low;
    }
    
    approx_detail_len[0] = low_len;
    approx_detail_len[1] = high_len;
}


double speck::make_coeff_positive( double* buf, size_t len, std::vector<bool>& sign_array )
{
    sign_array.assign( len, true );
    double max = std::abs( buf[0] );
    for( size_t i = 0; i < len; i++ )
    {
        if( buf[i] < 0.0 )
        {
            buf[i]       *= -1.0;
            sign_array[i] = false;
        }
        if( buf[i] > max )
            max = buf[i];
    }

    return max;
}
