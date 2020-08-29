#include "SPECK2D_Compressor.h"

SPECK2D_Compressor::SPECK2D_Compressor( size_t x, size_t y )
                  : m_dim_x( x ), m_dim_y( y ), m_total_vals( x * y )
{
    m_cdf.set_dims( x, y );
    m_encoder.set_dims( x, y );
}

template< typename T >
auto SPECK2D_Compressor::copy_data( const T* p, size_t len ) -> int
{
    if( len != m_total_vals )
        return 1;

    static_assert(std::is_floating_point<T>::value,
                  "!! Only floating point values are supported !!");

    m_val_buf = speck::unique_malloc<double>( len );
    for( size_t i = 0; i < len; i++ )
        m_val_buf[i] = p[i];

    return 0;
}
template auto SPECK2D_Compressor::copy_data( const double*, size_t ) -> int;
template auto SPECK2D_Compressor::copy_data( const float*,  size_t ) -> int;


    
auto SPECK2D_Compressor::take_data( speck::buffer_type_d buf, size_t len ) -> int
{
    if( len != m_total_vals )
        return 1;

    m_val_buf = std::move( buf );

    return 0;
}
    

auto SPECK2D_Compressor::read_floats( const char* filename ) -> int
{
    std::FILE* file = std::fopen( filename, "rb" );
    if( !file )
        return 1;

    std::fseek( file, 0, SEEK_END );
    const size_t file_size = std::ftell( file );
    std::fseek( file, 0, SEEK_SET );
    if( file_size % 4 != 0 || file_size / 4 != m_total_vals ) {
        std::fclose( file );
        return 1;
    }

    auto tmp_buf = speck::unique_malloc<float>(   file_size / 4 );
    size_t nread  = std::fread( tmp_buf.get(), 4, file_size / 4, file );
    std::fclose( file );
    if( nread != file_size / 4)
        return 1;
    
    return( this->copy_data( tmp_buf.get(), file_size / 4 ) );
}


auto SPECK2D_Compressor::compress() -> int
{
    if( m_val_buf == nullptr )
        return 1;

    m_cdf.take_data( std::move(m_val_buf), m_total_vals );
    m_cdf.dwt2d();

    m_encoder.set_image_mean( m_cdf.get_mean() );
    m_encoder.take_coeffs( m_cdf.release_data(), m_total_vals );

#ifdef QZ_TERM
    // m_encoder.set_quantization_term_level( m_qz_lev );
#else
    m_encoder.set_bit_budget( m_bpp * m_total_vals );
#endif

    int rtn = m_encoder.encode();
    return rtn;
}


auto SPECK2D_Compressor::get_compressed_buffer( speck::buffer_type_raw& out_buf, 
                                                size_t& out_size ) const -> int
{
    int rtn = m_encoder.get_compressed_buffer( out_buf, out_size );
    return rtn;
}
    

auto SPECK2D_Compressor::write_bitstream( const char* filename ) const -> int
{
    speck::buffer_type_raw out_buf;
    size_t out_size;
    int rtn = this->get_compressed_buffer( out_buf, out_size );
    if( rtn != 0 )
        return rtn;

    std::FILE* file = std::fopen( filename, "wb" );
    if( file ) {
        std::fwrite( out_buf.get(), 1, out_size, file );
        std::fclose( file );
        return 0;
    }
    else {
        return 1;
    }
}


#ifdef QZ_TERM
// void SPECK2D_Compressor::set_qz_level( int32_t q )
// {
//     m_qz_lev = q;
// }
#else
auto SPECK2D_Compressor::set_bpp( float bpp ) -> int
{
    if( bpp < 0.0 || bpp > 64.0 )
        return 1;
    else {
        m_bpp = bpp;
        return 0;
    }
}
#endif
