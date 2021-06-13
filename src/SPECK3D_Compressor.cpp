#include "SPECK3D_Compressor.h"

#include <cassert>
#include <cstring>


template< typename T >
auto SPECK3D_Compressor::copy_data( const T* p, size_t len, speck::dims_type dims ) -> RTNType
{
    static_assert(std::is_floating_point<T>::value,
                  "!! Only floating point values are supported !!");

    if( len != dims[0] * dims[1] * dims[2] )
        return RTNType::DimMismatch;

    m_val_buf.resize( len );
    std::copy( p, p + len, m_val_buf.begin() );

    m_dims = dims;

    return RTNType::Good;
}
template auto SPECK3D_Compressor::copy_data( const double*, size_t, speck::dims_type ) -> RTNType;
template auto SPECK3D_Compressor::copy_data( const float*,  size_t, speck::dims_type ) -> RTNType;


auto SPECK3D_Compressor::take_data( speck::vecd_type&& buf, speck::dims_type dims ) -> RTNType
{
    if( buf.size() != dims[0] * dims[1] * dims[2] )
        return RTNType::DimMismatch;

    m_val_buf = std::move( buf );
    m_dims    = dims;

    return RTNType::Good;
}


auto SPECK3D_Compressor::view_encoded_bitstream() const -> const std::vector<uint8_t>&
{
    return m_encoded_stream;
}


