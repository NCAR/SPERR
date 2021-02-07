#include "SPECK3D_Decompressor.h"

#include <cstring>
#include <cassert>

#ifdef USE_ZSTD
    #include "zstd.h"
#endif


void SPECK3D_Decompressor::copy_bitstream( const void* p, size_t len )
{
    auto buf = std::make_unique<uint8_t[]>( len );
    std::memcpy( buf.get(), p, len );
    m_entire_stream = {std::move(buf), len};
    m_speck_stream  = {nullptr, 0};
#ifdef QZ_TERM
    m_sperr_stream  = {nullptr, 0};
    m_sperr_los.clear();
#endif
}

    
void SPECK3D_Decompressor::take_bitstream( speck::smart_buffer_uint8 buf )
{
    m_entire_stream = std::move( buf );
    m_speck_stream  = {nullptr, 0};
#ifdef QZ_TERM
    m_sperr_stream  = {nullptr, 0};
    m_sperr_los.clear();
#endif
}
    

auto SPECK3D_Decompressor::read_bitstream( const char* filename ) -> RTNType
{
    auto buf = speck::read_whole_file<uint8_t>( filename );
    if( speck::empty_buf(buf) )
        return RTNType::IOError;
    
    m_entire_stream.first   = std::move( buf.first );
    m_entire_stream.second  = buf.second;
    m_speck_stream  = {nullptr, 0};
#ifdef QZ_TERM
    m_sperr_stream  = {nullptr, 0};
    m_sperr_los.clear();
#endif

    return RTNType::Good;
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


auto SPECK3D_Decompressor::decompress() -> RTNType
{
    auto rtn = RTNType::Good;
    if( speck::empty_buf(m_speck_stream) )
        rtn  = this->m_parse_metadata();
    if( rtn != RTNType::Good )
        return rtn;

    // Step 1: SPECK decode.
    rtn = m_decoder.parse_encoded_bitstream( m_speck_stream.first.get(),
                                             m_speck_stream.second );
    if( rtn != RTNType::Good )
        return rtn;

    const auto dims = m_decoder.get_dims();
    assert( dims[0] > 1 && dims[1] > 1 && dims[2] > 1 );
    auto total_vals = dims[0] * dims[1] * dims[2];

    m_decoder.set_bit_budget( size_t(m_bpp * total_vals) );
    rtn = m_decoder.decode();
    if( rtn != RTNType::Good )
        return rtn;

    // Step 2: Inverse Wavelet Transform
    m_cdf.set_dims( dims[0], dims[1], dims[2] );
    m_cdf.set_mean( m_decoder.get_image_mean() );
    auto coeffs = m_decoder.release_data();
    m_cdf.take_data( std::move(coeffs.first), coeffs.second );
    m_cdf.idwt3d();

    // Step 3: If there's SPERR data, then do the correction.
    // This condition occurs only in QZ_TERM mode.
#ifdef QZ_TERM
    if( !speck::empty_buf(m_sperr_stream) ) {
        rtn = m_sperr.parse_encoded_bitstream(m_sperr_stream.first.get(), 
                                              m_sperr_stream.second);
        if( rtn != RTNType::Good )
            return rtn;
        rtn = m_sperr.decode();
        if( rtn != RTNType::Good )
            return rtn;
        m_sperr_los = m_sperr.release_outliers();
    }
#endif

    return RTNType::Good;
}


auto SPECK3D_Decompressor::get_decompressed_volume_f() const
                           -> std::pair<speck::buffer_type_f, size_t>
{
    auto vol = m_cdf.get_read_only_data();
    if( vol.first == nullptr || vol.second == 0 )
        return {nullptr, 0};

    auto out_buf = std::make_unique<float[]>(vol.second);
    auto begin = speck::begin( vol.first );
    auto end   = speck::end( vol.first, vol.second );
    std::copy( begin, end, speck::begin( out_buf ) );

    // If there are corrections for outliers, do it now!
#ifdef QZ_TERM
    for( const auto& outlier : m_sperr_los ) {
        out_buf[outlier.location] += outlier.error;
    }
#endif

    return {std::move(out_buf), vol.second};
}


auto SPECK3D_Decompressor::get_decompressed_volume_d() const
                           -> std::pair<speck::buffer_type_d, size_t>
{
    auto vol = m_cdf.get_read_only_data();
    if(  vol.first == nullptr || vol.second == 0 )
        return {nullptr, 0};
    
    auto out_buf = std::make_unique<double[]>(vol.second);
    std::memcpy( out_buf.get(), vol.first.get(), sizeof(double) * vol.second );

    // If there are corrections for outliers, do it now!
#ifdef QZ_TERM
    for( const auto& outlier : m_sperr_los ) {
        out_buf[outlier.location] += outlier.error;
    }
#endif

    return {std::move(out_buf), vol.second};
}


auto SPECK3D_Decompressor::write_volume_d( const char* filename ) const -> RTNType 
{
    // Get a read-only handle of the volume from m_cdf, and then write it to disk.
    auto vol = m_cdf.get_read_only_data( );
    if( vol.first == nullptr || vol.second == 0 )
        return RTNType::Error;

    return speck::write_n_bytes( filename, sizeof(double) * vol.second, vol.first.get() );
}


auto SPECK3D_Decompressor::write_volume_f( const char* filename ) const -> RTNType
{
    // Need to get a volume represented as floats, then write it to disk.
    auto vol = get_decompressed_volume_f();
    if( vol.first == nullptr || vol.second == 0 )
        return RTNType::Error;

    return speck::write_n_bytes( filename, sizeof(float) * vol.second, vol.first.get() );
}


auto SPECK3D_Decompressor::m_parse_metadata() -> RTNType
{
    // This method parses the metadata of a bitstream and performs the following tasks:
    // Note: the definition of metadata is documented in the file SPECK3D_Compressor.cpp
    // 1) Verify if the major version number is consistant.
    // 2) Verify that the application of ZSTD is consistant, and apply ZSTD if needed.
    // 3) Make sure if the bitstream is for 3D encoding.
    // 4) Fill `m_speck_stream` and `m_sperr_stream` as appropriate and empty `m_entire_stream`.

    assert( !speck::empty_buf(m_entire_stream) && speck::empty_buf(m_speck_stream) );
#ifdef QZ_TERM
    assert( speck::empty_buf(m_sperr_stream) );
#endif

    // Helper alias
    const uint8_t* entire_ptr  = m_entire_stream.first.get();
    size_t         entire_size = m_entire_stream.second;

    uint8_t meta[2];
    assert( sizeof(meta) == m_meta_size );
    bool    metabool[8];
    std::memcpy( meta, entire_ptr, sizeof(meta) );
    speck::unpack_8_booleans( metabool, meta[1] );

    // Task 1)
    if( meta[0] != uint8_t(SPECK_VERSION_MAJOR) )
        return RTNType::VersionMismatch;

    // Task 3)
    if( metabool[1] != true ) // true means it's 3D
        return RTNType::DimMismatch;

    // Task 2)
#ifdef USE_ZSTD
    if( metabool[0] == false )
        return RTNType::ZSTDMismatch;

    const auto content_size = ZSTD_getFrameContentSize( entire_ptr  + m_meta_size,
                                                        entire_size - m_meta_size );
    if( content_size == ZSTD_CONTENTSIZE_ERROR || content_size == ZSTD_CONTENTSIZE_UNKNOWN )
        return RTNType::ZSTDError;

    auto content_buf = std::make_unique<uint8_t[]>( m_meta_size + content_size );
    const auto decomp_size = ZSTD_decompress( content_buf.get() + m_meta_size, 
                                              content_size,
                                              entire_ptr  + m_meta_size,
                                              entire_size - m_meta_size );
    if( ZSTD_isError( decomp_size ) || decomp_size != content_size )
        return RTNType::ZSTDError;

    // Replace `m_entire_stream` with the decompressed version.
    std::memcpy( content_buf.get(), meta, sizeof(meta) );
    m_entire_stream.first  = std::move( content_buf );
    m_entire_stream.second = m_meta_size + decomp_size;
    entire_ptr             = m_entire_stream.first.get();
    entire_size            = m_entire_stream.second;
#else
    if( metabool[0] == true )
        return RTNType::ZSTDMismatch;
#endif
        
    // Task 4)
    const auto speck_size = m_decoder.get_speck_stream_size(entire_ptr + m_meta_size);
    if( metabool[2] ) { 
        // There is SPERR bitstream, which could only happen in QZ_TERM mode.
#ifdef QZ_TERM
        assert( m_meta_size + speck_size <= entire_size );

        auto buf = std::make_unique<uint8_t[]>( speck_size );
        std::memcpy( buf.get(), entire_ptr + m_meta_size, speck_size );
        m_speck_stream = {std::move(buf), speck_size};

        auto sperr_size = entire_size - m_meta_size - speck_size;
        buf = std::make_unique<uint8_t[]>( sperr_size );
        std::memcpy( buf.get(), entire_ptr + m_meta_size + speck_size, sperr_size );
        m_sperr_stream = {std::move(buf), sperr_size};
#else
        return RTNType::InvalidParam;
#endif
    }
    else {
        // There is no SPERR bitstream. This condition could occur in either QZ_TERM or not.
        assert( m_meta_size + speck_size == entire_size );

        auto buf = std::make_unique<uint8_t[]>( speck_size );
        std::memcpy( buf.get(), entire_ptr + m_meta_size, speck_size );
        m_speck_stream = {std::move(buf), speck_size};
    }
        
    // Clean up work
    m_entire_stream = {nullptr, 0};
    entire_ptr      = nullptr;
    entire_size     = 0;

    return RTNType::Good;
}










