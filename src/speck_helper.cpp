#include "speck_helper.h"

#include <cmath>
#include <cassert>


long speck::calc_num_of_xform_levels( long len )
{
    assert( len > 0 );
    // I decide 8 is the minimal length to do one level of xform.
    float f      = std::log2( float(len) / 8.0f ); 
    long num_of_levs = f < 0.0f ? 0 : long(f) + 1;

    return num_of_levs;
}
    

long speck::calc_approx_detail_len( long  orig_len,   long  lev,
                                    long& approx_len, long& detail_len )
{
    assert( orig_len > 0 || lev >= 0 );
    long low_len = orig_len, new_low;
    long high_len = 0;
    for( long i = 0; i < lev; i++ )
    {
        new_low  = low_len % 2 == 0 ? low_len / 2 : (low_len + 1) / 2;
        high_len = low_len - new_low;
        low_len  = new_low;
    }
    
    approx_len = low_len;
    detail_len = high_len;

    return low_len;
}
