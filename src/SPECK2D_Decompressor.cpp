#include "SPECK2D_Decompressor.h"

#include <cstring>
#include <cassert>

#ifdef USE_ZSTD
    #include "zstd.h"
#endif


void SPECK2D_Decompressor::copy_bitstream( const void* p, size_t len )
{
    m_entire_stream = std::make_unique<uint8_t[]>( len );
    std::memcpy( m_entire_stream.get(), p, len );
    m_entire_stream_len = len;
    m_metadata_parsed = false;
}

    
void SPECK2D_Decompressor::take_bitstream( speck::buffer_type_uint8 buf, size_t len )
{
    m_entire_stream = std::move( buf );
    m_entire_stream_len = len;
    m_metadata_parsed = false;
}


auto SPECK2D_Decompressor::read_bitstream( const char* filename ) -> RTNType
{
    auto buf = speck::read_whole_file<uint8_t>( filename );
    if( buf.first == nullptr || buf.second == 0 )
        return RTNType::IOError;
    
    m_entire_stream     = std::move( buf.first );
    m_entire_stream_len = buf.second;
    m_metadata_parsed   = false;

    return RTNType::Good;
}


auto SPECK2D_Decompressor::decompress() -> RTNType
{
    if( m_entire_stream == nullptr || m_entire_stream_len == 0 )
        return RTNType::Error;

    auto rtn = RTNType::Good;
    if( !m_metadata_parsed )
        rtn = this->m_parse_metadata();
    if( rtn != RTNType::Good )
        return rtn;

    rtn = m_decoder.parse_encoded_bitstream( m_entire_stream.get() + m_meta_size,
                                             m_entire_stream_len   - m_meta_size );
    if( rtn != RTNType::Good )
        return rtn;

    const auto dims = m_decoder.get_dims();
    assert( dims[0] > 1 && dims[1] > 1 && dims[2] == 1 );
    auto total_vals = dims[0] * dims[1];

    m_decoder.set_bit_budget( size_t(m_bpp * total_vals) );
    rtn = m_decoder.decode();
    if( rtn != RTNType::Good )
        return rtn;

    auto coeffs = m_decoder.release_data();
    if( !speck::size_is(coeffs, total_vals) )
        return RTNType::Error;
    m_cdf.take_data( std::move(coeffs.first), total_vals, dims[0], dims[1] );
    m_cdf.set_mean( m_decoder.get_image_mean() );
    m_cdf.idwt2d();

    return RTNType::Good;
}


auto SPECK2D_Decompressor::get_decompressed_slice_f() const 
                           -> std::pair<speck::buffer_type_f, size_t>
{
    auto slice = m_cdf.get_read_only_data( );
    if( slice.first == nullptr || slice.second == 0 )
        return {nullptr, 0};

    auto out_buf = std::make_unique<float[]>(slice.second);
    auto begin = speck::begin( slice.first );
    auto end   = begin + slice.second;
    std::copy( begin, end, speck::begin( out_buf ) );

    return {std::move(out_buf), slice.second};
}


auto SPECK2D_Decompressor::get_decompressed_slice_d() const
                           -> std::pair<speck::buffer_type_d, size_t> 
{
    auto slice = m_cdf.get_read_only_data();
    if( slice.first == nullptr || slice.second == 0 )
        return {nullptr, 0};

    auto out_buf  = std::make_unique<double[]>(slice.second);
    std::memcpy( out_buf.get(), slice.first.get(), sizeof(double) * slice.second );

    return {std::move(out_buf), slice.second};
}


auto SPECK2D_Decompressor::write_slice_d( const char* filename ) const -> RTNType
{
    // Get a read-only handle of the slice from m_cdf, and then write it to disk.
    auto slice = m_cdf.get_read_only_data( );
    if( slice.first == nullptr || slice.second == 0 )
        return RTNType::Error;

    return speck::write_n_bytes( filename, sizeof(double) * slice.second, slice.first.get() );
}


auto SPECK2D_Decompressor::write_slice_f( const char* filename ) const -> RTNType
{
    // Need to get a slice represented as floats, then write it to disk.
    auto slice = this->get_decompressed_slice_f();
    if( slice.first == nullptr || slice.second == 0 )
        return RTNType::Error;

    return speck::write_n_bytes( filename, sizeof(float) * slice.second, slice.first.get() );
}


auto SPECK2D_Decompressor::set_bpp( float bpp ) -> RTNType
{
    if( bpp < 0.0 || bpp > 64.0 )
        return RTNType::InvalidParam;
    else {
        m_bpp = bpp;
        return RTNType::Good;
    }
}


auto SPECK2D_Decompressor::m_parse_metadata() -> RTNType
{
    // This method parses the metadata of a bitstream and performs the following tasks:
    // 1) Verify if the major version number is consistant.
    // 2) Verify that the application of ZSTD is consistant, and apply ZSTD if needed.
    // 3) Make sure if the bitstream is for 2D encoding.
    // 3) Separate error-bound data from SPECK stream if needed.

    assert( m_entire_stream != nullptr && m_entire_stream_len > 2 && !m_metadata_parsed);

    uint8_t meta[2];
    assert( sizeof(meta) == m_meta_size );
    bool    metabool[8];
    std::memcpy( meta, m_entire_stream.get(), sizeof(meta) );
    speck::unpack_8_booleans( metabool, meta[1] );

    // Task 1)
    if( meta[0] != uint8_t(SPECK_VERSION_MAJOR) )
        return RTNType::VersionMismatch;

    // Task 2)
#ifdef USE_ZSTD
    if( metabool[0] == false )
        return RTNType::ZSTDMismatch;

    const auto content_size = ZSTD_getFrameContentSize( m_entire_stream.get() + m_meta_size,
                                                        m_entire_stream_len   - m_meta_size );
    if( content_size == ZSTD_CONTENTSIZE_ERROR || content_size == ZSTD_CONTENTSIZE_UNKNOWN )
        return RTNType::ZSTDError;

    auto content_buf = std::make_unique<uint8_t[]>( m_meta_size + content_size );
    const auto decomp_size = ZSTD_decompress( content_buf.get() + m_meta_size,
                                              content_size,
                                              m_entire_stream.get() + m_meta_size,
                                              m_entire_stream_len   - m_meta_size );
    if( ZSTD_isError( decomp_size ) || decomp_size != content_size )
        return RTNType::ZSTDError;

    // Replace `m_entire_stream` with the decompressed version.
    std::memcpy( content_buf.get(), meta, sizeof(meta) );
    m_entire_stream = std::move( content_buf );
    m_entire_stream_len = m_meta_size + decomp_size;
#else
    if( metabool[0] == true )
        return RTNType::ZSTDMismatch;
#endif

    // Task 3)
    if( metabool[1] != false ) // false means it's 2D
        return RTNType::DimMismatch;

    m_metadata_parsed = true;

    return RTNType::Good;
}








