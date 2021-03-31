
#include "SPECK3D_OMP_D.h"

#include <cassert>
#include <cstring>
#include <numeric>
#include <algorithm>
#include <omp.h>


auto SPECK3D_OMP_D::set_bpp( float bpp ) -> RTNType
{
    if( bpp < 0.0 || bpp > 64.0 )
        return RTNType::InvalidParam;
    else {
        m_bpp = bpp;
        return RTNType::Good;
    }
}
    

void SPECK3D_OMP_D::set_num_threads( size_t n )
{
    if( n > 0 )
        m_num_threads = n;
}


auto SPECK3D_OMP_D::use_bitstream( const void* p, size_t total_len ) -> RTNType
{
    // This method parses the header of a bitstream and puts volume dimension and 
    // chunk size information in respective member variables.
    // It also stores the offset number to reach all chunks.
    // It does not, however, read the actual bitstream. The actual bitstream
    // will be provided when the decompress() method is called.

    const uint8_t* u8p = static_cast<const uint8_t*>( p );
    size_t loc = 0;

    // Parse Step 1: Major version number need to match
    uint8_t ver;
    std::memcpy( &ver, u8p + loc, sizeof(ver) );
    loc += sizeof(ver);
    if( ver / 10 != SPECK_VERSION_MAJOR )
        return RTNType::VersionMismatch;

    // Parse Step 2: ZSTD application needs to be consistent.
    bool b[8];
    speck::unpack_8_booleans( b, u8p[loc] );
    loc++;
#ifdef USE_ZSTD
    if( b[0] == false ) return {};
#else
    if( b[0] == true )  return {};
#endif
    
    // Parse Step 3: Extract volume and chunk dimensions
    uint32_t vcdim[6];
    std::memcpy( vcdim, u8p + loc, sizeof(vcdim) );
    loc += sizeof(vcdim);
    m_dim_x     = vcdim[0];
    m_dim_y     = vcdim[1];
    m_dim_z     = vcdim[2];
    m_chunk_x   = vcdim[3];
    m_chunk_y   = vcdim[4];
    m_chunk_z   = vcdim[5];
    if( m_total_vals != m_dim_x * m_dim_y * m_dim_z ) {
        m_total_vals  = m_dim_x * m_dim_y * m_dim_z;
        m_vol_buf.reset(nullptr);
    }

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

    // Sanity check: if the buffer size matches what the header claims
    const auto header_size  = m_header_magic + num_chunks * 4;
    const auto suppose_size = std::accumulate(chunk_sizes.begin(), chunk_sizes.end(), header_size);
    if( suppose_size != total_len )
        return RTNType::WrongSize;

    // We also calculate the offset value to address each bitstream chunk.
    m_offsets.assign( num_chunks + 1, 0 );
    m_offsets[0] = header_size;
    for( size_t i = 0; i < num_chunks; i++ )
        m_offsets[i + 1] = m_offsets[i] + chunk_sizes[i];

    // Finally, we keep a copy of the bitstream pointer
    m_bitstream_ptr = u8p;

    return RTNType::Good;
}


auto SPECK3D_OMP_D::decompress( const void* p ) -> RTNType
{
    if( m_dim_x   == 0 || m_dim_y   == 0 || m_dim_z   == 0 ||
        m_chunk_x == 0 || m_chunk_y == 0 || m_chunk_z == 0 )
        return RTNType::Error;
    if( p == nullptr || m_bitstream_ptr == nullptr )
        return RTNType::Error;
    if( static_cast<const uint8_t*>(p) != m_bitstream_ptr )
        return RTNType::Error;
        
    // Let's figure out the chunk information
    const auto chunks = speck::chunk_volume( {m_dim_x,   m_dim_y,   m_dim_z}, 
                                             {m_chunk_x, m_chunk_y, m_chunk_z} );
    const auto num_chunks = chunks.size();
    if( m_offsets.size() != num_chunks + 1 )
        return RTNType::Error;

    // Allocate a buffer to store the entire volume
    if( m_vol_buf == nullptr )
        m_vol_buf =  std::make_unique<double[]>( m_total_vals );

    // Create number of decompressor instances equal to the number of threads
    auto decompressors = std::vector<SPECK3D_Decompressor>( m_num_threads );
    auto chunk_rtn     = std::vector<RTNType>( num_chunks * 3, RTNType::Good );

    // Each compressor instance takes on a chunk
    //
    #pragma omp parallel for num_threads(m_num_threads)
    for( size_t i = 0; i < num_chunks; i++ ) {
        auto& decompressor = decompressors[ omp_get_thread_num() ];
        decompressor.set_bpp( m_bpp );
        chunk_rtn[ i * 3 ] = decompressor.use_bitstream( m_bitstream_ptr + m_offsets[i],
                                                         m_offsets[i+1]  - m_offsets[i] );

        chunk_rtn[ i*3+1 ] = decompressor.decompress();
        auto small_vol     = decompressor.get_decompressed_volume<double>();
        if( speck::empty_buf( small_vol ) )
            chunk_rtn[ i*3+2 ] = RTNType::Error;
        else{
            chunk_rtn[ i*3+2 ] = RTNType::Good;
            speck::scatter_chunk( m_vol_buf.get(), {m_dim_x, m_dim_y, m_dim_z}, 
                                  small_vol.first, chunks[i] );
        }
    }

    if(std::any_of( chunk_rtn.begin(), chunk_rtn.end(), [](auto r){return r != RTNType::Good;} ))
        return RTNType::Error; 
    else
        return RTNType::Good;
}


auto SPECK3D_OMP_D::release_data_volume() -> speck::smart_buffer_d
{
    return {std::move(m_vol_buf), m_total_vals};
}


template<typename T>
auto SPECK3D_OMP_D::get_data_volume() const -> std::pair<std::unique_ptr<T[]>, size_t>
{
    if( m_vol_buf == nullptr )
        return {nullptr, 0};

    auto buf = std::make_unique<T[]>( m_total_vals );
    std::copy( speck::begin(m_vol_buf), speck::begin(m_vol_buf) + m_total_vals, speck::begin(buf) );
    return {std::move(buf), m_total_vals};
}
template auto SPECK3D_OMP_D::get_data_volume() const -> std::pair<std::unique_ptr<float[]>, size_t>;
template auto SPECK3D_OMP_D::get_data_volume() const -> std::pair<std::unique_ptr<double[]>, size_t>;





