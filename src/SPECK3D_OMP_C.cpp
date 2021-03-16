#include "SPECK3D_OMP_C.h"

#include <cassert>
#include <cstring>
#include <numeric>   // std::accumulate()
#include <algorithm> // std::all_of()
#include <omp.h>


void SPECK3D_OMP_C::set_dims( size_t x, size_t y, size_t z )
{
    m_dim_x = x;
    m_dim_y = y;
    m_dim_z = z;
}


void SPECK3D_OMP_C::prefer_chunk_size( size_t x, size_t y, size_t z )
{
    m_chunk_x = x;
    m_chunk_y = y;
    m_chunk_z = z;
}

    
void SPECK3D_OMP_C::set_num_threads( size_t n )
{
    if( n > 0 )
        m_num_threads = n;
}


#ifdef QZ_TERM
void SPECK3D_OMP_C::set_qz_level( int32_t q )
{
    m_qz_lev = q;
}
auto SPECK3D_OMP_C::set_tolerance( double t ) -> RTNType
{
    if( t <= 0.0 )
        return RTNType::InvalidParam;
    else {
        m_tol = t;
        return RTNType::Good;
    } 
}
auto SPECK3D_OMP_C::get_outlier_stats() const -> std::pair<size_t, size_t>
{
    using pair = std::pair<size_t, size_t>;
    pair sum{0, 0};
    auto op  = [](const pair& a, const pair& b) -> pair
               {return {a.first + b.first, a.second + b.second};};
    return std::accumulate( m_outlier_stats.begin(), m_outlier_stats.end(), sum, op );
}
#else
auto SPECK3D_OMP_C::set_bpp( float bpp ) -> RTNType
{
    if( bpp < 0.0 || bpp > 64.0 )
        return RTNType::InvalidParam;
    else {
        m_bpp = bpp;
        return RTNType::Good;
    }
}
#endif


template<typename T>
auto SPECK3D_OMP_C::use_volume( const T* vol, size_t len ) -> RTNType
{
    if( len != m_dim_x * m_dim_y * m_dim_z )
        return RTNType::WrongSize;

    // If preferred chunk size is not set, then use the volume size as chunk size.
    if( m_chunk_x == 0 || m_chunk_y == 0 || m_chunk_z == 0 ) {
        m_chunk_x = m_dim_x;
        m_chunk_y = m_dim_y;
        m_chunk_z = m_dim_z;
    }

    // Block the volume into smaller chunks
    auto chunks = speck::chunk_volume( {m_dim_x,   m_dim_y,   m_dim_z}, 
                                       {m_chunk_x, m_chunk_y, m_chunk_z} );
    const auto num_chunks = chunks.size();
    m_chunk_buffers.resize( num_chunks );
    for( auto& buf : m_chunk_buffers )
        buf.reset(nullptr);

    #pragma omp parallel for num_threads(m_num_threads)
    for( size_t i = 0; i < num_chunks; i++ ) {
        auto buf = speck::gather_chunk( vol, {m_dim_x, m_dim_y, m_dim_z}, chunks[i] );
        m_chunk_buffers[i] = std::move(buf);
    }

    if(std::any_of( m_chunk_buffers.begin(), m_chunk_buffers.end(), 
                    [](const auto& r){return r == nullptr;} ))
        return RTNType::Error;
    else
        return RTNType::Good;

}
template auto SPECK3D_OMP_C::use_volume( const float* ,  size_t ) -> RTNType;
template auto SPECK3D_OMP_C::use_volume( const double* , size_t ) -> RTNType;


