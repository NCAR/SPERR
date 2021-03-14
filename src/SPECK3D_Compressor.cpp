#include "SPECK3D_Compressor.h"

#include <cassert>
#include <cstring>

#ifdef USE_ZSTD
  #include "zstd.h"
#endif


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


template<typename T>
auto SPECK3D_Compressor::gather_chunk( const T* vol, const std::array<size_t, 3>& vol_dim,
                                       const std::array<size_t, 6>& chunk ) -> RTNType
{
    if( chunk[0] + chunk[1] > vol_dim[0] || 
        chunk[2] + chunk[3] > vol_dim[1] ||
        chunk[4] + chunk[5] > vol_dim[2] )
        return RTNType::DimMismatch;

    auto len = chunk[1] * chunk[3] * chunk[5];
    if( m_val_buf == nullptr || m_total_vals != len ) {
        m_total_vals = len;
        m_val_buf = std::make_unique<double[]>( len );
    }

    m_dim_x = chunk[1];
    m_dim_y = chunk[3];
    m_dim_z = chunk[5];

    size_t idx = 0;
    for( size_t z = chunk[4]; z < chunk[4] + chunk[5]; z++ ) {
      const size_t plane_offset = z * vol_dim[0] * vol_dim[1];
      for( size_t y = chunk[2]; y < chunk[2] + chunk[3]; y++ ) {
        const size_t col_offset = plane_offset + y * vol_dim[0];
        for( size_t x = chunk[0]; x < chunk[0] + chunk[1]; x++ )
          m_val_buf[idx++] = vol[col_offset + x];
      }
    }

    return RTNType::Good;
}
template auto SPECK3D_Compressor::gather_chunk( const double*, const std::array<size_t, 3>&,
                                                const std::array<size_t, 6>& ) -> RTNType;
template auto SPECK3D_Compressor::gather_chunk( const float*, const std::array<size_t, 3>&,
                                                const std::array<size_t, 6>& ) -> RTNType;
 

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
    std::vector<speck::Outlier> LOS; // List of OutlierS
    for( size_t i = 0; i < m_total_vals; i++ ) {
        auto diff = m_val_buf[i] - vol.first[i];
        if( std::abs(diff) > m_tol )
            LOS.emplace_back( i, diff );
    }

    // Now we encode any outlier that's found.
    if( !LOS.empty() ) {
        m_num_outlier = LOS.size();
        m_sperr.set_length( m_total_vals );
        m_sperr.use_outlier_list( std::move(LOS) );
        rtn  = m_sperr.encode();
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

    m_cdf.take_data( std::move(m_val_buf), m_total_vals );
    m_val_buf = nullptr; // give the moved-from object a specified state
    m_cdf.dwt3d();
    auto cdf_out = m_cdf.release_data();
    if( cdf_out.first == nullptr || cdf_out.second != m_total_vals )
        return RTNType::Error;

    m_encoder.set_image_mean( m_cdf.get_mean() );
    m_encoder.take_data( std::move(cdf_out.first), cdf_out.second );
    m_encoder.set_bit_budget( size_t(m_bpp * m_total_vals) );

    auto rtn = m_encoder.encode();
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
    auto buf = std::make_unique<uint8_t[]>( total_size );
    std::memcpy( buf.get(), m_speck_stream.first.get(), m_speck_stream.second );
    if( !speck::empty_buf(m_sperr_stream) ) { // UB to memcpy nullptr, so we do the test.
        std::memcpy( buf.get() + m_speck_stream.second,
                     m_sperr_stream.first.get(),
                     m_sperr_stream.second );
    }

#ifdef USE_ZSTD
    const size_t comp_buf_size = ZSTD_compressBound( total_size );
    auto comp_buf = std::make_unique<uint8_t[]>( comp_buf_size );
    const size_t comp_size = ZSTD_compress( comp_buf.get(), comp_buf_size, buf.get(), total_size,
                                            ZSTD_CLEVEL_DEFAULT + 6 );
    if( ZSTD_isError( comp_size ) )
        return {nullptr, 0};
    else
        return {std::move(comp_buf), comp_size};
#else
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
