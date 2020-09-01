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
    

auto SPECK3D_Decompressor::read_bitstream( const char* filename ) -> int
{
    std::FILE* file = std::fopen( filename, "rb" );
    if( !file )
        return 1;

    std::fseek( file, 0, SEEK_END );
    const size_t file_size = std::ftell( file );
    std::fseek( file, 0, SEEK_SET );

    auto tmp_buf = speck::unique_malloc<uint8_t>( file_size );
    size_t nread  = std::fread( tmp_buf.get(), 1, file_size, file );
    std::fclose( file );
    if( nread != file_size )
        return 1;
    
    m_stream_buf = std::move( tmp_buf );
    m_stream_buf_size = file_size;

    return 0;
}


auto SPECK3D_Decompressor::decompress() -> int
{
    if( m_stream_buf == nullptr )
        return 1;

    m_decoder.read_compressed_buffer( m_stream_buf.get(), m_stream_buf_size );
    m_decoder.get_dims( m_dim_x, m_dim_y, m_dim_z );
    m_total_vals = m_dim_x * m_dim_y * m_dim_z;
    m_decoder.set_bit_budget( size_t(m_bpp * m_total_vals) );
    int rtn = m_decoder.decode();
    if( rtn != 0 )
        return rtn;

    m_cdf.set_dims( m_dim_x, m_dim_y, m_dim_z );
    m_cdf.set_mean( m_decoder.get_image_mean() );
    m_cdf.take_data( m_decoder.release_coeffs_double(), m_total_vals );
    m_cdf.idwt3d();

    return 0;
}


auto SPECK3D_Decompressor::get_decompressed_volume_f( 
                           speck::buffer_type_f& out_buf, size_t& out_size ) const -> int
{
    out_buf  = speck::unique_malloc<float>(m_total_vals);
    out_size = m_total_vals;

    const auto& vol = m_cdf.get_read_only_data();
    for( size_t i = 0; i < m_total_vals; i++ )
        out_buf[i] = vol[i];

    return 0;
}


auto SPECK3D_Decompressor::get_decompressed_volume_d( 
                           speck::buffer_type_d& out_buf, size_t& out_size ) const -> int
{
    out_buf  = speck::unique_malloc<double>(m_total_vals);
    out_size = m_total_vals;

    std::memcpy( m_cdf.get_read_only_data().get(), out_buf.get(), 
                 sizeof(double) * m_total_vals );
    return 0;
}


auto SPECK3D_Decompressor::write_volume_d( const char* filename ) const -> int
{
    // Get a read-only handle of the volume from m_cdf, and then write it to disk.
    const auto& vol = m_cdf.get_read_only_data();
    if( vol == nullptr )
        return 1;

    std::FILE* file = std::fopen( filename, "wb" );
    if( file ) {
        std::fwrite( vol.get(), sizeof(double), m_total_vals, file );
        std::fclose( file );
        return 0;
    }
    else {
        return 1;
    }

return 0;
}


auto SPECK3D_Decompressor::write_volume_f( const char* filename ) const -> int
{
    // Need to get a volume represented as floats, then write it to disk.
    speck::buffer_type_f buf;
    size_t buf_size;
    this->get_decompressed_volume_f( buf, buf_size );
    if( buf_size != m_total_vals )
        return 1;

    std::FILE* file = std::fopen( filename, "wb" );
    if( file ) {
        std::fwrite( buf.get(), sizeof(float), m_total_vals, file );
        std::fclose( file );
        return 0;
    }
    else {
        return 1;
    }

return 0;
}


auto SPECK3D_Decompressor::set_bpp( float bpp ) -> int
{
    if( bpp < 0.0 || bpp > 64.0 )
        return 1;
    else {
        m_bpp = bpp;
        return 0;
    }
}


void SPECK3D_Decompressor::get_volume_dims( size_t& x, size_t& y, size_t& z ) const
{
    x = m_dim_x;
    y = m_dim_y;
    z = m_dim_z;
}
