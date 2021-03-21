#include "SPECK3D_Decompressor.h"

#include <cstring>
#include <cassert>

#ifdef USE_ZSTD
  #include "zstd.h"
#endif


auto SPECK3D_Decompressor::use_bitstream( const void* p, size_t len ) -> RTNType
{
#ifdef USE_ZSTD
    // Preprocessing: if ZSTD is enabled, we need to run ZSTD decompression first!
    const size_t content_size = ZSTD_getFrameContentSize( p, len );
    if( content_size == ZSTD_CONTENTSIZE_ERROR || content_size == ZSTD_CONTENTSIZE_UNKNOWN )
        return RTNType::ZSTDError;

    // If `m_tmp_buf` is not big enough for the decompressed buffer, we re-size it using
    // the same strategy as std::vector .
    if( content_size  > m_tmp_buf.second ) {
        auto tmp_size = std::max( content_size, m_tmp_buf.second * 2 );
        m_tmp_buf     = {std::make_unique<uint8_t[]>(tmp_size), tmp_size};
    }

    const auto decomp_size = ZSTD_decompress( m_tmp_buf.first.get(), content_size, p, len );
    if( ZSTD_isError( decomp_size ) || decomp_size != content_size )
        return RTNType::ZSTDError;
    const uint8_t* const ptr     = m_tmp_buf.first.get();
    const size_t         ptr_len = decomp_size;
#else
    const uint8_t* const ptr     = static_cast<const uint8_t*>(p);
    const size_t         ptr_len = len;
#endif

    // Step 1: extract SPECK stream from it
    m_speck_stream.clear();
    const auto speck_size = m_decoder.get_speck_stream_size(ptr);
    if( speck_size > ptr_len )
        return RTNType::WrongSize;
    m_speck_stream.resize( speck_size, 0 );
    std::copy( ptr, ptr + speck_size, m_speck_stream.begin() );

    // Step 2: keep the volume dimension from the header
    auto dims = m_decoder.get_speck_stream_dims( ptr );
    m_dim_x   = dims[0];
    m_dim_y   = dims[1];
    m_dim_z   = dims[2];

    // Step 3: extract SPERR stream from it
#ifdef QZ_TERM
    m_sperr_stream.clear();
    if( speck_size < ptr_len ) {
        const uint8_t* const sperr_p = ptr + speck_size;
        const auto sperr_size = m_sperr.get_sperr_stream_size( sperr_p );
        if( sperr_size != ptr_len - speck_size )
            return RTNType::WrongSize;
        m_sperr_stream.resize( sperr_size, 0 );
        std::copy( sperr_p, sperr_p + sperr_size, m_sperr_stream.begin() );
    }
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


auto SPECK3D_Decompressor::decompress( ) -> RTNType
{
    // Step 1: SPECK decode.
    if( m_speck_stream.empty() )
        return RTNType::Error;
    
    auto rtn = m_decoder.parse_encoded_bitstream(m_speck_stream.data(), m_speck_stream.size());
    if( rtn != RTNType::Good )
        return rtn;

    m_decoder.set_bit_budget( size_t(m_bpp * float(m_dim_x * m_dim_y * m_dim_z)) );
    rtn = m_decoder.decode();
    if( rtn != RTNType::Good )
        return rtn;

    // Step 2: Inverse Wavelet Transform
    auto coeffs = m_decoder.release_data();
    m_cdf.take_data( std::move(coeffs.first), coeffs.second, m_dim_x, m_dim_y, m_dim_z );
    m_cdf.set_mean( m_decoder.get_image_mean() );
    m_cdf.idwt3d();

    // Step 3: If there's SPERR data, then do the correction.
    // This condition occurs only in QZ_TERM mode.
#ifdef QZ_TERM
    m_LOS.clear();
    if( !m_sperr_stream.empty() ) {
        rtn = m_sperr.parse_encoded_bitstream(m_sperr_stream.data(), m_sperr_stream.size());
        if( rtn != RTNType::Good )
            return rtn;
        rtn = m_sperr.decode();
        if( rtn != RTNType::Good )
            return rtn;
        m_LOS = m_sperr.view_outliers();
    }
#endif

    return RTNType::Good;
}


template<typename T>
auto SPECK3D_Decompressor::get_decompressed_volume() const ->
                           std::pair<std::unique_ptr<T[]>, size_t>
{
    auto [vol, len]  = m_cdf.get_read_only_data();
    if( vol == nullptr || len == 0 )
        return {nullptr, 0};

    auto out_buf = std::make_unique<T[]>( len );
    auto begin   = speck::begin( vol );
    auto end     = begin + len;
    std::copy( begin, end, speck::begin(out_buf) );

#ifdef QZ_TERM
    // If there are corrections for outliers, do it now!
    for( const auto& outlier : m_LOS ) {
        out_buf[outlier.location] += outlier.error;
    }
#endif

    return {std::move(out_buf), len};
}
template auto SPECK3D_Decompressor::get_decompressed_volume() const -> speck::smart_buffer_f;
template auto SPECK3D_Decompressor::get_decompressed_volume() const -> speck::smart_buffer_d;


auto SPECK3D_Decompressor::get_dims() const -> std::array<size_t, 3>
{
    return {m_dim_x, m_dim_y, m_dim_z};
}


