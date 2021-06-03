#include <algorithm> // std::all_of()
#include <cassert>
#include <cstring>   // std::memcpy()
#include <cmath>     // std::sqrt()
#include <numeric>   // std::accumulate()

#include "Conditioner.h"

void speck::Conditioner::toggle_subtract_mean( bool b )
{
    m_s_mean = b;
}

void speck::Conditioner::toggle_divide_by_rms( bool b )
{
    m_d_rms = b;
}

void speck::Conditioner::set_num_strides( size_t s )
{
    m_num_strides = s;
}

auto speck::Conditioner::m_calc_mean( buffer_type_d& buf, size_t len ) const -> double
{
    assert( len % m_num_strides == 0 );

    m_stride_buf.resize( m_num_strides );
    const size_t stride_size = len / m_num_strides;

    for (size_t s = 0; s < m_num_strides; s++) {
        auto begin = speck::begin( buf ) + stride_size * s;
        auto end   = begin + stride_size;
        m_stride_buf[s] = std::accumulate( begin, end, double{0.0} ) / double(stride_size);
    }

    double sum = std::accumulate( m_stride_buf.begin(), m_stride_buf.end(), double{0.0} );

    return (sum / double(m_stride_buf.size()));
}

auto speck::Conditioner::m_calc_rms( buffer_type_d& buf, size_t len ) const -> double
{
    assert( len % m_num_strides == 0 );

    m_stride_buf.resize( m_num_strides );
    const size_t stride_size = len / m_num_strides;

    for (size_t s = 0; s < m_num_strides; s++) {
        auto begin = speck::begin( buf ) + stride_size * s;
        auto end   = begin + stride_size;
        m_stride_buf[s]  = std::accumulate( begin, end, double{0.0}, 
                           [](auto a, auto b){return a + b * b;} );
        m_stride_buf[s] /= double(stride_size);
    }

    double rms = std::accumulate( m_stride_buf.begin(), m_stride_buf.end(), double{0.0} );
    rms /= double(m_stride_buf.size());
    rms  = std::sqrt(rms);

    return rms;
}

auto speck::Conditioner::condition( buffer_type_d& buf, size_t len ) const
                         -> std::pair<RTNType, std::array<uint8_t, 17>>
{
    std::array<uint8_t, 17> meta;
    meta.fill( 0 );
    double mean = 0.0;
    double rms  = 1.0;
    const auto begin = speck::begin(buf);
    const auto end   = begin + len;

    if( len % m_num_strides != 0 )
        return {RTNType::WrongSize, meta};

    // If divide_by_rms is requested, need to make sure rms is non-zero
    if( m_d_rms ) {
        if( std::all_of( begin, end, [](auto v){return v == 0.0;} ) )
            return {RTNType::Error, meta};
    }

    // Perform subtract mean first
    if( m_s_mean ) {
        mean = m_calc_mean( buf, len );
        std::for_each( begin, end, [mean](auto& v){v -= mean;} );
    }

    // Perform divide_by_rms second
    if( m_d_rms ) {
        rms = m_calc_rms( buf, len );
        std::for_each( begin, end, [rms](auto& v){v /= rms;} );
    }

    // pack meta
    bool b8[8] = {m_s_mean, m_d_rms, false, false, false, false, false, false};
    speck::pack_8_booleans( meta[0], b8 );
    size_t pos = 1;
    std::memcpy( meta.data() + pos, &mean, sizeof(mean) );
    pos += sizeof(mean);
    std::memcpy( meta.data() + pos, &rms, sizeof(rms) );
    pos += sizeof(rms);
    assert( pos == m_meta_size );

    return {RTNType::Good, meta};
}

auto speck::Conditioner::inverse_condition( buffer_type_d& buf, size_t len, const uint8_t* meta ) const
                         -> RTNType
{
    if( len % m_num_strides != 0 )
        return RTNType::WrongSize;

    // unpack meta
    bool   b8[8];
    speck::unpack_8_booleans( b8, meta[0] );
    double mean = 0.0;
    double rms  = 1.0;
    size_t pos  = 1;
    std::memcpy( &mean, meta + pos, sizeof(mean) );
    pos += sizeof(mean);
    std::memcpy( &rms, meta + pos, sizeof(rms) );
    pos += sizeof(rms);
    assert( pos == m_meta_size );

    const auto begin = speck::begin(buf);
    const auto end   = begin + len;

    // Perform inverse of divide_by_rms, which is multiply by rms
    if( b8[1] ) {
        std::for_each( begin, end, [rms](auto& v){v *= rms;} );
    }

    // Perform inverse of subtract_mean, which is add mean.
    if( b8[0] ) {
        std::for_each( begin, end, [mean](auto& v){v += mean;} );
    }

    return RTNType::Good;
}







