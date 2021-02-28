#include "SPECK3D_Compressor.h"

#include <cassert>
#include <cstring>


SPECK3D_Compressor::SPECK3D_Compressor( size_t x, size_t y, size_t z )
                  : m_dim_x(x), m_dim_y(y), m_dim_z(z), 
                    m_total_vals( x * y * z )
{
    m_cdf.set_dims( x, y, z );
    m_encoder.set_dims( x, y, z );
#ifdef QZ_TERM
    m_sperr.set_length( m_total_vals );
#endif
}


template< typename T >
auto SPECK3D_Compressor::copy_data( const T* p, size_t len ) -> RTNType
{
    if( len != m_total_vals )
        return RTNType::WrongSize;

    static_assert(std::is_floating_point<T>::value,
                  "!! Only floating point values are supported !!");

    m_val_buf = std::make_unique<double[]>( len );
    std::copy( p, p + len, speck::begin(m_val_buf) );

    return RTNType::Good;
}
template auto SPECK3D_Compressor::copy_data( const double*, size_t ) -> RTNType;
template auto SPECK3D_Compressor::copy_data( const float*,  size_t ) -> RTNType;


auto SPECK3D_Compressor::take_data( speck::buffer_type_d buf, size_t len ) -> RTNType
{
    if( len != m_total_vals )
        return RTNType::WrongSize;

    m_val_buf = std::move( buf );

    return RTNType::Good;
}


template<typename T>
auto SPECK3D_Compressor::gather_block( const T* vol, std::array<size_t, 3> vol_dim,
                                       std::array<size_t, 6> block ) -> RTNType
{
    if( block[1] != m_dim_x || block[3] != m_dim_y || block[5] != m_dim_z )
        return RTNType::DimMismatch;

    m_val_buf  = std::make_unique<double[]>( m_total_vals );
    size_t idx = 0;
    for( size_t z = block[4]; z < block[4] + block[5]; z++ ) {
      const size_t plane_offset = z * vol_dim[0] * vol_dim[1];
      for( size_t y = block[2]; y < block[2] + block[3]; y++ ) {
        const size_t col_offset = plane_offset + y * vol_dim[0];
        for( size_t x = block[0]; x < block[0] + block[1]; x++ )
          m_val_buf[idx++] = vol[col_offset + x];
      }
    }

    return RTNType::Good;
}
template auto SPECK3D_Compressor::gather_block( const double*, std::array<size_t, 3>,
                                                std::array<size_t, 6> ) -> RTNType;
template auto SPECK3D_Compressor::gather_block( const float*, std::array<size_t, 3>,
                                                std::array<size_t, 6> ) -> RTNType;
 

#ifdef QZ_TERM
auto SPECK3D_Compressor::compress() -> RTNType
{
    if( m_val_buf == nullptr || m_total_vals == 0 )
        return RTNType::Error;
    m_speck_stream = {nullptr, 0};
    m_sperr_stream = {nullptr, 0};
    m_num_outlier  = 0;

    // Note that we keep the original buffer untouched for outlier calculations later.
    m_cdf.copy_data( m_val_buf.get(), m_total_vals );
    m_cdf.dwt3d();
    auto cdf_out = m_cdf.release_data();
    m_encoder.set_image_mean( m_cdf.get_mean() );
    m_encoder.take_data( std::move(cdf_out.first), cdf_out.second );
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
    if( coeffs.first == nullptr || coeffs.second != m_total_vals )
        return RTNType::Error;
    m_cdf.take_data( std::move(coeffs.first), coeffs.second );
    m_cdf.idwt3d();

    // Now we find all the outliers!
    auto vol = m_cdf.get_read_only_data();
    if( vol.first == nullptr || vol.second != m_total_vals )
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
    return {std::move(buf), total_size};
}


