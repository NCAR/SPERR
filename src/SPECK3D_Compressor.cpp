#include "SPECK3D_Compressor.h"

#include <cassert>
#include <cstring>


template< typename T >
auto SPECK3D_Compressor::copy_data( const T* p, size_t len, size_t dimx, size_t dimy, size_t dimz) 
                       -> RTNType
{
    static_assert(std::is_floating_point<T>::value,
                  "!! Only floating point values are supported !!");

    if( len != dimx * dimy * dimz )
        return RTNType::DimMismatch;

    if( m_val_buf == nullptr || m_total_vals != len ) {
        m_total_vals = len;
        m_val_buf = std::make_unique<double[]>( len );
    }
    std::copy( p, p + len, speck::begin(m_val_buf) );

    m_dim_x = dimx;
    m_dim_y = dimy;
    m_dim_z = dimz;

    return RTNType::Good;
}
template auto SPECK3D_Compressor::copy_data( const double*, size_t, size_t, size_t, size_t ) -> RTNType;
template auto SPECK3D_Compressor::copy_data( const float*,  size_t, size_t, size_t, size_t ) -> RTNType;


auto SPECK3D_Compressor::take_data( speck::buffer_type_d buf, size_t len,
                                    size_t dimx, size_t dimy, size_t dimz ) -> RTNType
{
    if( len != dimx * dimy * dimz )
        return RTNType::DimMismatch;

    m_val_buf    = std::move( buf );
    m_total_vals = len;
    m_dim_x      = dimx;
    m_dim_y      = dimy;
    m_dim_z      = dimz;

    return RTNType::Good;
}

 
#ifdef QZ_TERM
auto SPECK3D_Compressor::compress() -> RTNType
{
    if( m_val_buf == nullptr || m_total_vals == 0 )
        return RTNType::Error;
    m_speck_stream = {nullptr, 0};
    m_sperr_stream = {nullptr, 0};
    m_num_outlier  = 0;

    // Note that we keep the original buffer untouched for outlier calculations later.
    m_cdf.copy_data( m_val_buf.get(), m_total_vals, m_dim_x, m_dim_y, m_dim_z );
    m_cdf.dwt3d();
    auto cdf_out = m_cdf.release_data();
    m_encoder.set_image_mean( m_cdf.get_mean() );
    m_encoder.take_data( std::move(cdf_out.first), cdf_out.second, m_dim_x, m_dim_y, m_dim_z );
    m_encoder.set_quantization_term_level( m_qz_lev );
    auto rtn = m_encoder.encode();
    if( rtn != RTNType::Good )
        return rtn;
    else {
        m_speck_stream = m_encoder.get_encoded_bitstream();
        if( speck::empty_buf(m_speck_stream) )
            return RTNType::Error;
    }

    // Now we perform a decompression pass reusing the same object states and memory blocks.
    m_encoder.set_bit_budget( 0 );
    rtn = m_encoder.decode();    
    if( rtn != RTNType::Good )
        return rtn;
    auto coeffs = m_encoder.release_data();
    if( !speck::size_is(coeffs, m_total_vals) )
        return RTNType::Error;
    m_cdf.take_data( std::move(coeffs.first), coeffs.second, m_dim_x, m_dim_y, m_dim_z );
    m_cdf.idwt3d();

    // Now we find all the outliers!
    auto vol = m_cdf.get_read_only_data();
    if( !speck::size_is( vol, m_total_vals ) )
        return RTNType::Error;
    m_LOS.clear();
    for( size_t i = 0; i < m_total_vals; i++ ) {
        auto diff = m_val_buf[i] - vol.first[i];
        if( std::abs(diff) > m_tol )
            m_LOS.emplace_back( i, diff );
    }

    // Now we encode any outlier that's found.
    if( !m_LOS.empty() ) {
        m_num_outlier = m_LOS.size();
        m_sperr.set_length( m_total_vals );
        m_sperr.use_outlier_list( m_LOS );
        rtn = m_sperr.encode();
        if( rtn != RTNType::Good )
            return rtn;
        m_sperr_stream = m_sperr.get_encoded_bitstream();
        if( speck::empty_buf(m_sperr_stream) )
            return RTNType::Error;
    }

    return RTNType::Good;
}
#else
auto SPECK3D_Compressor::compress() -> RTNType
{
    if( m_val_buf == nullptr || m_total_vals == 0 )
        return RTNType::Error;
    m_speck_stream = {nullptr, 0};
    m_sperr_stream = {nullptr, 0};

    RTNType rtn;
    rtn = m_cdf.take_data( std::move(m_val_buf), m_total_vals, m_dim_x, m_dim_y, m_dim_z );
    if( rtn != RTNType::Good )
        return rtn;
    m_cdf.dwt3d();
    auto cdf_out = m_cdf.release_data();
    if( cdf_out.first == nullptr || cdf_out.second != m_total_vals )
        return RTNType::Error;

    m_encoder.set_image_mean( m_cdf.get_mean() );
    rtn = m_encoder.take_data(std::move(cdf_out.first), cdf_out.second, m_dim_x, m_dim_y, m_dim_z);
    if( rtn != RTNType::Good )
        return rtn;
    m_encoder.set_bit_budget( size_t(m_bpp * m_total_vals) );

    rtn = m_encoder.encode();
    if( rtn != RTNType::Good )
        return rtn;
    else {
        m_speck_stream = m_encoder.get_encoded_bitstream();
        return (speck::empty_buf(m_speck_stream) ? RTNType::Error : RTNType::Good);
    }
}
#endif


