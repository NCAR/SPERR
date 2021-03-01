#include "SPECK3D_OMP_C.h"

#include <cassert>
#include <cstring>
#include <numeric>   // std::accumulate()
#include <algorithm> // std::all_of()


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
    return std::accumulate( m_otlr_stats.begin(), m_otlr_stats.end(), sum, op );
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

    // Block the volume into smaller chunks
    auto chunks = speck::chunk_volume( {m_dim_x, m_dim_y, m_dim_z}, 
                                       {m_chunk_x, m_chunk_y, m_chunk_z} );
    m_num_chunks = chunks.size();

    // Create many compressor instances
    m_compressors.reserve( m_num_chunks );
    for( size_t i = 0; i < m_num_chunks; i++ )
        m_compressors.emplace_back( chunks[i][1], chunks[i][3], chunks[i][5] );

    // Ask these compressor instances to go grab their own chunks
    m_chunk_rtn.assign( m_num_chunks, RTNType::Good );

    // #pragma omp parallel for
    for( size_t i = 0; i < m_num_chunks; i++ ) {
        m_chunk_rtn[i] = m_compressors[i].gather_chunk( vol, {m_dim_x, m_dim_y, m_dim_z}, chunks[i] );
    }

    if( std::all_of( m_chunk_rtn.begin(), m_chunk_rtn.end(), 
        [](const auto& r){return r == RTNType::Good;} ) )
        return RTNType::Good;
    else
        return RTNType::Error;
}
template auto SPECK3D_OMP_C::use_volume( const float* ,  size_t ) -> RTNType;
template auto SPECK3D_OMP_C::use_volume( const double* , size_t ) -> RTNType;


auto SPECK3D_OMP_C::compress() -> RTNType
{
    // First we need to make sure that the compressor list isn't empty.
    if( m_compressors.empty() || m_num_chunks == 0 )
        return RTNType::Error;

    auto m_chunk_rtn = std::vector<RTNType>( m_num_chunks, RTNType::Good );
    m_encoded_streams.reserve( m_num_chunks );
    for( size_t i = 0; i < m_num_chunks; i++ )
        m_encoded_streams.emplace_back(nullptr, 0);

    // #pragma omp parallel for
    for( size_t i = 0; i < m_num_chunks; i++ ) {
        auto& compressor = m_compressors[i];

        // Note that we have already made sure that `m_tol` and `m_bpp` are valid, so
        // we don't need to check return values here.
#ifdef QZ_TERM
        compressor.set_qz_level(  m_qz_lev );
        compressor.set_tolerance( m_tol );
#else
        compressor.set_bpp( m_bpp );
#endif

        m_chunk_rtn[i] = compressor.compress();

        m_encoded_streams[i] = std::move(compressor.get_encoded_bitstream());
    }

    if( std::any_of( m_chunk_rtn.begin(), m_chunk_rtn.end(), 
        [](const auto& r){return r != RTNType::Good;} ) )
        return RTNType::Error;

    if( std::any_of( m_encoded_streams.begin(), m_encoded_streams.end(),
        [](const auto& s){return speck::empty_buf(s);} ) )
        return RTNType::Error;

    // Let's destroy the compressor instances to free some memory
    m_compressors.clear();

    return RTNType::Good;
}



