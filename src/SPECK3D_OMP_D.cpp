
#include "SPECK3D_OMP_D.h"

#include <cassert>
#include <cstring>
#include <numeric>
#include <algorithm>


// Debug only
auto SPECK3D_OMP_D::take_chunk_bitstream( std::vector<speck::smart_buffer_uint8> chunks,
                                          std::array<size_t, 3> chunk_size ) -> RTNType
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

    // Each decompressor reports its dimension, so we know the whole volume dimension
    m_dim_x = 0;
    m_dim_y = 0;
    m_dim_z = 0;
    for( const auto& d : m_decompressors ) {
        auto dims = d.get_dims();
        m_dim_x  += dims[0];
        m_dim_y  += dims[1];
        m_dim_z  += dims[2];
    }
    m_chunk_x = chunk_size[0];
    m_chunk_y = chunk_size[1];
    m_chunk_z = chunk_size[2];

    if(std::all_of( use_rtn.begin(), use_rtn.end(), [](auto r){return r == RTNType::Good;} ))
        return RTNType::Good;
    else
        return RTNType::Error;
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














