#include "SPECK_Storage.h"

#include <cassert>
#include <cstring>
#include <fstream>

template<typename T>
void speck::SPECK_Storage::copy_coeffs( const T& p, size_t len )
{
    assert( len > 0 );
    assert( m_coeff_len == 0 || m_coeff_len == len );
    m_coeff_len = len;
#ifdef SPECK_USE_DOUBLE
    m_coeff_buf = std::make_unique<double[]>( len );
#else
    m_coeff_buf = std::make_unique<float[]>( len );
#endif
    for( size_t i = 0; i < len; i++ )
        m_coeff_buf[i] = p[i];
}
template void speck::SPECK_Storage::copy_coeffs( const buffer_type_d&, size_t);
template void speck::SPECK_Storage::copy_coeffs( const buffer_type_f&, size_t);


#ifdef SPECK_USE_DOUBLE
    void speck::SPECK_Storage::take_coeffs( buffer_type_d coeffs, size_t len )
    {
        assert( len > 0 );
        assert( m_coeff_len == 0 || m_coeff_len == len );
        m_coeff_len = len;
        m_coeff_buf = std::move( coeffs );
    }
    void speck::SPECK_Storage::take_coeffs( buffer_type_f coeffs, size_t len )
    {   // cannot really take the coeffs if the data type doesn't match...
        copy_coeffs( std::move(coeffs), len );
    }
#else
    void speck::SPECK_Storage::take_coeffs( buffer_type_d coeffs, size_t len )
    {   // cannot really take the coeffs if the data type doesn't match...
        copy_coeffs( std::move(coeffs), len );
    }
    void speck::SPECK_Storage::take_coeffs( buffer_type_f coeffs, size_t len )
    {
        assert( len > 0 );
        assert( m_coeff_len == 0 || m_coeff_len == len );
        m_coeff_len = len;
        m_coeff_buf = std::move( coeffs );
    }
#endif


void speck::SPECK_Storage::copy_bitstream( const std::vector<bool>& stream )
{
    m_bit_buffer = stream;
}


void speck::SPECK_Storage::take_bitstream( std::vector<bool>& stream )
{
    m_bit_buffer.resize( 0 );
    std::swap( m_bit_buffer, stream );
}


const std::vector<bool>& speck::SPECK_Storage::get_read_only_bitstream() const
{
    return m_bit_buffer;
}


std::vector<bool>& speck::SPECK_Storage::release_bitstream()
{
    return m_bit_buffer;
}

#ifdef SPECK_USE_DOUBLE
    speck::buffer_type_d speck::SPECK_Storage::release_coeffs_double()
    {
        m_coeff_len = 0;
        return std::move( m_coeff_buf );
    }
    speck::buffer_type_f speck::SPECK_Storage::release_coeffs_float()
    {
        assert( m_coeff_len > 0 );
        buffer_type_f tmp = std::make_unique<float[]>( m_coeff_len );
        for( size_t i = 0; i < m_coeff_len; i++ )
            tmp[i] = m_coeff_buf[i];
        m_coeff_buf = nullptr;      // also destroy the current buffer
        return std::move( tmp );
    }
#else
    speck::buffer_type_d speck::SPECK_Storage::release_coeffs_double()
    {
        assert( m_coeff_len > 0 );
        buffer_type_d tmp = std::make_unique<double[]>( m_coeff_len );
        for( size_t i = 0; i < m_coeff_len; i++ )
            tmp[i] = m_coeff_buf[i];
        m_coeff_buf = nullptr;      // also destroy the current buffer
        return std::move( tmp );
    }
    speck::buffer_type_f speck::SPECK_Storage::release_coeffs_float()
    {
        m_coeff_len = 0;
        return std::move( m_coeff_buf );
    }
#endif


void speck::SPECK_Storage::set_image_mean( double mean )
{
    m_image_mean = mean;
}
double speck::SPECK_Storage::get_image_mean() const
{
    return m_image_mean;
}


int speck::SPECK_Storage::m_write( const buffer_type_c& header, size_t header_size,
                                   const char* filename                     ) const
{
    // Sanity check on the size of bit_buffer
    assert( m_bit_buffer.size() % 8 == 0 );

    // Allocate output buffer
    size_t total_size  = header_size + m_bit_buffer.size() / 8;
    buffer_type_c  buf = std::make_unique<char[]>( total_size );

    // Copy over header
    std::memcpy( buf.get(), header.get(), header_size );

    // Pack booleans to buf!
    const uint64_t magic = 0x8040201008040201;
    size_t   pos = header_size;
    bool     a[8];
    uint64_t t;
    for( size_t i = 0; i < m_bit_buffer.size(); i++ )
    {
        auto m = i % 8;
        a[m]   = m_bit_buffer[i];
        if( m == 7 )    // Need to pack 8 booleans!
        {
            std::memcpy( &t, a, 8 );
            buf[ pos++ ] = (magic * t) >> 56;
        }
    }

    // Write buf to a file.
    // Good introduction here: http://www.cplusplus.com/doc/tutorial/files/
    std::ofstream file( filename, std::ios::binary );
    if( file.is_open() )
    {
        file.write( buf.get(), total_size );
        file.close();
        return 0;
    }
    else
        return 1;
}


int speck::SPECK_Storage::m_read( buffer_type_c& header, size_t header_size,
                                  const char* filename )
{
    // Open a file and read its content
    std::ifstream file( filename, std::ios::binary );
    if( !file.is_open() )
        return 1;    
    file.seekg( 0, file.end );
    size_t total_size = file.tellg();
    file.seekg( 0, file.beg );
    buffer_type_c buf = std::make_unique<char[]>( total_size );
    file.read( buf.get(), total_size );
    file.close();

    // Copy over the header
    std::memcpy( header.get(), buf.get(), header_size );

    // Now interpret the booleans
    size_t num_of_bools = (total_size - header_size) * 8;
    m_bit_buffer.clear();
    m_bit_buffer.resize( num_of_bools );
    const uint64_t magic = 0x8040201008040201;
    const uint64_t mask  = 0x8080808080808080;
    size_t   pos = header_size;
    bool     a[8];
    uint64_t t;
    uint8_t  b;
    for( size_t i = 0; i < num_of_bools; i += 8 )
    {
        b = buf[ pos++ ];
        t = ((magic * b) & mask) >> 7;
        std::memcpy( a, &t, 8 );
        for( size_t j = 0; j < 8; j++ )
            m_bit_buffer[ i + j ] = a[j];
    }

    return 0;
}



