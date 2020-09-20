#include "SPECK3D_Decompressor.h"

#include <cstring>
#include <cassert>


void SPECK3D_Decompressor::copy_bitstream( const void* p, size_t len )
{
    m_stream_buf = speck::unique_malloc<uint8_t>( len );
    std::memcpy( m_stream_buf.get(), p, len );
    m_stream_buf_size = len;
}

    
void SPECK3D_Decompressor::take_bitstream( speck::buffer_type_raw buf, size_t len )
{
    m_stream_buf = std::move( buf );
    m_stream_buf_size = len;
}
    

auto SPECK3D_Decompressor::read_bitstream( const char* filename ) -> RTNType
{
    std::FILE* file = std::fopen( filename, "rb" );
    if( !file )
        return RTNType::IOError;

    std::fseek( file, 0, SEEK_END );
    const size_t file_size = std::ftell( file );
    std::fseek( file, 0, SEEK_SET );

    auto tmp_buf = speck::unique_malloc<uint8_t>( file_size );
    size_t nread  = std::fread( tmp_buf.get(), 1, file_size, file );
    std::fclose( file );
    if( nread != file_size )
        return RTNType::IOError;
    
    m_stream_buf = std::move( tmp_buf );
    m_stream_buf_size = file_size;

    return RTNType::Good;
}


auto SPECK3D_Decompressor::decompress() -> RTNType
{
    if( m_stream_buf == nullptr || m_stream_buf_size == 0 )
        return RTNType::Error;

    auto rtn = m_decoder.parse_encoded_bitstream( m_stream_buf.get(), m_stream_buf_size );
    if( rtn != RTNType::Good )
        return rtn;

    m_decoder.get_dims( m_dim_x, m_dim_y, m_dim_z );
    assert( m_dim_x > 1 && m_dim_y > 1 && m_dim_z > 1 );
    m_total_vals = m_dim_x * m_dim_y * m_dim_z;

    m_decoder.set_bit_budget( size_t(m_bpp * m_total_vals) );
    rtn = m_decoder.decode();
    if( rtn != RTNType::Good )
        return rtn;

    m_cdf.set_dims( m_dim_x, m_dim_y, m_dim_z );
    m_cdf.set_mean( m_decoder.get_image_mean() );
    auto coeffs = m_decoder.release_data();
    if( coeffs.first == nullptr || coeffs.second != m_total_vals )
        return RTNType::Error;
    m_cdf.take_data( std::move(coeffs.first), coeffs.second );
    m_cdf.idwt3d();

    return RTNType::Good;
}


auto SPECK3D_Decompressor::get_decompressed_volume_f() const
                           -> std::pair<speck::buffer_type_f, size_t>
{
    auto vol = m_cdf.get_read_only_data();
    if( vol.first == nullptr || vol.second != m_total_vals )
        return {nullptr, 0};

    auto out_buf = speck::unique_malloc<float>(m_total_vals);
    for( size_t i = 0; i < m_total_vals; i++ )
        out_buf[i] = vol.first[i];

    return {std::move(out_buf), m_total_vals};
}


auto SPECK3D_Decompressor::get_decompressed_volume_d() const
                           -> std::pair<speck::buffer_type_d, size_t>
{
    auto vol = m_cdf.get_read_only_data();
    if( vol.first == nullptr || vol.second != m_total_vals )
        return {nullptr, 0};
    
    auto out_buf  = speck::unique_malloc<double>(m_total_vals);
    std::memcpy( out_buf.get(), vol.first.get(), sizeof(double) * m_total_vals );

    return {std::move(out_buf), m_total_vals};
}


auto SPECK3D_Decompressor::write_volume_d( const char* filename ) const -> RTNType 
{
    // Get a read-only handle of the volume from m_cdf, and then write it to disk.
    auto vol = m_cdf.get_read_only_data( );
    if( vol.first == nullptr || vol.second != m_total_vals )
        return RTNType::Error;

    std::FILE* file = std::fopen( filename, "wb" );
    if( file ) {
        auto nwrite = std::fwrite( vol.first.get(), sizeof(double), m_total_vals, file );
        std::fclose( file );
        if( nwrite != m_total_vals )
            return RTNType::IOError;
        else
            return RTNType::Good;
    }
    else {
        return RTNType::IOError;
    }
}


auto SPECK3D_Decompressor::write_volume_f( const char* filename ) const -> RTNType
{
    // Need to get a volume represented as floats, then write it to disk.
    auto vol = get_decompressed_volume_f();
    if( vol.first == nullptr || vol.second != m_total_vals )
        return RTNType::Error;

    std::FILE* file = std::fopen( filename, "wb" );
    if( file ) {
        auto nwrite = std::fwrite( vol.first.get(), sizeof(float), m_total_vals, file );
        std::fclose( file );
        if( nwrite != m_total_vals )
            return RTNType::Error;
        else
            return RTNType::Good;
    }
    else {
        return RTNType::IOError;
    }
}


auto SPECK3D_Decompressor::set_bpp( float bpp ) -> RTNType
{
    if( bpp < 0.0 || bpp > 64.0 )
        return RTNType::InvalidParam;
    else {
        m_bpp = bpp;
        return RTNType::Good;
    }
}


void SPECK3D_Decompressor::get_volume_dims( size_t& x, size_t& y, size_t& z ) const
{
    x = m_dim_x;
    y = m_dim_y;
    z = m_dim_z;
}
