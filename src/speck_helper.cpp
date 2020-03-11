#include "speck_helper.h"

#include <cmath>
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <memory>


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
int speck::output_speck2d( size_t dim_x, size_t dim_y, double mean, uint16_t max_coeff_bits,
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
    std::unique_ptr<uint8_t[]> buf = std::make_unique<uint8_t[]>( total_size );
    uint8_t* const bufptr = buf.get();
    std::memcpy( bufptr + pos, dims,  sizeof(dims) );      pos += sizeof(dims);
    std::memcpy( bufptr + pos, &mean, sizeof(mean) );      pos += sizeof(mean);
    std::memcpy( bufptr + pos, &max_coeff_bits, sizeof(max_coeff_bits) );
    pos += sizeof(max_coeff_bits);

    // Pack booleans to buf!
    const uint64_t magic = 0x8040201008040201;
    bool  a[8];
    const uint64_t* const a_ptr = reinterpret_cast<uint64_t*>(a);
    for( size_t i = 0; i < bit_buffer.size(); i++ )
    {
        auto m = i % 8;
        a[m]  = bit_buffer[i];
        if( m == 7 )    // Need to pack 8 booleans!
        {
            //uint64_t t   = *((uint64_t*)a);
            uint64_t t   = *a_ptr;
            uint8_t  c   = (magic * t) >> 56;
            bufptr[ pos++ ] = c;
        }
    }

    // Write buf to a file.
    // Good introduction here: http://www.cplusplus.com/doc/tutorial/files/
    std::ofstream file( filename, std::ios::binary );
    if( file.is_open() )
    {
        file.write( reinterpret_cast<const char*>(buf.get()), total_size );
        file.close();
        return 0;
    }
    else
        return 1;
}


int speck::input_speck2d( size_t& dim_x, size_t& dim_y, double& mean, uint16_t& max_coeff_bits,
                           std::vector<bool>& bit_buffer, const char* filename )
{
    // The header format need to be kept in sync with the output routine.
    uint32_t dims[2];
    dims[0] = uint32_t(dim_x);
    dims[1] = uint32_t(dim_y);
    double    my_mean     = 0.0;
    uint16_t  my_max_bits = 0;

    // Open a file and read its content
    std::ifstream file( filename, std::ios::binary );
    if( !file.is_open() )
        return 1;    
    file.seekg( 0, file.end );
    size_t total_size = file.tellg();
    file.seekg( 0, file.beg );
    std::unique_ptr<uint8_t[]> buf = std::make_unique<uint8_t[]>( total_size );
    file.read( reinterpret_cast<char*>(buf.get()), total_size );
    file.close();

    // Now interpret the header
    size_t pos = 0;
    const uint8_t* const bufptr  = buf.get();
    std::memcpy( dims,          bufptr + pos, sizeof(dims));        pos += sizeof(dims);
    std::memcpy( &my_mean,      bufptr + pos, sizeof(my_mean));     pos += sizeof(my_mean);
    std::memcpy( &my_max_bits,  bufptr + pos, sizeof(my_max_bits)); pos += sizeof(my_max_bits);
    dim_x          = dims[0];
    dim_y          = dims[1];
    mean           = my_mean;
    max_coeff_bits = my_max_bits;

    // Now interpret the booleans
    size_t num_bools = (total_size - pos) * 8;
    bit_buffer.clear();
    bit_buffer.resize( num_bools );
    const uint64_t magic = 0x8040201008040201;
    const uint64_t mask  = 0x8080808080808080;
    bool  a[8];
    uint64_t* const a_ptr = reinterpret_cast<uint64_t*>(a);
    for( size_t i = 0; i < num_bools; i += 8 )
    {
        uint8_t b        = buf[ pos++ ];
        //*((uint64_t*)a)  = ((magic * b) & mask) >> 7;
        *a_ptr           = ((magic * b) & mask) >> 7;
        for( size_t j = 0; j < 8; j++ )
            bit_buffer[ i + j ] = a[j];
    }

    return 0;
}


int speck::output_speck2d( size_t dim_x, size_t dim_y, double mean, uint16_t max_coeff_bits,
                           const std::vector<bool>& bit_buffer, const std::string& filename )
{
    return output_speck2d( dim_x, dim_y, mean, max_coeff_bits, bit_buffer, filename.c_str() );
}
int speck::input_speck2d( size_t& dim_x, size_t& dim_y, double& mean, uint16_t& max_coeff_bits,
                           std::vector<bool>& bit_buffer, const std::string& filename )
{
    return input_speck2d( dim_x, dim_y, mean, max_coeff_bits, bit_buffer, filename.c_str() );
}
