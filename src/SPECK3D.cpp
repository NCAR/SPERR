#include "SPECK3D.h"
#include <cassert>
#include <cstring>
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
void speck::SPECK3D::set_dims( size_t x, size_t y, size_t z)
{
    // Sanity Check
    assert( m_coeff_len == 0 || m_coeff_len == x * y * z );
    m_dim_x = x;
    m_dim_y = y;
    m_dim_z = z;
    m_coeff_len = x * y * z;
}

void speck::SPECK3D::get_dims( size_t& x, size_t& y, size_t& z ) const
{
    x = m_dim_x;
    y = m_dim_y;
    z = m_dim_z;
}

void speck::SPECK3D::set_max_coeff_bits( uint16_t bits )
{
    m_max_coefficient_bits = bits;
}

void speck::SPECK3D::set_bit_budget( size_t budget )
{
    size_t mod = budget % 8;
    if( mod == 0 )
        m_budget = budget;
    else    // we can fill up the last byte
        m_budget = budget + 8 - mod;
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

    
int speck::SPECK3D::encode()
{
    assert( m_ready_to_encode() );
    m_encode_mode = true;

#ifdef PRINT
    std::cout << "---- Now encoding ----" << std::endl;
#endif

    m_initialize_sets_lists();

    m_bit_buffer.clear();
    m_bit_buffer.reserve( m_budget );
    auto max_coeff = speck::make_coeff_positive( m_coeff_buf, m_coeff_len, m_sign_array );

    // Even if m_max_coefficient_bit == 0, the quantization step would start from 1.0,
    //   then 0.5, then 0.25, etc. The algorithm will carry on just fine, just the bits
    //   saved in the first few iterations are unnecessary.
    m_max_coefficient_bits = uint16_t( std::log2(max_coeff) );
    m_threshold = std::pow( 2.0f, float(m_max_coefficient_bits) );
    int rtn = 0;
    for( size_t bitplane = 0; bitplane < 128; bitplane++ )
    {
        if( (rtn = m_sorting_pass()) )
            break;
        if( (rtn = m_refinement_pass()) )
            break;

        m_threshold *= 0.5f;
        m_clean_LIS();
    }

    return 0;
}


int speck::SPECK3D::decode()
{
    assert( m_ready_to_decode() );
    m_encode_mode = false;

    // By default, decode all the available bits
    if( m_budget == 0 )
        m_budget = m_bit_buffer.size();

#ifdef PRINT
    std::cout << "---- Now decoding ----" << std::endl;
#endif

#ifdef SPECK_USE_DOUBLE
    m_coeff_buf = std::make_unique<double[]>( m_coeff_len );
#else
    m_coeff_buf = std::make_unique<float[]>( m_coeff_len );
#endif

    // initialize coefficients to be zero, and signs to be all positive
    for( size_t i = 0; i < m_coeff_len; i++ )
        m_coeff_buf[i] = 0.0f;
    m_sign_array.assign( m_coeff_len, true );

    m_initialize_sets_lists();

    m_bit_idx = 0;
    m_threshold = std::pow( 2.0f, float(m_max_coefficient_bits) );
    int rtn = 0;
    for( size_t bitplane = 0; bitplane < 128; bitplane++ )
    {
        if( (rtn = m_sorting_pass()) )
            break;
        if( (rtn = m_refinement_pass()) )
            break;

        m_threshold *= 0.5f;

        m_clean_LIS();
    }

    // Restore coefficient signs
    for( size_t i = 0; i < m_coeff_len; i++ )
    {
        if( !m_sign_array[i] )
            m_coeff_buf[i] = -m_coeff_buf[i];
    }

    return 0;
}

    
void speck::SPECK3D::m_initialize_sets_lists()
{
    std::array<size_t, 3> num_of_parts; // how many times each dimension could be partitioned?
    m_num_of_partitions(  num_of_parts );
    size_t num_of_sizes = 1;
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
    big.length_x = uint32_t( m_dim_x ); // Truncate 64-bit int to 32-bit, but should be OK.
    big.length_y = uint32_t( m_dim_y ); // Truncate 64-bit int to 32-bit, but should be OK.
    big.length_z = uint32_t( m_dim_z ); // Truncate 64-bit int to 32-bit, but should be OK.

    const auto num_of_xforms_xy = speck::calc_num_of_xforms( std::min(m_dim_x, m_dim_y) );
    const auto num_of_xforms_z  = speck::calc_num_of_xforms( m_dim_z );
    size_t xf = 0;
    std::array<SPECKSet3D, 8> subsets;
    while( xf < num_of_xforms_xy && xf < num_of_xforms_z )
    {
        m_partition_S_XYZ( big, subsets );
        big = subsets[0];
        for( size_t i = 1; i < 8; i++ )
        {
            auto   parts = subsets[i].total_partitions();
            m_LIS[ parts ].push_back( subsets[i] );
        }
        xf++;
    }

    // One of these two conditions could happen if num_of_xforms_xy != num_of_xforms_z
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


int speck::SPECK3D::m_sorting_pass()
{
#ifdef PRINT
    std::cout << "--> sorting pass, threshold = " << m_threshold << std::endl;
#endif

    if( m_encode_mode )
    {   // Update the significance map based on the current threshold
        m_significance_map.assign( m_coeff_len, false );
        for( size_t i = 0; i < m_coeff_len; i++ )
        {
            if( m_coeff_buf[i] >= m_threshold )
                m_significance_map[i] = true;
        }
    }

    int rtn = 0;
    for( size_t tmp = 0; tmp < m_LIS.size(); tmp++ )
    {
        // From the end of m_LIS to its front
        size_t idx1 = m_LIS.size() - 1 - tmp;
        for( size_t idx2 = 0; idx2 < m_LIS[idx1].size(); idx2++ )
        {
            const auto& s = m_LIS[idx1][idx2];
            if( s.type != SetType::Garbage )
            {
                if( (rtn = m_process_S( idx1, idx2 )) )
                    return rtn;
            }
        }
    }

    return 0;
}


int speck::SPECK3D::m_refinement_pass()
{
#ifdef PRINT
    std::cout << "--> refinement pass, threshold = " << m_threshold << std::endl;
#endif

    int rtn = 0;
    for( auto& p : m_LSP )
    {
        if( p.signif == Significance::NewlySig )
            p.signif  = Significance::Sig;
        else
        {
            if( m_encode_mode )
            {
                // Output decision on refinement or not
                const auto idx = p.start_z * m_dim_x * m_dim_y + 
                                 p.start_y * m_dim_x + p.start_x;
                if( m_coeff_buf[idx] >= m_threshold ) 
                {
                    m_bit_buffer.push_back( true );
                    m_coeff_buf[idx] -= m_threshold;
                }
                else
                    m_bit_buffer.push_back( false );

                // Let's also see if we've reached the bit budget
                if( m_bit_buffer.size() >= m_budget )
                    return 1;
            }
            else
            {
                if( m_bit_idx >= m_budget || m_bit_idx >= m_bit_buffer.size() )
                    return 1;

                const auto bit = m_bit_buffer[ m_bit_idx++ ];
                const auto idx = p.start_z * m_dim_x * m_dim_y + 
                                 p.start_y * m_dim_x + p.start_x;
                m_coeff_buf[ idx ] += bit ? m_threshold * 0.5f : m_threshold * -0.5f;
            }
        }
    }

    return 0;
}


int speck::SPECK3D::m_process_S( size_t idx1, size_t idx2 )
{
    auto& set = m_LIS[idx1][idx2];
    int rtn = 0;

    if( m_encode_mode )
    {
        // decide the significance of this set
        set.signif = Significance::Insig;
        const size_t slice_size = m_dim_x * m_dim_y;
        for( auto z = set.start_z; z < (set.start_z + set.length_z); z++ )
        {
            const size_t slice_offset = z * slice_size;
            for( auto y = set.start_y; y < (set.start_y + set.length_y); y++ )
            {
                const size_t col_offset = slice_offset + y * m_dim_x;
                for( auto x = set.start_x; x < (set.start_x + set.length_x); x++ )
                {
                    const size_t idx = col_offset + x;
                    if( m_significance_map[ idx ] )
                    {
                        set.signif = Significance::Sig;
                        goto end_loop_label;
                    }
                }
            }
        }
        end_loop_label:
        // output the significance value 
        // "set" hasn't had a chance to be marked as NewlySig yet, so only need to
        // compare with Sig.
        auto bit = (set.signif == Significance::Sig);
        m_bit_buffer.push_back( bit );
        
        // Let's also see if we're reached the bit budget
        if( m_bit_buffer.size() >= m_budget )
            return 1;
    }
    else    // decoding mode
    {
        if( m_bit_idx >= m_budget || m_bit_idx >= m_bit_buffer.size() )
            return 1;
        auto bit   = m_bit_buffer[ m_bit_idx++ ];
        set.signif = bit ? Significance::Sig : Significance::Insig;
    }

    if( set.signif == Significance::Sig )
    {
        if( set.is_pixel() )
        {
            set.signif = Significance::NewlySig;
            if( m_encode_mode )
            {
                // Output pixel sign
                const auto idx = set.start_z * m_dim_x * m_dim_y + 
                                 set.start_y * m_dim_x + set.start_x;
                m_bit_buffer.push_back( m_sign_array[idx] );

                // Progressive quantization!
                m_coeff_buf[ idx ] -= m_threshold;

                // Let's also see if we're reached the bit budget
                if( m_bit_buffer.size() >= m_budget )
                    return 1;
            }
            else
            {
                if( m_bit_idx >= m_budget || m_bit_idx >= m_bit_buffer.size() )
                    return 1;

                const auto idx = set.start_z * m_dim_x * m_dim_y + 
                                 set.start_y * m_dim_x + set.start_x;
                m_sign_array[ idx ] = m_bit_buffer[ m_bit_idx++ ];

                // Progressive quantization!
                m_coeff_buf[ idx ] = 1.5f * m_threshold;

            }
            m_LSP.push_back( set );         // a copy is saved to m_LSP
        }
        else    // Not a pixel. Keep dividing it
        {
            if( (rtn = m_code_S( idx1, idx2 )) )
                return rtn;
        }
        set.type = SetType::Garbage;    // this current one is gonna be discarded.
        m_LIS_garbage_cnt[ set.total_partitions() ]++;
    }

    return 0;
}

    
int  speck::SPECK3D::m_code_S( size_t idx1, size_t idx2 )
{
    const auto& set = m_LIS[idx1][idx2];
    std::array< SPECKSet3D, 8 > subsets;
    m_partition_S_XYZ( set, subsets );
    int rtn = 0;
    for( size_t i = 0; i < 8; i++ )
    {
        const auto& s = subsets[i];
        if( !s.is_empty() )
        {
            auto   newidx1 = s.total_partitions();
            m_LIS[ newidx1 ].push_back( s );
            auto   newidx2 = m_LIS[ newidx1 ].size() - 1;
            if( (rtn = m_process_S( newidx1, newidx2 )) )
                return rtn;
        }
    }

    return 0;
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


void speck::SPECK3D::m_partition_S_XYZ( const SPECKSet3D& set, 
                     std::array<SPECKSet3D, 8>& subsets ) const
{
    const uint32_t split_x[2]{ set.length_x - set.length_x / 2,  set.length_x / 2 };
    const uint32_t split_y[2]{ set.length_y - set.length_y / 2,  set.length_y / 2 };
    const uint32_t split_z[2]{ set.length_z - set.length_z / 2,  set.length_z / 2 };

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
    const uint32_t split_x[2]{ set.length_x - set.length_x / 2,  set.length_x / 2 };
    const uint32_t split_y[2]{ set.length_y - set.length_y / 2,  set.length_y / 2 };

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
    const uint32_t split_z[2]{ set.length_z - set.length_z / 2,  set.length_z / 2 };

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


bool speck::SPECK3D::m_ready_to_encode() const
{
    if( m_coeff_buf == nullptr )
        return false;
    if( m_dim_x == 0 || m_dim_y == 0 || m_dim_z == 0)
        return false;
    if( m_budget == 0 )
        return false;

    return true;
}


bool speck::SPECK3D::m_ready_to_decode() const
{
    if( m_bit_buffer.empty() )
        return false;
    if( m_dim_x == 0 || m_dim_y == 0 || m_dim_z == 0 )
        return false;

    return true;
}


int speck::SPECK3D::write_to_disk( const std::string& filename ) const
{
    // Header definition:
    // information: dim_x,     dim_y,     dim_z,     image_mean,  max_coeff_bits,  bitstream
    // format:      uint32_t,  uint32_t,  uint32_t,  double       uint16_t,        packed_bytes
    const size_t header_size = 22;
    
    // Create and fill header buffer
    size_t pos = 0;
    uint32_t dims[3]{ uint32_t(m_dim_x), uint32_t(m_dim_y), uint32_t(m_dim_z) };
    buffer_type_c header = std::make_unique<char[]>( header_size );
    std::memcpy( header.get(), dims, sizeof(dims) );    
    pos += sizeof(dims);
    std::memcpy( header.get() + pos, &m_image_mean, sizeof(m_image_mean) );  
    pos += sizeof(m_image_mean);
    std::memcpy( header.get() + pos, &m_max_coefficient_bits, sizeof(m_max_coefficient_bits) );
    pos += sizeof(m_max_coefficient_bits);
    assert( pos == header_size );

    // Call the actual write function
    int rtn = m_write( header, header_size, filename.c_str() );
    return rtn;
}


int speck::SPECK3D::read_from_disk( const std::string& filename )
{
    // Header definition:
    // information: dim_x,     dim_y,     dim_z,     image_mean,  max_coeff_bits,  bitstream
    // format:      uint32_t,  uint32_t,  uint32_t,  double       uint16_t,        packed_bytes
    const size_t header_size = 22;

    // Create the header buffer, and read from file
    // Note that m_bit_buffer is filled by m_read().
    buffer_type_c header = std::make_unique<char[]>( header_size );
    int rtn = m_read( header, header_size, filename.c_str() );
    if( rtn )
        return rtn;

    // Parse the header
    uint32_t dims[3];
    size_t   pos = 0;
    std::memcpy( dims, header.get(), sizeof(dims) );    
    pos += sizeof(dims);
    std::memcpy( &m_image_mean, header.get() + pos, sizeof(m_image_mean) );
    pos += sizeof(m_image_mean);
    std::memcpy( &m_max_coefficient_bits, header.get() + pos, sizeof(m_max_coefficient_bits) );
    pos += sizeof(m_max_coefficient_bits);
    assert( pos == header_size );

    this->set_dims( size_t(dims[0]), size_t(dims[1]), size_t(dims[2]) );

    return 0;
}
