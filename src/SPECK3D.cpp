#include "SPECK3D.h"
#include <cassert>
#include <cmath>
#include <iostream>
#include <array>
#include <algorithm>


//
// Class SPECKSet3D
//
bool speck::SPECKSet3D::is_pixel() const
{
    return (length_x == 1 && length_y == 1 && length_z == 1);
}

bool speck::SPECKSet3D::is_empty() const
{
    return (length_z == 0 || length_y == 0 || length_x == 0);
}
    
//
// Class SPECK3D
//
void speck::SPECK3D::assign_dims( size_t x, size_t y, size_t z)
{
    // Sanity Check
    assert( m_coeff_len == 0 || m_coeff_len == x * y * z );
    m_dim_x = x;
    m_dim_y = y;
    m_dim_z = z;
    m_coeff_len = x * y * z;
}

void speck::SPECK3D::assign_max_coeff_bits( uint16_t bits )
{
    m_max_coefficient_bits = bits;
}

void speck::SPECK3D::assign_bit_budget( size_t budget )
{
    size_t mod = budget % 8;
    if( mod == 0 )
        m_budget = budget;
        else    // we can fill up the last byte
        m_budget = budget + 8 - mod;
}

uint16_t speck::SPECK3D::get_max_coeff_bits()   const
{
    return m_max_coefficient_bits;
}


void speck::SPECK3D::m_clean_LIS()
{
    std::vector<SPECKSet3D> tmp;

    for( size_t i = 0; i < m_LIS_garbage_cnt.size(); i++ )
    {
        // Only consolidate memory if the garbage amount is big enough, 
        // in both absolute and relative senses.
        if( m_LIS_garbage_cnt[i] >  m_vec_init_capacity && 
            m_LIS_garbage_cnt[i] >= m_LIS[i].size() / 2  )
        {
            tmp.clear();
            tmp.reserve( m_vec_init_capacity );
            for( const auto& s : m_LIS[i] )
                if( s.type != SetType::Garbage )
                    tmp.push_back( s );
            std::swap( m_LIS[i], tmp );
            m_LIS_garbage_cnt[i] = 0;
        }
    }
}

    
int  speck::SPECK3D::m_decide_set_significance( SPECKSet3D& set,
                     std::array<Significance, 8>& sigs )
{
    // If decoding, simply read a bit from the bit buffer.
    if( !m_encode_mode )
    {
        if( m_bit_idx >= m_budget || m_bit_idx >= m_bit_buffer.size() )
            return 1;

        auto bit   = m_bit_buffer[ m_bit_idx++ ];
        set.signif = bit ? Significance::Sig : Significance::Insig;
        return 0;
    }

    // If encoding, we do some comparison work
    set.signif = Significance::Insig;
    const size_t slice_size = m_dim_x * m_dim_y;
    std::vector< UINT > signif_idx;     // keep indices of detected significant coeffs
    for( auto z = set.start_z; z < (set.start_z + set.length_z); z++ )
    for( auto y = set.start_y; y < (set.start_y + set.length_y); y++ )
    for( auto x = set.start_x; x < (set.start_x + set.length_x); x++ )
    {
        size_t idx = z * slice_size + y * m_dim_x + x;
        if( m_significance_map[ idx ] )
        {
            set.signif = Significance::Sig;
            signif_idx.push_back( x );  
            signif_idx.push_back( y ); 
            signif_idx.push_back( z ); 
        }
    }

    // If there are significant coefficients, we also calculate which subband they
    // live in, and record that information in "sigs."
    if( !signif_idx.empty() )
    {
        for( size_t i = 0; i < sigs.size(); i++ )
            sigs[i] = speck::Significance::Insig;
        const auto detail_start_x = set.start_x + set.length_x - set.length_x / 2;
        const auto detail_start_y = set.start_y + set.length_y - set.length_y / 2;
        const auto detail_start_z = set.start_z + set.length_z - set.length_z / 2;
        for( size_t i = 0; i < signif_idx.size(); i += 3 )
        {
            auto x = signif_idx[i];
            auto y = signif_idx[i + 1];
            auto z = signif_idx[i + 2];
            size_t subband = 0;
            if( x >= detail_start_x )
                subband += 1;
            if( y >= detail_start_y )
                subband += 2;
            if( z >= detail_start_z )
                subband += 4;
            sigs[ subband ] = speck::Significance::Sig;
        }
    }

    return 0;
}
    

