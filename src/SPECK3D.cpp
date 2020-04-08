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
    

size_t speck::SPECKSet3D::total_partitions() const
{
    return size_t(part_level_x) + size_t(part_level_y) + size_t(part_level_z); 
}

#ifdef PRINT
void speck::SPECKSet3D::print() const
{
    printf("  start: (%d, %d, %d), length: (%d, %d, %d). Part: (%d, %d, %d)\n",
            start_x, start_y, start_z, length_x, length_y, length_z,
            part_level_x, part_level_y, part_level_z              );
}
#endif

    
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

    
void speck::SPECK3D::m_initialize_sets_lists()
{
    std::array<size_t, 3> num_of_parts; // how many times each dimension is partitioned?
    m_num_of_partitions(  num_of_parts );
    size_t num_of_sizes = 3;
    for( size_t i = 0; i < 3; i++ )
        num_of_sizes += num_of_parts[i];

    // initialize LIS
    m_LIS.clear();
    m_LIS.resize( num_of_sizes );
    for( auto& v : m_LIS )
        v.reserve( m_vec_init_capacity );
    m_LIS_garbage_cnt.assign( num_of_sizes, 0 );

    // Starting from a set representing the whole volume, identify the smaller sets
    //   and put them in LIS accordingly.
    SPECKSet3D big;
    big.length_x = UINT( m_dim_x ); // Truncate 64-bit int to 32-bit, but should be OK.
    big.length_y = UINT( m_dim_y ); // Truncate 64-bit int to 32-bit, but should be OK.
    big.length_z = UINT( m_dim_z ); // Truncate 64-bit int to 32-bit, but should be OK.

    const auto num_of_xforms_xy = speck::calc_num_of_xforms( std::min(m_dim_x, m_dim_y) );
    const auto num_of_xforms_z  = speck::calc_num_of_xforms( m_dim_z );
    size_t xf = 0;
    std::array<SPECKSet3D, 8> subsets;
    for( xf = 0; xf < num_of_xforms_xy && xf < num_of_xforms_z; xf++ )
    {
        m_partition_S_XYZ( big, subsets );
        big = subsets[0];
        for( size_t i = 1; i < 8; i++ )
        {
            auto   parts = subsets[i].total_partitions();
            m_LIS[ parts ].push_back( subsets[i] );
        }
    }
    // One of the follow two conditions could happen if 
    //   num_of_xforms_xy != num_of_xforms_z
    if( xf < num_of_xforms_xy )
    {
        std::array<SPECKSet3D, 4> sub4;
        while( xf < num_of_xforms_xy )
        {
            m_partition_S_XY( big, sub4 );
            big = sub4[0];
            for( size_t i = 1; i < 4; i++ )
            {
                auto   parts = sub4[i].total_partitions();
                m_LIS[ parts ].push_back( sub4[i] );
            }
            xf++;
        }
    }
    else if( xf < num_of_xforms_z )
    {
        std::array<SPECKSet3D, 2> sub2;
        while( xf < num_of_xforms_z )
        {
            m_partition_S_Z( big, sub2 );
            big = sub2[0];
            auto   parts = sub2[1].total_partitions();
            m_LIS[ parts ].push_back( sub2[1] );
            xf++;
        }
    }
    // Right now big is the set that's most likely to be significant, so insert
    //   it at the front of it's corresponding vector. One-time expense.
    auto   parts = big.total_partitions();
    m_LIS[ parts ].insert( m_LIS[parts].cbegin(), big );

    // initialize LSP
    m_LSP.clear();
    m_LSP.reserve( m_vec_init_capacity );
}

    
void speck::SPECK3D::m_num_of_partitions( std::array<size_t, 3>& parts ) const
{
    size_t num_of_parts = 0;    // Num. of partitions we can do along X
    size_t dim = m_dim_x;
    while( dim > 1 )
    {
        num_of_parts++;
        dim -= dim / 2;
    }
    parts[0] = num_of_parts;

    num_of_parts = 0;       // Num. of partitions we can do along Y
    dim = m_dim_y;
    while( dim > 1 )
    {
        num_of_parts++;
        dim -= dim / 2;
    }
    parts[1] = num_of_parts;

    num_of_parts = 0;       // Num. of partitions we can do along Z
    dim = m_dim_z;
    while( dim > 1 )
    {
        num_of_parts++;
        dim -= dim / 2;
    }
    parts[2] = num_of_parts;
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
    

void speck::SPECK3D::m_partition_S_XYZ( const SPECKSet3D& set, 
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
    auto& sub0    = subsets[sub_i];
    sub0.start_x  = set.start_x;                sub0.length_x = split_x[0];
    sub0.start_y  = set.start_y;                sub0.length_y = split_y[0];
    sub0.start_z  = set.start_z;                sub0.length_z = split_z[0];

    // subset (1, 0, 0)
    sub_i         = 1 * offsets[0] + 0 * offsets[1] + 0 * offsets[2];
    auto& sub1    = subsets[sub_i];
    sub1.start_x  = set.start_x + split_x[0];   sub1.length_x = split_x[1];
    sub1.start_y  = set.start_y;                sub1.length_y = split_y[0];
    sub1.start_z  = set.start_z;                sub1.length_z = split_z[0];

    // subset (0, 1, 0)
    sub_i         = 0 * offsets[0] + 1 * offsets[1] + 0 * offsets[2];
    auto& sub2    = subsets[sub_i];
    sub2.start_x  = set.start_x;                sub2.length_x = split_x[0];
    sub2.start_y  = set.start_y + split_y[0];   sub2.length_y = split_y[1];
    sub2.start_z  = set.start_z;                sub2.length_z = split_z[0];

    // subset (1, 1, 0)
    sub_i         = 1 * offsets[0] + 1 * offsets[1] + 0 * offsets[2];
    auto& sub3    = subsets[sub_i];
    sub3.start_x  = set.start_x + split_x[0];   sub3.length_x = split_x[1];
    sub3.start_y  = set.start_y + split_y[0];   sub3.length_y = split_y[1];
    sub3.start_z  = set.start_z;                sub3.length_z = split_z[0];

    // subset (0, 0, 1)
    sub_i         = 0 * offsets[0] + 0 * offsets[1] + 1 * offsets[2];
    auto& sub4    = subsets[sub_i];
    sub4.start_x  = set.start_x;                sub4.length_x = split_x[0];
    sub4.start_y  = set.start_y;                sub4.length_y = split_y[0];
    sub4.start_z  = set.start_z + split_z[0];   sub4.length_z = split_z[1];

    // subset (1, 0, 1)
    sub_i         = 1 * offsets[0] + 0 * offsets[1] + 1 * offsets[2];
    auto& sub5    = subsets[sub_i];
    sub5.start_x  = set.start_x + split_x[0];   sub5.length_x = split_x[1];
    sub5.start_y  = set.start_y;                sub5.length_y = split_y[0];
    sub5.start_z  = set.start_z + split_z[0];   sub5.length_z = split_z[1];

    // subset (0, 1, 1)
    sub_i         = 0 * offsets[0] + 1 * offsets[1] + 1 * offsets[2];
    auto& sub6    = subsets[sub_i];
    sub6.start_x  = set.start_x;                sub6.length_x = split_x[0];
    sub6.start_y  = set.start_y + split_y[0];   sub6.length_y = split_y[1];
    sub6.start_z  = set.start_z + split_z[0];   sub6.length_z = split_z[1];

    // subset (1, 1, 1)
    sub_i         = 1 * offsets[0] + 1 * offsets[1] + 1 * offsets[2];
    auto& sub7    = subsets[sub_i];
    sub7.start_x  = set.start_x + split_x[0];   sub7.length_x = split_x[1];
    sub7.start_y  = set.start_y + split_y[0];   sub7.length_y = split_y[1];
    sub7.start_z  = set.start_z + split_z[0];   sub7.length_z = split_z[1];
}
    

void speck::SPECK3D::m_partition_S_XY( const SPECKSet3D& set, 
                     std::array<SPECKSet3D, 4>& subsets ) const
{
    const UINT split_x[2]{ set.length_x - set.length_x / 2,  set.length_x / 2 };
    const UINT split_y[2]{ set.length_y - set.length_y / 2,  set.length_y / 2 };

    for( size_t i = 0; i < 4; i++ )
    {
        subsets[i].part_level_x = set.part_level_x;
        if( split_x[1] > 0 )    
            (subsets[i].part_level_x)++;
        subsets[i].part_level_y = set.part_level_y;
        if( split_y[1] > 0 )
            (subsets[i].part_level_y)++;
        subsets[i].part_level_z = set.part_level_z;
    }

    //
    // The actual figuring out where it starts/ends part...
    //
    const size_t offsets[3]{ 1, 2, 4 };
    // subset (0, 0, 0)
    size_t sub_i  = 0 * offsets[0] + 0 * offsets[1] + 0 * offsets[2];
    auto& sub0    = subsets[sub_i];
    sub0.start_x  = set.start_x;                sub0.length_x = split_x[0];
    sub0.start_y  = set.start_y;                sub0.length_y = split_y[0];
    sub0.start_z  = set.start_z;                sub0.length_z = set.length_z;

    // subset (1, 0, 0)
    sub_i         = 1 * offsets[0] + 0 * offsets[1] + 0 * offsets[2];
    auto& sub1    = subsets[sub_i];
    sub1.start_x  = set.start_x + split_x[0];   sub1.length_x = split_x[1];
    sub1.start_y  = set.start_y;                sub1.length_y = split_y[0];
    sub1.start_z  = set.start_z;                sub1.length_z = set.length_z;

    // subset (0, 1, 0)
    sub_i         = 0 * offsets[0] + 1 * offsets[1] + 0 * offsets[2];
    auto& sub2    = subsets[sub_i];
    sub2.start_x  = set.start_x;                sub2.length_x = split_x[0];
    sub2.start_y  = set.start_y + split_y[0];   sub2.length_y = split_y[1];
    sub2.start_z  = set.start_z;                sub2.length_z = set.length_z;

    // subset (1, 1, 0)
    sub_i         = 1 * offsets[0] + 1 * offsets[1] + 0 * offsets[2];
    auto& sub3    = subsets[sub_i];
    sub3.start_x  = set.start_x + split_x[0];   sub3.length_x = split_x[1];
    sub3.start_y  = set.start_y + split_y[0];   sub3.length_y = split_y[1];
    sub3.start_z  = set.start_z;                sub3.length_z = set.length_z;
}
    

void speck::SPECK3D::m_partition_S_Z( const SPECKSet3D& set, 
                     std::array<SPECKSet3D, 2>& subsets ) const
{
    const UINT split_z[2]{ set.length_z - set.length_z / 2,  set.length_z / 2 };

    for( size_t i = 0; i < 2; i++ )
    {
        subsets[i].part_level_x = set.part_level_x;
        subsets[i].part_level_y = set.part_level_y;
        subsets[i].part_level_z = set.part_level_z;
        if( split_z[1] > 0 )
            (subsets[i].part_level_z)++;
    }

    //
    // The actual figuring out where it starts/ends part...
    //
    // subset (0, 0, 0)
    auto& sub0    = subsets[0];
    sub0.start_x  = set.start_x;                sub0.length_x = set.length_x;
    sub0.start_y  = set.start_y;                sub0.length_y = set.length_y;
    sub0.start_z  = set.start_z;                sub0.length_z = split_z[0];

    // subset (0, 0, 1)
    auto& sub1    = subsets[1];
    sub1.start_x  = set.start_x;                sub1.length_x = set.length_x;
    sub1.start_y  = set.start_y;                sub1.length_y = set.length_y;
    sub1.start_z  = set.start_z + split_z[0];   sub1.length_z = split_z[1];
}


int speck::SPECK3D::m_output_pixel_sign( const SPECKSet3D& pixel )
{
    const auto idx = pixel.start_z * m_dim_x * m_dim_y + 
                     pixel.start_y * m_dim_x + pixel.start_x;

#ifdef PRINT
    if( m_sign_array[ idx ] )
        std::cout << "p1" << std::endl;
    else
        std::cout << "p0" << std::endl;
#endif

    m_bit_buffer.push_back( m_sign_array[idx] );

    // Progressive quantization!
    m_coeff_buf[ idx ] -= m_threshold;

    // Let's also see if we're reached the bit budget
    if( m_bit_buffer.size() >= m_budget )
        return 1;
    else
        return 0;
}


int speck::SPECK3D::m_input_pixel_sign( const SPECKSet3D& pixel )
{
    if( m_bit_idx >= m_budget || m_bit_idx >= m_bit_buffer.size() )
        return 1;

    const auto idx = pixel.start_z * m_dim_x * m_dim_y +
                     pixel.start_y * m_dim_x + pixel.start_x;
    m_sign_array[ idx ] = m_bit_buffer[ m_bit_idx++ ];

    // Progressive quantization!
    m_coeff_buf[ idx ] = 1.5f * m_threshold;

#ifdef PRINT
    auto bit = m_sign_array[ idx ];
    std::string str = bit ? "p1" : "p0";
    std::cout << str << std::endl;
#endif

    return 0;
}


int speck::SPECK3D::m_output_refinement( const SPECKSet3D& pixel )
{
    const auto idx = pixel.start_z * m_dim_x * m_dim_y +
                     pixel.start_y * m_dim_x + pixel.start_x;

    if( m_coeff_buf[idx] >= m_threshold ) 
    {
        m_bit_buffer.push_back( true );
#ifdef PRINT
        std::cout << "r1" << std::endl;
#endif
        m_coeff_buf[idx] -= m_threshold;
    }
    else
    {
        m_bit_buffer.push_back( false );
#ifdef PRINT
        std::cout << "r0" << std::endl;
#endif
    }

    // Let's also see if we're reached the bit budget
    if( m_bit_buffer.size() >= m_budget )
        return 1;
    else
        return 0;
}


int speck::SPECK3D::m_input_refinement( const SPECKSet3D& pixel )
{
    if( m_bit_idx >= m_budget || m_bit_idx >= m_bit_buffer.size() )
        return 1;

    const auto bit = m_bit_buffer[ m_bit_idx++ ];
    const auto idx = pixel.start_z * m_dim_x * m_dim_y + 
                     pixel.start_y * m_dim_x + pixel.start_x;
    m_coeff_buf[ idx ] += bit ? m_threshold * 0.5f : m_threshold * -0.5f;

#ifdef PRINT
    if( bit )
        std::cout << "r1" << std::endl;
    else
        std::cout << "r0" << std::endl;
#endif

    return 0;
}
    

void speck::SPECK3D::action()
{
    m_initialize_sets_lists();
    printf("m_LIS length: %lu\n", m_LIS.size() );
    size_t num_of_sets = 0;
    for( const auto& l : m_LIS )
        num_of_sets += l.size();
    printf("m_LIS contains set total number: %lu\n", num_of_sets );

    for( const auto& l : m_LIS )
    {
        if( !l.empty() )
        {
            printf("new length:\n");
            for( const auto& s : l )
                s.print();
        }
    }
}
