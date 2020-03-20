#include "SPECK_Storage.h"


void speck::SPECK_Storage::take_coeffs( std::unique_ptr<double[]> ptr )
{
    m_coeff_buf = std::move( ptr );
}

template<typename T>
void speck::SPECK_Storage::copy_coeffs( const T* p, size_t len )
{
    static_assert( std::is_floating_point<T>::value, 
                   "!! Only floating point values are supported !!" );
    assert( len > 0 );

    m_coeff_buf = std::make_unique<double[]>( len );
    for( size_t i = 0; i < len; i++ )
        m_coeff_buf[i] = p[i];
}
template void speck::SPECK_Storage::copy_coeffs( const float*,  size_t );
template void speck::SPECK_Storage::copy_coeffs( const double*, size_t);


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


const speck::buffer_type& speck::SPECK_Storage::get_read_only_coeffs() const
{
    return m_coeff_buf;
}


std::unique_ptr<double[]> speck::SPECK_Storage::release_coeffs()
{
    return std::move( m_coeff_buf );
}