auto SPECK3D_OMP_C::compress() -> RTNType
{
    // Need to make sure that the chunks are ready! 
    auto chunks = speck::chunk_volume( {m_dim_x,   m_dim_y,   m_dim_z}, 
                                       {m_chunk_x, m_chunk_y, m_chunk_z} );
    const auto num_chunks = chunks.size();
    if( m_chunk_buffers.size() != num_chunks )
        return RTNType::Error;
    if(std::any_of( m_chunk_buffers.begin(), m_chunk_buffers.end(), 
                    [](const auto& r){return r == nullptr;} ))
        return RTNType::Error;

    // Let's prepare some data structures for compression!
    auto compressors = std::vector<SPECK3D_Compressor>( m_num_threads );
    auto chunk_rtn   = std::vector<RTNType>( num_chunks, RTNType::Good );
    m_encoded_streams.clear();
    m_encoded_streams.resize( num_chunks );
    for( auto& s : m_encoded_streams )
        s = {nullptr, 0};
#ifdef QZ_TERM
    m_outlier_stats.clear();
    m_outlier_stats.assign( num_chunks, {0, 0} );
#endif

    // Each thread uses a compressor instance to work on a chunk.
    //
    #pragma omp parallel for num_threads(m_num_threads)
    for( size_t i  = 0; i < num_chunks; i++ ) {
        auto& compressor = compressors[ omp_get_thread_num() ];
        const auto buf_len = chunks[i][1] * chunks[i][3] * chunks[i][5];

        // The following few operations have no chance to fail.
        compressor.take_data( std::move(m_chunk_buffers[i]), buf_len,
                              chunks[i][1], chunks[i][3], chunks[i][5] );
#ifdef QZ_TERM
        compressor.set_qz_level(  m_qz_lev );
        compressor.set_tolerance( m_tol );
#else
        compressor.set_bpp( m_bpp );
#endif

        chunk_rtn[i]         = compressor.compress();
        m_encoded_streams[i] = compressor.get_encoded_bitstream();

#ifdef QZ_TERM
        m_outlier_stats[i] = compressor.get_outlier_stats();
#endif
    }

    if( std::any_of( chunk_rtn.begin(), chunk_rtn.end(), 
                     [](const auto& r){return r != RTNType::Good;} ) )
        return RTNType::Error;
    if( std::any_of( m_encoded_streams.begin(), m_encoded_streams.end(),
                     [](const auto& s){return speck::empty_buf(s);} ) )
        return RTNType::Error;

    return RTNType::Good;
}


auto SPECK3D_OMP_C::get_encoded_bitstream() const -> speck::smart_buffer_uint8
{
    //
    // This method packs a header and chunk bitstreams together to the caller.
    //
    auto header = m_generate_header();
    if( speck::empty_buf(header) )
        return header;

    auto total_size = header.second;
    for( const auto& s : m_encoded_streams )
        total_size += s.second;
    auto buf = std::make_unique<uint8_t[]>( total_size );

    std::memcpy( buf.get(), header.first.get(), header.second );
    auto loc = header.second;
    for( const auto& s : m_encoded_streams ) {
        std::memcpy( buf.get() + loc, s.first.get(), s.second );
        loc += s.second;
    }

    return {std::move(buf), total_size};
}


auto SPECK3D_OMP_C::m_generate_header() const -> speck::smart_buffer_uint8
{
    // The header would contain the following information
    //  -- a version number                     (1 byte)
    //  -- 8 booleans                           (1 byte)
    //  -- volume and chunk dimensions          (4 x 6 = 24 bytes)
    //  -- length of bitstream for each chunk   (4 x num_chunks)

    auto chunks = speck::chunk_volume( {m_dim_x, m_dim_y, m_dim_z}, 
                                       {m_chunk_x, m_chunk_y, m_chunk_z} );
    const auto num_chunks  = chunks.size();
    if( num_chunks != m_encoded_streams.size() )
        return {nullptr, 0};
    const auto header_size = m_header_magic + num_chunks * 4;
    auto header = std::make_unique<uint8_t[]>( header_size );
    size_t loc = 0;

    // Version number
    uint8_t ver = 10 * SPECK_VERSION_MAJOR + SPECK_VERSION_MINOR;
    std::memcpy( header.get() + loc, &ver, sizeof(ver) );
    loc += sizeof(ver);

    // 8 booleans: 
    // bool[0]  : if ZSTD is used
    // bool[1-7]: undefined
    bool b[8] = {false, false, false, false, false, false, false, false};
#ifdef USE_ZSTD
    b[0] = true;
#endif
    speck::pack_8_booleans( header[loc], b );
    loc += 1;

    // Volume and chunk dimensions
    uint32_t vcdim[6] = {uint32_t(m_dim_x),   uint32_t(m_dim_y),   uint32_t(m_dim_z), 
                         uint32_t(m_chunk_x), uint32_t(m_chunk_y), uint32_t(m_chunk_z)};
    std::memcpy( header.get() + loc, vcdim, sizeof(vcdim) );
    loc += sizeof(vcdim);

    // Length of bitstream for each chunk
    // Note that we use uint32_t to keep the length, and we need to make sure
    // that no chunk size is bigger than that.
    for( const auto& stream : m_encoded_streams ) {
        if( stream.second >= std::numeric_limits<uint32_t>::max() )
            return {nullptr, 0};
    }
    for( const auto& stream : m_encoded_streams ) {
        uint32_t len = stream.second;
        std::memcpy( header.get() + loc, &len, sizeof(len) );
        loc += sizeof(len);
    }
    assert( loc == header_size );

    return {std::move(header), header_size};
}