auto SPECK3D_Compressor::get_encoded_bitstream() -> std::vector<uint8_t>
{
    return std::move(m_encoded_stream);
}

 
#ifdef QZ_TERM
auto SPECK3D_Compressor::compress() -> RTNType
{
    if( m_val_buf.empty() ) 
        return RTNType::Error;
    m_condi_stream.clear();
    m_speck_stream.clear();
    m_sperr_stream.clear();
    m_num_outlier  = 0;
    const auto total_vals = m_dims[0] * m_dims[1] * m_dims[2];

    // Note that we keep the original buffer untouched for outlier calculations later.
    m_cdf.copy_data( m_val_buf.data(), total_vals, m_dims );
    m_cdf.dwt3d();
    auto cdf_out = m_cdf.release_data();
    m_encoder.take_data( std::move(cdf_out.first), cdf_out.second, m_dim_x, m_dim_y, m_dim_z );
    m_encoder.set_quantization_term_level( m_qz_lev );
    auto rtn = m_encoder.encode();
    if( rtn != RTNType::Good )
        return rtn;
    else {
        m_speck_stream = m_encoder.view_encoded_bitstream();
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
    auto vol = m_cdf.view_data();
    if( !speck::size_is( vol, m_total_vals ) )
        return RTNType::Error;
    m_LOS.clear();
    //
    // Observation: for some data points, the reconstruction error in double falls
    // below `m_tol`, while in float would fall above `m_tol`. (Github issue #78).
    // Solution: find those data points, and use their slightly reduced error as the new tolerance.
    //
    auto new_tol  = m_tol;
    if( m_tmp_diff.second < m_total_vals )
        m_tmp_diff = std::make_pair(std::make_unique<double[]>(m_total_vals), m_total_vals);
    auto& diff_v = m_tmp_diff.first;
    for( size_t i = 0; i < m_total_vals; i++ ) {
        diff_v[i] = m_val_buf[i] - vol.first[i];
        auto  f   = std::abs( float(m_val_buf[i]) - float(vol.first[i]) );
        if( f > m_tol && std::abs(diff_v[i]) <= m_tol )
            new_tol = std::min( new_tol, std::abs(diff_v[i]) );
    }
    for( size_t i = 0; i < m_total_vals; i++ ) {
        if( std::abs(diff_v[i]) >= new_tol )
            m_LOS.emplace_back( i, diff_v[i] );
    }
    m_sperr.set_tolerance( new_tol ); // Don't forget to pass in the new tolerance value!

    // Now we encode any outlier that's found.
    if( !m_LOS.empty() ) {
        m_num_outlier = m_LOS.size();
        m_sperr.set_length( m_total_vals );
        m_sperr.copy_outlier_list( m_LOS );
        rtn = m_sperr.encode();
        if( rtn != RTNType::Good )
            return rtn;
        m_sperr_stream = m_sperr.get_encoded_bitstream();
        if( speck::empty_buf(m_sperr_stream) )
            return RTNType::Error;
    }

    auto rtn = m_prepare_encoded_bitstream();

    return rtn;
}
#else
auto SPECK3D_Compressor::compress() -> RTNType
{
    if( m_val_buf.empty() ) 
        return RTNType::Error;
    m_speck_stream.clear(); 
    const auto total_vals = m_dims[0] * m_dims[1] * m_dims[2];

    // Step 1: data goes through the conditioner 
    m_conditioner.toggle_all_false();
    m_conditioner.toggle_subtract_mean( true );
    auto [rtn, condi_meta] = m_conditioner.condition( m_val_buf, total_vals );
    if( rtn != RTNType::Good )
        return rtn;

    // Copy conditioner meta data to `m_condi_stream`
    m_condi_stream.resize( condi_meta.size() );
    std::copy( condi_meta.begin(), condi_meta.end(), m_condi_stream.begin() );

    // Step 2: wavelet transform
    rtn = m_cdf.take_data( std::move(m_val_buf), m_dims );
    if( rtn != RTNType::Good )
        return rtn;
    m_cdf.dwt3d();
    auto cdf_out = m_cdf.release_data();

    // Step 3: SPECK encoding
    rtn = m_encoder.take_data(std::move(cdf_out), m_dims );
    if( rtn != RTNType::Good )
        return rtn;
    m_encoder.set_bit_budget( size_t(m_bpp * total_vals) );

    rtn = m_encoder.encode();
    if( rtn != RTNType::Good )
        return rtn;

    m_speck_stream = m_encoder.view_encoded_bitstream();
    if( m_speck_stream.empty() )
        return RTNType::Error;

    rtn = m_prepare_encoded_bitstream();

    return rtn;
}
#endif


#ifdef USE_ZSTD
auto SPECK3D_Compressor::m_prepare_encoded_bitstream() -> RTNType
{
#ifdef QZ_TERM
    const size_t total_size = m_condi_stream.size() + m_speck_stream.size() + m_sperr_stream.size();
#else
    const size_t total_size = m_condi_stream.size() + m_speck_stream.size();
#endif

    // Need to have a ZSTD Compression Context first
    if( m_cctx == nullptr ) {
        auto* ctx_p = ZSTD_createCCtx();
        if( ctx_p  == nullptr )
            return RTNType::ZSTDError;
        else
            m_cctx.reset(ctx_p);
    }

    m_zstd_buf.resize( total_size );
    std::copy( m_condi_stream.begin(), m_condi_stream.end(), m_zstd_buf.begin() );
    auto zstd_itr = m_zstd_buf.begin() + m_condi_stream.size();
    std::copy( m_speck_stream.begin(), m_speck_stream.end(), zstd_itr );
    zstd_itr += m_speck_stream.size();

#ifdef QZ_TERM
    std::copy( m_sperr_stream.begin(), m_sperr_stream.end(), zstd_itr );
    zstd_itr += m_sperr_stream.size();
#endif

    const size_t comp_buf_size = ZSTD_compressBound( total_size );
    m_encoded_stream.resize( comp_buf_size );
    const size_t comp_size = ZSTD_compressCCtx( m_cctx.get(),
                                                m_encoded_stream.data(),  comp_buf_size,
                                                m_zstd_buf.data(), total_size,
                                                ZSTD_CLEVEL_DEFAULT + 6 );
    if( ZSTD_isError( comp_size ) )
        return RTNType::ZSTDError;
    else {
        m_encoded_stream.resize( comp_size );
        return RTNType::Good;
    }
} // Finish the USE_ZSTD case

#else
// Start the no-ZSTD case

auto SPECK3D_Compressor::m_prepare_encoded_bitstream() -> RTNType
{
#ifdef QZ_TERM
    const size_t total_size = m_condi_stream.size() + m_speck_stream.size() + m_sperr_stream.size();
#else
    const size_t total_size = m_condi_stream.size() + m_speck_stream.size();
#endif

    m_encoded_stream.resize( total_size );
    std::copy( m_condi_stream.begin(), m_condi_stream.end(), m_encoded_stream.begin() );
    auto buf_itr = m_encoded_stream.begin() + m_condi_stream.size();
    std::copy( m_speck_stream.begin(), m_speck_stream.end(), buf_itr );
    buf_itr += m_speck_stream.size();

#ifdef QZ_TERM
    std::copy( m_sperr_stream.begin(), m_sperr_stream.end(), buf_itr );
    buf_itr += m_sperr_stream.size();
#endif

    return RTNType::Good;
}
#endif


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
