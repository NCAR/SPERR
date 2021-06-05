#include "SPECK3D_Decompressor.h"

#include <cstring>
#include <cassert>


auto SPECK3D_Decompressor::use_bitstream( const void* p, size_t len ) -> RTNType
{
#ifdef USE_ZSTD
    // Make sure that we have a ZSTD Decompression Context first
    if( m_dctx == nullptr ) {
        auto* ctx_p = ZSTD_createDCtx();
        if( ctx_p  == nullptr )
            return RTNType::ZSTDError;
        else
            m_dctx.reset(ctx_p);
    }

    const size_t content_size = ZSTD_getFrameContentSize( p, len );
    if( content_size == ZSTD_CONTENTSIZE_ERROR || content_size == ZSTD_CONTENTSIZE_UNKNOWN )
        return RTNType::ZSTDError;

    // If `m_tmp_buf` is not big enough for the decompressed buffer, we re-size it using
    // the same strategy as std::vector.
    if( content_size  > m_tmp_buf.second ) {
        auto tmp_size = std::max( content_size, m_tmp_buf.second * 2 );
        m_tmp_buf     = {std::make_unique<uint8_t[]>(tmp_size), tmp_size};
    }

    const auto decomp_size = ZSTD_decompressDCtx( m_dctx.get(),
                                                  m_tmp_buf.first.get(), content_size, 
                                                  p, len );
    if( ZSTD_isError( decomp_size ) || decomp_size != content_size )
        return RTNType::ZSTDError;
    const uint8_t* const ptr     = m_tmp_buf.first.get();
    const size_t         ptr_len = decomp_size;
#else
    const uint8_t* const ptr     = static_cast<const uint8_t*>(p);
    const size_t         ptr_len = len;
#endif

    // Step 1: extract conditioner stream from it
    const auto condi_size = m_conditioner.get_meta_size();
    if( condi_size > ptr_len )
        return RTNType::WrongSize;
    m_condi_stream.resize( condi_size, 0 );
    std::copy( ptr, ptr + condi_size, m_condi_stream.begin() );
    size_t pos = condi_size;

    // Step 2: extract SPECK stream from it
    m_speck_stream.clear();
    const uint8_t* const speck_p = ptr +pos;
    const auto speck_size = m_decoder.get_speck_stream_size(speck_p);
    if( pos + speck_size > ptr_len )
        return RTNType::WrongSize;
    m_speck_stream.resize( speck_size, 0 );
    std::copy( speck_p, speck_p + speck_size, m_speck_stream.begin() );
    pos += speck_size;

    // Step 3: keep the volume dimension from the header
    auto dims = m_decoder.get_speck_stream_dims( m_speck_stream.data() );
    m_dim_x   = dims[0];
    m_dim_y   = dims[1];
    m_dim_z   = dims[2];

#ifdef QZ_TERM
    // Step 4: extract SPERR stream from it
    m_sperr_stream.clear();
    if( pos < ptr_len ) {
        const uint8_t* const sperr_p = ptr + pos;
        const auto sperr_size = m_sperr.get_sperr_stream_size( sperr_p );
        if( sperr_size != ptr_len - pos )
            return RTNType::WrongSize;
        m_sperr_stream.resize( sperr_size, 0 );
        std::copy( sperr_p, sperr_p + pos, m_sperr_stream.begin() );
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
    //
    // (Here we ask `m_cdf` to make a copy of coefficients from `m_decoder` instead of
    //  transfer the ownership, because `m_decoder` will reuse that memory when processing
    //  the next chunk. For the same reason, `m_cdf` keeps its memory.)
    auto decoder_out = m_decoder.view_data();
    m_cdf.copy_data( decoder_out.first.get(), decoder_out.second, m_dim_x, m_dim_y, m_dim_z );
    m_cdf.idwt3d();

    // Step 3: Inverse Conditioning
    auto cdf_out = m_cdf.view_data();
    if( cdf_out.second != decoder_out.second )
        return RTNType::WrongSize;
    m_val_buf.resize( cdf_out.second, 0.0 );
    std::copy( cdf_out.first, cdf_out.first + cdf_out.second, m_val_buf.begin() );
    m_conditioner.inverse_condition( m_val_buf, m_val_buf.size(), m_condi_stream.data() );

#ifdef QZ_TERM
    // Step 4: If there's SPERR data, then do the correction.
    // This condition occurs only in QZ_TERM mode.
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
    for( const auto& outlier : m_LOS ) {
        m_val_buf[outlier.location] += outlier.error;
    }
#endif

    return RTNType::Good;
}


template<typename T>
auto SPECK3D_Decompressor::get_decompressed_volume() const ->
                           std::pair<std::unique_ptr<T[]>, size_t>
{
    const auto len = m_val_buf.size();
    if( len == 0 )
        return {nullptr, 0};

    auto out_buf = std::make_unique<T[]>( len );
    std::copy( m_val_buf.begin(), m_val_buf.end(), speck::begin(out_buf) );

    return {std::move(out_buf), len};
}
template auto SPECK3D_Decompressor::get_decompressed_volume() const -> speck::smart_buffer_f;
template auto SPECK3D_Decompressor::get_decompressed_volume() const -> speck::smart_buffer_d;


auto SPECK3D_Decompressor::get_dims() const -> std::array<size_t, 3>
{
    return {m_dim_x, m_dim_y, m_dim_z};
}