auto SPECK3D_Compressor::get_encoded_bitstream() const -> speck::smart_buffer_uint8
{
    const size_t total_size = m_speck_stream.second + m_sperr_stream.second;

#ifdef USE_ZSTD
    // Need to have a ZSTD Compression Context first
    if( m_cctx == nullptr ) {
        auto* ctx_p = ZSTD_createCCtx();
        if( ctx_p  == nullptr )
            return {nullptr, 0};
        else
            m_cctx.reset(ctx_p);
    }
        
    m_tmp_buf.resize( total_size, 0 );
    std::copy( speck::begin(m_speck_stream), speck::end(m_speck_stream), m_tmp_buf.begin() );
    if( !speck::empty_buf(m_sperr_stream) ) { // Not sure about nullptr, so we do the test.
        std::copy(  speck::begin(m_sperr_stream), speck::end(m_sperr_stream),
                    m_tmp_buf.begin() + m_speck_stream.second );
    }

    const size_t comp_buf_size = ZSTD_compressBound( total_size );
    auto comp_buf = std::make_unique<uint8_t[]>( comp_buf_size );
    const size_t comp_size = ZSTD_compressCCtx( m_cctx.get(),
                                                comp_buf.get(),   comp_buf_size,
                                                m_tmp_buf.data(), total_size,
                                                ZSTD_CLEVEL_DEFAULT + 6 );
    if( ZSTD_isError( comp_size ) )
        return {nullptr, 0};
    else
        return {std::move(comp_buf), comp_size};
#else
    auto buf = std::make_unique<uint8_t[]>( total_size );
    std::memcpy( buf.get(), m_speck_stream.first.get(), m_speck_stream.second );
    if( !speck::empty_buf(m_sperr_stream) ) { // UB to memcpy nullptr, so we do the test.
        std::memcpy( buf.get() + m_speck_stream.second,
                     m_sperr_stream.first.get(),
                     m_sperr_stream.second );
    }
    return {std::move(buf), total_size};
#endif
}



#ifdef QZ_TERM
void SPECK3D_Compressor::set_qz_level( int32_t q )
{
    m_qz_lev = q;
}
auto SPECK3D_Compressor::set_tolerance( double tol ) -> RTNType
{
    if( tol <= 0.0 )
        return RTNType::InvalidParam;
    else {
        m_tol = tol;
        m_sperr.set_tolerance( tol );
        return RTNType::Good;
    }
}
auto SPECK3D_Compressor::get_outlier_stats() const -> std::pair<size_t, size_t>
{
    return {m_num_outlier, m_sperr_stream.second};
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
