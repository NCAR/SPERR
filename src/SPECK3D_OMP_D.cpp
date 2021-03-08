
#include "SPECK3D_OMP_D.h"

#include <cassert>
#include <cstring>
#include <numeric>
#include <algorithm>


void SPECK3D_OMP_D::use_bitstream( const void* p, size_t len )
{

}


auto SPECK3D_OMP_D::decompress() -> RTNType
{
    const auto num_chunks = m_decompressors.size();
    if( num_chunks == 0 )
        return RTNType::Error;

    // Allocate a buffer to store the entire volume
    const size_t total_vals = m_dim_x * m_dim_y * m_dim_z;
    m_vol_buf = {std::make_unique<double[]>( total_vals ), total_vals};

    // Decompress each chunk
    auto chunk_rtn = std::vector<RTNType>( num_chunks, RTNType::Good );
    for( size_t i = 0; i < num_chunks; i++ ) {
        m_decompressors[i].set_bpp( m_bpp );
        chunk_rtn[i] = m_decompressors[i].decompress();
    }
    if(std::any_of( chunk_rtn.begin(), chunk_rtn.end(), [](auto r){return r != RTNType::Good;} ))
        return RTNType::Error; 

    // Put data from individual chunks to the whole volume buffer
    auto chunks = speck::chunk_volume( {m_dim_x,   m_dim_y,   m_dim_z}, 
                                       {m_chunk_x, m_chunk_y, m_chunk_z} );
    assert( chunks.size() == num_chunks );
    chunk_rtn.assign( num_chunks, RTNType::Good );
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
    std::copy(speck::begin(m_vol_buf), speck::end(m_vol_buf), speck::begin(buf));
    return {std::move(buf), m_vol_buf.second};
}
template auto SPECK3D_OMP_D::get_data_volume() const -> std::pair<std::unique_ptr<float[]>, size_t>;
template auto SPECK3D_OMP_D::get_data_volume() const -> std::pair<std::unique_ptr<double[]>, size_t>;


auto SPECK3D_OMP_D::m_parse_header( const uint8_t* p, size_t total_len ) -> std::vector<size_t>
{
    // This method parses the header of a bitstream and puts volume dimension and 
    // chunk size information in respective member variables.
    // It returns the size of all chunks.
    // Upon detecting an inconsistency in the header, it returns an empty vector.

    size_t  loc = 0;
    // Major version number need to match
    uint8_t ver;
    std::memcpy( &ver, p, sizeof(ver) );
    loc += sizeof(ver);
    if( ver % 10 != SPECK_VERSION_MAJOR )
        return {};

    // ZSTD application needs to be consistent.
    bool b[8];
    speck::unpack_8_booleans( b, p[loc] );
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
    std::memcpy( vcdim, p + loc, sizeof(vcdim) );
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
        std::memcpy( &len, p + loc, sizeof(len) );
        loc += sizeof(len);
        chunk_sizes[i] = loc;
    }

    // Finally, we test if the buffer size matches the header
    const auto header_size  = num_chunks * 4 + 1 + 1 + 24;
    const auto suppose_size = std::accumulate(chunk_sizes.begin(), chunk_sizes.end(), header_size);
    if( suppose_size != total_len )
        return {};
    else
        return std::move(chunk_sizes);
}



// Debug only
auto SPECK3D_OMP_D::take_chunk_bitstream( std::vector<speck::smart_buffer_uint8> chunks )
                                          -> RTNType
{
    const auto num_chunks = chunks.size();

    m_decompressors.reserve( num_chunks );
    for( size_t i = 0; i < num_chunks; i++ )
        m_decompressors.emplace_back();
    auto use_rtn = std::vector<RTNType>( num_chunks, RTNType::Good );

    // Each decompressor takes the bitstream
    for( size_t i = 0; i < num_chunks; i++ ) {
        use_rtn[i] = m_decompressors[i].use_bitstream( chunks[i].first.get(), chunks[i].second );
    }

    if(std::all_of( use_rtn.begin(), use_rtn.end(), [](auto r){return r == RTNType::Good;} ))
        return RTNType::Good;
    else
        return RTNType::Error;
}











