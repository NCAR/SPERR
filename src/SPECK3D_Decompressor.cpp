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
    auto content_buf = std::make_unique<uint8_t[]>( content_size );
    const auto decomp_size = ZSTD_decompress( content_buf.get(), content_size, p, len );
    if( ZSTD_isError( decomp_size ) || decomp_size != content_size )
        return RTNType::ZSTDError;
    const uint8_t* const ptr     = content_buf.get();
    const size_t         ptr_len = decomp_size;
#else
    const uint8_t* const ptr     = static_cast<const uint8_t*>(p);
    const size_t         ptr_len = len;
#endif

    // Step 1: extract SPECK stream from it
    m_speck_stream = {nullptr, 0};
    const auto speck_size = m_decoder.get_speck_stream_size(ptr);
    if( speck_size > ptr_len )
        return RTNType::WrongSize;
    auto buf = std::make_unique<uint8_t[]>( speck_size );
    std::memcpy( buf.get(), ptr, speck_size );
    m_speck_stream = {std::move(buf), speck_size};

    // Step 2: keep the volume dimension from the header
    auto dims = m_decoder.get_speck_stream_dims( ptr );
    m_dim_x   = dims[0];
    m_dim_y   = dims[1];
    m_dim_z   = dims[2];

    // Step 3: extract SPERR stream from it
#ifdef QZ_TERM
    m_sperr_stream = {nullptr, 0};
    if( speck_size < ptr_len ) {
        const uint8_t* const sperr_p = ptr + speck_size;
        const auto sperr_size = m_sperr.get_sperr_stream_size( sperr_p );
        if( sperr_size != ptr_len - speck_size )
            return RTNType::WrongSize;
        buf = std::make_unique<uint8_t[]>( sperr_size );
        std::memcpy( buf.get(), sperr_p, sperr_size );
        m_sperr_stream = {std::move(buf), sperr_size};
        m_sperr_los.clear();
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


auto SPECK3D_Decompressor::decompress() -> RTNType
{
    // Step 1: SPECK decode.
    if( speck::empty_buf(m_speck_stream) )
        return RTNType::Error;
    
    auto rtn = m_decoder.parse_encoded_bitstream( m_speck_stream.first.get(),
                                                  m_speck_stream.second );
    if( rtn != RTNType::Good )
        return rtn;

    m_decoder.set_bit_budget( size_t(m_bpp * float(m_dim_x * m_dim_y * m_dim_z)) );
    rtn = m_decoder.decode();
    if( rtn != RTNType::Good )
        return rtn;

    // Step 2: Inverse Wavelet Transform
    m_cdf.set_dims( m_dim_x, m_dim_y, m_dim_z );
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


template<typename T>
auto SPECK3D_Decompressor::get_decompressed_volume() const ->
                           std::pair<std::unique_ptr<T[]>, size_t>
{
    auto [vol, len]  = m_cdf.get_read_only_data();
    if( vol == nullptr || len == 0 )
        return {nullptr, 0};

    auto out_buf = std::make_unique<T[]>( len );
    auto begin   = speck::begin( vol );
    auto end     = speck::end( vol, len );
    std::copy( begin, end, speck::begin(out_buf) );

#ifdef QZ_TERM
    // If there are corrections for outliers, do it now!
    for( const auto& outlier : m_sperr_los ) {
        out_buf[outlier.location] += outlier.error;
    }
#endif

    return {std::move(out_buf), len};
}
template auto SPECK3D_Decompressor::get_decompressed_volume() const -> speck::smart_buffer_f;
template auto SPECK3D_Decompressor::get_decompressed_volume() const -> speck::smart_buffer_d;


template<typename T>
auto SPECK3D_Decompressor::scatter_chunk( T* big_vol, const std::array<size_t, 3>& vol_dim,
                                          const std::array<size_t, 6>& chunk ) const -> RTNType
{
    if( chunk[1] != m_dim_x || chunk[3] != m_dim_y || chunk[5] != m_dim_z )
        return RTNType::DimMismatch;

    auto [small_vol, len] = this->get_decompressed_volume<T>();
    size_t idx = 0;
    for( size_t z = chunk[4]; z < chunk[4] + chunk[5]; z++ ) {
      const size_t plane_offset = z * vol_dim[0] * vol_dim[1];
      for( size_t y = chunk[2]; y < chunk[2] + chunk[3]; y++ ) {
        const size_t col_offset = plane_offset + y * vol_dim[0];
        for( size_t x = chunk[0]; x < chunk[0] + chunk[1]; x++ )
          big_vol[ col_offset + x ] = small_vol[ idx++ ];
      }
    }

    return RTNType::Good;
}
template auto SPECK3D_Decompressor::scatter_chunk( float*, const std::array<size_t, 3>&,
                                                   const std::array<size_t, 6>& ) const -> RTNType;
template auto SPECK3D_Decompressor::scatter_chunk( double*, const std::array<size_t, 3>&,
                                                   const std::array<size_t, 6>& ) const -> RTNType;


auto SPECK3D_Decompressor::get_dims() const -> std::array<size_t, 3>
{
    return {m_dim_x, m_dim_y, m_dim_z};
}


#if 0
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
#endif









