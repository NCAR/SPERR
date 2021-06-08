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

    // If `m_zstd_buf` is not big enough for the decompressed buffer, we re-size it 
    if( content_size   > m_zstd_buf_len ) {
        m_zstd_buf_len = std::max( content_size, m_zstd_buf_len * 2 );
        m_zstd_buf     = std::make_unique<uint8_t[]>(m_zstd_buf_len);
    }

    const auto decomp_size = ZSTD_decompressDCtx( m_dctx.get(),
                                                  m_zstd_buf.get(), content_size, 
                                                  p, len );
    if( ZSTD_isError( decomp_size ) || decomp_size != content_size )
        return RTNType::ZSTDError;
    const uint8_t* const ptr     = m_zstd_buf.get();
    const size_t         ptr_len = decomp_size;
#else
    const uint8_t* const ptr     = static_cast<const uint8_t*>(p);
    const size_t         ptr_len = len;
#endif

    // Step 1: extract conditioner stream from it
    m_condi_stream.clear();
    const auto condi_size = m_conditioner.get_meta_size();
    if( condi_size > ptr_len )
        return RTNType::WrongSize;
    m_condi_stream.resize( condi_size, 0 );
    std::copy( ptr, ptr + condi_size, m_condi_stream.begin() );
    size_t pos = condi_size;

    // Step 2: extract SPECK stream from it
    m_speck_stream.clear();
    const uint8_t* const speck_p = ptr + pos;
    const auto speck_size = m_decoder.get_speck_stream_size(speck_p);
    if( pos + speck_size > ptr_len )
        return RTNType::WrongSize;
    m_speck_stream.resize( speck_size, 0 );
    std::copy( speck_p, speck_p + speck_size, m_speck_stream.begin() );
    pos += speck_size;

    // Step 3: keep the volume dimension from the header
    auto dims = m_decoder.get_speck_stream_dims( m_speck_stream.data() );
    m_dim_x = dims[0];
    m_dim_y = dims[1];
    m_dim_z = dims[2];

#ifdef QZ_TERM
    // Step 4: extract SPERR stream from it
    m_sperr_stream.clear();
    if( pos < ptr_len ) {
        const uint8_t* const sperr_p = ptr + pos;
        const auto sperr_size = m_sperr.get_sperr_stream_size( sperr_p );
        if( pos + sperr_size != ptr_len )
            return RTNType::WrongSize;
        m_sperr_stream.resize( sperr_size, 0 );
        std::copy( sperr_p, sperr_p + pos, m_sperr_stream.begin() );
    }
#endif

    return RTNType::Good;
}


#ifndef QZ_TERM
auto SPECK3D_Decompressor::set_bpp( float bpp ) -> RTNType
{
    if( bpp < 0.0 || bpp > 64.0 )
        return RTNType::InvalidParam;
    else {
        m_bpp = bpp;
        return RTNType::Good;
    }
}
#endif


auto SPECK3D_Decompressor::decompress( ) -> RTNType
{
    // Step 1: SPECK decode.
    if( m_speck_stream.empty() )
        return RTNType::Error;
    
    auto rtn = m_decoder.parse_encoded_bitstream(m_speck_stream.data(), m_speck_stream.size());
    if( rtn != RTNType::Good )
        return rtn;

#ifndef QZ_TERM
    m_decoder.set_bit_budget( size_t(m_bpp * float(m_dim_x * m_dim_y * m_dim_z)) );
#endif

    rtn = m_decoder.decode();
    if( rtn != RTNType::Good )
        return rtn;

    // Step 2: Inverse Wavelet Transform
    //
    // (Here we ask `m_cdf` to make a copy of coefficients from `m_decoder` instead of
    //  transfer the ownership, because `m_decoder` will reuse that memory when processing
    //  the next chunk. For the same reason, `m_cdf` keeps its memory.)
    auto decoder_out = m_decoder.view_data();
    m_cdf.copy_data( decoder_out.data(), decoder_out.size(), m_dim_x, m_dim_y, m_dim_z );
    m_cdf.idwt3d();

    // Step 3: Inverse Conditioning
    auto cdf_out = m_cdf.view_data();
    m_val_buf.resize( cdf_out.size() );
    std::copy( cdf_out.begin(), cdf_out.end(), m_val_buf.begin() );
    m_conditioner.inverse_condition( m_val_buf, m_val_buf.size(), m_condi_stream.data() );

#ifdef QZ_TERM
    // Step 4: If there's SPERR data, then do the correction.
    // This condition occurs only in QZ_TERM mode.
    if( !m_sperr_stream.empty() ) {
        rtn = m_sperr.parse_encoded_bitstream(m_sperr_stream.data(), m_sperr_stream.size());
        if( rtn != RTNType::Good )
            return rtn;
        rtn = m_sperr.decode();
        if( rtn != RTNType::Good )
            return rtn;

        const auto& los = m_sperr.view_outliers();
        for( const auto& outlier : los )
            m_val_buf[outlier.location] += outlier.error;
    }
#endif

    return RTNType::Good;
}


template<typename T>
auto SPECK3D_Decompressor::get_data() const -> std::vector<T>
{
    auto out_buf = std::vector<T>( m_val_buf.size() );
    std::copy( m_val_buf.begin(), m_val_buf.end(), out_buf.begin() );

    return std::move(out_buf);
}
template auto SPECK3D_Decompressor::get_data() const -> std::vector<double>;
template auto SPECK3D_Decompressor::get_data() const -> std::vector<float>;


auto SPECK3D_Decompressor::view_data() const -> const std::vector<double>&
{
    return m_val_buf;
}


auto SPECK3D_Decompressor::release_data() -> std::vector<double>
{
    m_dim_x = 0;
    m_dim_y = 0;
    m_dim_z = 0;
    return std::move(m_val_buf);
}

auto SPECK3D_Decompressor::get_dims() const -> std::array<size_t, 3>
{
    return {m_dim_x, m_dim_y, m_dim_z};
}


