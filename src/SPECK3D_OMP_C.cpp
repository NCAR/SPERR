#include "SPECK3D_OMP_C.h"

#include <cassert>
#include <cstring>
#include <numeric> // std::accumulate()


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
    m_bpp = bpp;
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
    const auto num_chunks = chunks.size();

    // Create many compressor instances
    m_compressors.reserve( num_chunks );
    for( size_t i = 0; i < num_chunks; i++ )
        m_compressors.emplace_back( chunks[i][1], chunks[i][3], chunks[i][5] );

    // Ask these compressor instances to go grab their own chunks
    // #pragma omp parallel for
    for( size_t i = 0; i < num_chunks; i++ ) {
        m_compressors[i].gather_chunk( vol, {m_dim_x, m_dim_y, m_dim_z}, chunks[i] );
    }

    return RTNType::Good;
}





