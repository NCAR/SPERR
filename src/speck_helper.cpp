#include "speck_helper.h"

#include <cmath>
#include <cassert>
#include <cstdlib>
#include <cstring>


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

// Good solution to deal with bools and unsigned chars
// https://stackoverflow.com/questions/8461126/how-to-create-a-byte-out-of-8-bool-values-and-vice-versa
int speck::encode_header_speck2d( size_t dim_x, size_t dim_y, double mean, uint16_t max_coeff_bits,
                                  const std::vector<bool>& bit_buffer, const char* filename )
{
    // Sanity check on the size of bit_buffer
    assert( bit_buffer.size() % 8 == 0 );
    // Follow the output specification to use uint32_t to represent dimensions
    uint32_t dims[2];
    dims[0] = uint32_t(dim_x);
    dims[1] = uint32_t(dim_y);

    size_t header_size = sizeof(dims) + sizeof(mean) + sizeof(max_coeff_bits);
    size_t total_size  = header_size + bit_buffer.size() / 8;

    // Copy over header values 
    size_t pos = 0;
    unsigned char* buf = new unsigned char[ total_size ];
    std::memcpy( buf + pos, dims,  sizeof(dims) );      pos += sizeof(dims);
    std::memcpy( buf + pos, &mean, sizeof(mean) );      pos += sizeof(mean);
    std::memcpy( buf + pos, &max_coeff_bits, sizeof(max_coeff_bits) );
    pos += sizeof(max_coeff_bits);

    // Pack booleans to buf!
    const uint64_t magic = 0x8040201008040201;
    bool  a[8];
    for( size_t i = 0; i < bit_buffer.size(); i++ )
    {
        auto m = i % 8;
        a[m]  = bit_buffer[i];
        if( m == 7 )    // Need to pack 8 booleans!
        {
            uint64_t t   = *((uint64_t*)a);
            uint8_t  c   = (magic * t) >> 56;
            buf[ pos++ ] = c;
        }
    }

    delete[] buf;

    return 0;
}

