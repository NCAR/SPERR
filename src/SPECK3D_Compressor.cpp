#include "SPECK3D_Compressor.h"

#include <cassert>

SPECK3D_Compressor::SPECK3D_Compressor( size_t x, size_t y, size_t z )
                  : m_dim_x( x ), m_dim_y( y ), m_dim_z( z ),
                    m_total_vals( x * y * z )
{
    m_cdf.set_dims( x, y, z );
    m_encoder.set_dims( x, y, z );
}

template< typename T >
auto SPECK3D_Compressor::copy_data( const T* p, size_t len ) -> RTNType
{
    if( len != m_total_vals )
        return RTNType::WrongSize;

    static_assert(std::is_floating_point<T>::value,
                  "!! Only floating point values are supported !!");

    m_val_buf = speck::unique_malloc<double>( len );
    for( size_t i = 0; i < len; i++ )
        m_val_buf[i] = p[i];

    return RTNType::Good;
}
template auto SPECK3D_Compressor::copy_data( const double*, size_t ) -> RTNType;
template auto SPECK3D_Compressor::copy_data( const float*,  size_t ) -> RTNType;


auto SPECK3D_Compressor::take_data( speck::buffer_type_d buf, size_t len ) -> RTNType
{
    if( len != m_total_vals )
        return RTNType::WrongSize;

    m_val_buf = std::move( buf );

    return RTNType::Good;
}
 

auto SPECK3D_Compressor::read_floats( const char* filename ) -> RTNType
{
    std::FILE* file = std::fopen( filename, "rb" );
    if( !file )
        return RTNType::IOError;

    std::fseek( file, 0, SEEK_END );
    const size_t file_size = std::ftell( file );
    std::fseek( file, 0, SEEK_SET );
    if( file_size % 4 != 0 || file_size / 4 != m_total_vals ) {
        std::fclose( file );
        return RTNType::WrongSize;
    }

    auto tmp_buf = speck::unique_malloc<float>(   file_size / 4 );
    size_t nread  = std::fread( tmp_buf.get(), 4, file_size / 4, file );
    std::fclose( file );
    if( nread != file_size / 4)
        return RTNType::IOError;
    
    return( this->copy_data( tmp_buf.get(), file_size / 4 ) );
}


auto SPECK3D_Compressor::compress() -> RTNType
{
    if( m_val_buf == nullptr )
        return RTNType::Error;

    m_cdf.take_data( std::move(m_val_buf), m_total_vals );
    m_cdf.dwt3d();
    auto cdf_out = m_cdf.release_data();
    if( cdf_out.first == nullptr || cdf_out.second != m_total_vals )
        return RTNType::Error;

    m_encoder.set_image_mean( m_cdf.get_mean() );
    m_encoder.take_data( std::move(cdf_out.first), cdf_out.second );

#ifdef QZ_TERM
    m_encoder.set_quantization_term_level( m_qz_lev );
#else
    m_encoder.set_bit_budget( size_t(m_bpp * m_total_vals) );
#endif

    return (m_encoder.encode());
}


auto SPECK3D_Compressor::get_encoded_bitstream() const
                         -> std::pair<speck::buffer_type_raw, size_t>
{
    return (m_encoder.get_encoded_bitstream());
}
    

auto SPECK3D_Compressor::write_bitstream( const char* filename ) const -> RTNType
{
    auto stream = this->get_encoded_bitstream();
    if( stream.first == nullptr || stream.second == 0 )
        return RTNType::Error;

    std::FILE* file = std::fopen( filename, "wb" );
    if( file ) {
        auto nwrite = std::fwrite( stream.first.get(), 1, stream.second, file );
        std::fclose( file );
        if( nwrite != stream.second )
            return RTNType::IOError;
        else
            return RTNType::Good;
    }
    else {
        return RTNType::IOError;
    }
}


#ifdef QZ_TERM
void SPECK3D_Compressor::set_qz_level( int32_t q )
{
    m_qz_lev = q;
}
#else
auto SPECK3D_Compressor::set_bpp( float bpp ) -> RTNType
{
    if( bpp < 0.0 || bpp > 64.0 )
        return RTNType::InvalidParam;
    else {
        m_bpp = bpp;
        return RTNType::Good;
    }
}
#endif
