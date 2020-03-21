#include "SPECK_Storage.h"
#include <type_traits>
#include <cassert>

template<typename T>
void speck::SPECK_Storage::copy_coeffs( const T p, size_t len )
{
    assert( len > 0 );

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
template void speck::SPECK_Storage::copy_coeffs( const double*, size_t);
template void speck::SPECK_Storage::copy_coeffs( const float*,  size_t);


#ifdef SPECK_USE_DOUBLE
    void speck::SPECK_Storage::take_coeffs( buffer_type_d coeffs, size_t len )
    {
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
        return std::move( tmp );
    }
#else
    speck::buffer_type_d speck::SPECK_Storage::release_coeffs_double()
    {
        assert( m_coeff_len > 0 );
        buffer_type_d tmp = std::make_unique<double[]>( m_coeff_len );
        for( size_t i = 0; i < m_coeff_len; i++ )
            tmp[i] = m_coeff_buf[i];
        return std::move( tmp );
    }
    speck::buffer_type_f speck::SPECK_Storage::release_coeffs_float()
    {
        m_coeff_len = 0;
        return std::move( m_coeff_buf );
    }
#endif