void speck::SPECK3D::m_partition_S( const SPECKSet3D& set, 
                     std::array<SPECKSet3D, 8>& subsets ) const
{
    const UINT split_x[2]{ set.length_x - set.length_x / 2,  set.length_x / 2 };
    const UINT split_y[2]{ set.length_y - set.length_y / 2,  set.length_y / 2 };
    const UINT split_z[2]{ set.length_z - set.length_z / 2,  set.length_z / 2 };

    for( size_t i = 0; i < 8; i++ )
    {
        subsets[i].part_level_x = set.part_level_x;
        if( split_x[1] > 0 )    
            (subsets[i].part_level_x)++;
        subsets[i].part_level_y = set.part_level_y;
        if( split_y[1] > 0 )
            (subsets[i].part_level_y)++;
        subsets[i].part_level_z = set.part_level_z;
        if( split_z[1] > 0 )
            (subsets[i].part_level_z)++;
    }

    //
    // The actual figuring out where it starts/ends part...
    //
    const size_t offsets[3]{ 1, 2, 4 };
    // subset (0, 0, 0)
    size_t sub_i  = 0 * offsets[0] + 0 * offsets[1] + 0 * offsets[2];
    auto& sub     = subsets[sub_i];
    sub.start_x   = set.start_x;                sub.length_x = split_x[0];
    sub.start_y   = set.start_y;                sub.length_y = split_y[0];
    sub.start_z   = set.start_z;                sub.length_z = split_z[0];

    // subset (1, 0, 0)
    sub_i         = 1 * offsets[0] + 0 * offsets[1] + 0 * offsets[2];
    sub           = subsets[sub_i];
    sub.start_x   = set.start_x + split_x[0];   sub.length_x = split_x[1];
    sub.start_y   = set.start_y;                sub.length_y = split_y[0];
    sub.start_z   = set.start_z;                sub.length_z = split_z[0];

    // subset (0, 1, 0)
    sub_i         = 0 * offsets[0] + 1 * offsets[1] + 0 * offsets[2];
    sub           = subsets[sub_i];
    sub.start_x   = set.start_x;                sub.length_x = split_x[0];
    sub.start_y   = set.start_y + split_y[0];   sub.length_y = split_y[1];
    sub.start_z   = set.start_z;                sub.length_z = split_z[0];

    // subset (1, 1, 0)
    sub_i         = 1 * offsets[0] + 1 * offsets[1] + 0 * offsets[2];
    sub           = subsets[sub_i];
    sub.start_x   = set.start_x + split_x[0];   sub.length_x = split_x[1];
    sub.start_y   = set.start_y + split_y[0];   sub.length_y = split_y[1];
    sub.start_z   = set.start_z;                sub.length_z = split_z[0];

    // subset (0, 0, 1)
    sub_i         = 0 * offsets[0] + 0 * offsets[1] + 1 * offsets[2];
    sub           = subsets[sub_i];
    sub.start_x   = set.start_x;                sub.length_x = split_x[0];
    sub.start_y   = set.start_y;                sub.length_y = split_y[0];
    sub.start_z   = set.start_z + split_z[0];   sub.length_z = split_z[1];

    // subset (1, 0, 1)
    sub_i         = 1 * offsets[0] + 0 * offsets[1] + 1 * offsets[2];
    sub           = subsets[sub_i];
    sub.start_x   = set.start_x + split_x[0];   sub.length_x = split_x[1];
    sub.start_y   = set.start_y;                sub.length_y = split_y[0];
    sub.start_z   = set.start_z + split_z[0];   sub.length_z = split_z[1];

    // subset (0, 1, 1)
    sub_i         = 0 * offsets[0] + 1 * offsets[1] + 1 * offsets[2];
    sub           = subsets[sub_i];
    sub.start_x   = set.start_x;                sub.length_x = split_x[0];
    sub.start_y   = set.start_y + split_y[0];   sub.length_y = split_y[1];
    sub.start_z   = set.start_z + split_z[0];   sub.length_z = split_z[1];

    // subset (1, 1, 1)
    sub_i         = 1 * offsets[0] + 1 * offsets[1] + 1 * offsets[2];
    sub           = subsets[sub_i];
    sub.start_x   = set.start_x + split_x[0];   sub.length_x = split_x[1];
    sub.start_y   = set.start_y + split_y[0];   sub.length_y = split_y[1];
    sub.start_z   = set.start_z + split_z[0];   sub.length_z = split_z[1];
}


int speck::SPECK3D::m_output_set_significance( const SPECKSet3D& set )
{
#ifdef PRINT
    if( set.signif == Significance::Sig )
        std::cout << "s1" << std::endl;
    else
        std::cout << "s0" << std::endl;
#endif

    auto bit = (set.signif == Significance::Sig);
    m_bit_buffer.push_back( bit );
    
    // Let's also see if we're reached the bit budget
    if( m_bit_buffer.size() >= m_budget )
        return 1;
    else
        return 0;
}
