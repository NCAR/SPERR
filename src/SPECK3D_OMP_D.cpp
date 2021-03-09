
#include "SPECK3D_OMP_D.h"

#include <cassert>
#include <cstring>
#include <numeric>
#include <algorithm>


auto SPECK3D_OMP_D::use_bitstream( const void* p, size_t len ) -> RTNType
{
    const auto chunk_sizes = m_parse_header( p, len );
    const auto num_chunks  = chunk_sizes.size();
    if( num_chunks == 0 )
        return RTNType::Error;

    // Each decompressor takes a chunk of the bitstream
    m_decompressors.reserve( num_chunks );
    for( size_t i = 0; i < num_chunks; i++ )
        m_decompressors.emplace_back();
    auto use_rtn = std::vector<RTNType>( num_chunks, RTNType::Good );
    const auto header_size   = m_header_magic + num_chunks * 4;
    const uint8_t* const u8p = static_cast<const uint8_t*>(p);

    #pragma omp parallel for
    for( size_t i = 0; i < num_chunks; i++ ) {
        // Where does this chunk start?
        // This calculation is duplicate but it's really cheap anyway.
        size_t offset = std::accumulate(chunk_sizes.begin(), chunk_sizes.begin() + i, header_size);
        use_rtn[i] = m_decompressors[i].use_bitstream( u8p + offset, chunk_sizes[i] );
    }

    if(std::all_of( use_rtn.begin(), use_rtn.end(), [](auto r){return r == RTNType::Good;} ))
        return RTNType::Good;
    else
        return RTNType::Error;
}


auto SPECK3D_OMP_D::decompress() -> RTNType
{
    if( m_dim_x   == 0 || m_dim_y   == 0 || m_dim_z   == 0 ||
        m_chunk_x == 0 || m_chunk_y == 0 || m_chunk_z == 0 )
        return RTNType::Error;
    const auto chunks = speck::chunk_volume( {m_dim_x,   m_dim_y,   m_dim_z}, 
                                             {m_chunk_x, m_chunk_y, m_chunk_z} );
    const auto num_chunks = chunks.size();

    if( m_decompressors.size() != num_chunks )
        return RTNType::Error;

    // Allocate a buffer to store the entire volume
    const size_t total_vals = m_dim_x * m_dim_y * m_dim_z;
    m_vol_buf = {std::make_unique<double[]>( total_vals ), total_vals};
    auto chunk_rtn = std::vector<RTNType>( num_chunks, RTNType::Good );

    // Decompress each chunk
    #pragma omp parallel for
    for( size_t i = 0; i < num_chunks; i++ ) {
        m_decompressors[i].set_bpp( m_bpp );
        chunk_rtn[i] = m_decompressors[i].decompress();
    }
    if(std::any_of( chunk_rtn.begin(), chunk_rtn.end(), [](auto r){return r != RTNType::Good;} ))
        return RTNType::Error; 

    // Put data from individual chunks to the whole volume buffer
    chunk_rtn.assign( num_chunks, RTNType::Good );
    #pragma omp parallel for
    for( size_t i = 0; i < num_chunks; i++ ) {
        chunk_rtn[i] = m_decompressors[i].scatter_chunk( m_vol_buf.first.get(),
                                                         {m_dim_x, m_dim_y, m_dim_z},
                                                         chunks[i] );
    }
    if(std::any_of( chunk_rtn.begin(), chunk_rtn.end(), [](auto r){return r != RTNType::Good;} ))
        return RTNType::Error; 

    // Destroy decompressor instances since they take huge amounts of memory
    m_decompressors.clear();

    return RTNType::Good;
}


auto SPECK3D_OMP_D::release_data_volume() -> speck::smart_buffer_d
{
    return std::move(m_vol_buf);
}


template<typename T>
auto SPECK3D_OMP_D::get_data_volume() const -> std::pair<std::unique_ptr<T[]>, size_t>
{
    if( speck::empty_buf(m_vol_buf) )
        return {nullptr, 0};

    auto buf = std::make_unique<T[]>( m_vol_buf.second );
    std::copy( speck::begin(m_vol_buf), speck::end(m_vol_buf), speck::begin(buf) );
    return {std::move(buf), m_vol_buf.second};
}
template auto SPECK3D_OMP_D::get_data_volume() const -> std::pair<std::unique_ptr<float[]>, size_t>;
template auto SPECK3D_OMP_D::get_data_volume() const -> std::pair<std::unique_ptr<double[]>, size_t>;


auto SPECK3D_OMP_D::m_parse_header( const void* buf, size_t total_len ) -> std::vector<size_t>
{
    // This method parses the header of a bitstream and puts volume dimension and 
    // chunk size information in respective member variables.
    // It returns the size of all chunks.
    // Upon detecting an inconsistency in the header, it returns an empty vector.

    const uint8_t* u8p = static_cast<const uint8_t*>( buf );
    size_t  loc = 0;

    // Major version number need to match
    uint8_t ver;
    std::memcpy( &ver, u8p + loc, sizeof(ver) );
    loc += sizeof(ver);
    if( ver / 10 != SPECK_VERSION_MAJOR )
        return {};

    // ZSTD application needs to be consistent.
    bool b[8];
    speck::unpack_8_booleans( b, u8p[loc] );
    loc++;
#ifdef USE_ZSTD
    if( b[0] == false ) 
        return {};
#else
    if( b[0] == true )  
        return {};
#endif
    
    // Extract volume and chunk dimensions
    uint32_t vcdim[6];
    std::memcpy( vcdim, u8p + loc, sizeof(vcdim) );
    loc += sizeof(vcdim);
    m_dim_x   = vcdim[0];
    m_dim_y   = vcdim[1];
    m_dim_z   = vcdim[2];
    m_chunk_x = vcdim[3];
    m_chunk_y = vcdim[4];
    m_chunk_z = vcdim[5];

    // Figure out how many chunks and their length
    auto chunks = speck::chunk_volume( {m_dim_x,   m_dim_y,   m_dim_z}, 
                                       {m_chunk_x, m_chunk_y, m_chunk_z} );
    const auto num_chunks = chunks.size();
    auto chunk_sizes = std::vector<size_t>( num_chunks, 0 );
    for( size_t i = 0; i < num_chunks; i++ ) {
        uint32_t len;
        std::memcpy( &len, u8p + loc, sizeof(len) );
        loc += sizeof(len);
        chunk_sizes[i] = len;
    }

    // Finally, we test if the buffer size matches what the header claims
    const auto header_size  = m_header_magic + num_chunks * 4;
    const auto suppose_size = std::accumulate(chunk_sizes.begin(), chunk_sizes.end(), header_size);
    if( suppose_size != total_len )
        return {};
    else
        return std::move(chunk_sizes);
}