#if 0
auto SPECK3D_Compressor::get_encoded_bitstream() const -> speck::smart_buffer_uint8
{
    // After receiving the bitstream from SPECK3D, this method does 3 things:
    // 1) prepend a proper header containing meta data.
    // 2) potentially append a block of data that performs outlier correction.
    // 3) potentially apply ZSTD on the entire memory block except the meta data.

    if( speck::empty_buf(m_speck_stream) )
        return {nullptr, 0};

    // Meta data definition:
    // the 1st byte records the current major version of SPECK, and
    // the 2nd byte records 8 booleans, with their meanings documented below:
    // 
    // bool_byte[0]  : if the rest of the stream is zstd compressed.
    // bool_byte[1]  : if this bitstream is for 3D (true) or 2D (false) data.
    // bool_byte[2]  : if there is error-bound data after the SPECK stream.
    // bool_byte[3-7]: unused 
    //
    uint8_t meta[2] = {uint8_t(SPECK_VERSION_MAJOR), 0};
    assert( sizeof(meta) == m_meta_size );
    bool metabool[8];
#ifdef USE_ZSTD
    metabool[0] = true;     // zstd
#else
    metabool[0] = false;    // no zstd
#endif
    metabool[1] = true;     // 3D
    if( speck::empty_buf(m_sperr_stream) )
        metabool[2] = false;// no sperr data attached
    else
        metabool[2] = true; // has sperr data attached 
    for( int i = 3; i < 8; i++ )
        metabool[i] = false;// unused
    speck::pack_8_booleans( meta[1], metabool );

#ifdef USE_ZSTD
    // If there's no SPERR stream, simply send SPECK3D stream to ZSTD.
    // If there IS SPERR stream, we need to combine SPECK3D and SPERR streams 
    // before sending to ZSTD.
    //
    const uint8_t* uncomp_buf  = nullptr;
    size_t         uncomp_size = 0;
    std::unique_ptr<uint8_t[]> buf_owner;
    if( speck::empty_buf(m_sperr_stream) ) {
        uncomp_buf  = m_speck_stream.first.get();
        uncomp_size = m_speck_stream.second;
    }
    else {
        uncomp_size = m_speck_stream.second + m_sperr_stream.second;
        buf_owner   = std::make_unique<uint8_t[]>(uncomp_size);
        uncomp_buf  = buf_owner.get();
        std::memcpy(buf_owner.get(), m_speck_stream.first.get(), m_speck_stream.second);
        std::memcpy(buf_owner.get() + m_speck_stream.second,
                    m_sperr_stream.first.get(),
                    m_sperr_stream.second);
    }

    const size_t comp_buf_size = ZSTD_compressBound( uncomp_size );
    auto comp_buf = std::make_unique<uint8_t[]>( m_meta_size + comp_buf_size );
    std::memcpy(comp_buf.get(), meta, m_meta_size);
    size_t comp_size = ZSTD_compress( comp_buf.get() + m_meta_size, comp_buf_size,
                                      uncomp_buf, uncomp_size,
                                      ZSTD_CLEVEL_DEFAULT + 3 );
    if( ZSTD_isError( comp_size ))
        return {nullptr, 0};
    else
        return {std::move(comp_buf), m_meta_size + comp_size};   
#else
    const size_t total_size = m_meta_size + m_speck_stream.second + m_sperr_stream.second;
    auto buf = std::make_unique<uint8_t[]>( total_size );
    std::memcpy( buf.get(), meta, m_meta_size );  // copy over metadata
    std::memcpy( buf.get() + m_meta_size,         // copy over speck stream
                 m_speck_stream.first.get(), 
                 m_speck_stream.second );
    if( !speck::empty_buf(m_sperr_stream) ) { // UB to memcpy nullptr, so we do the test.
        std::memcpy( buf.get() + m_meta_size + m_speck_stream.second,
                     m_sperr_stream.first.get(),
                     m_sperr_stream.second );
    }
    return {std::move(buf), total_size};
#endif
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
