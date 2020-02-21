#include "SPECK2D.h"
#include <cassert>
#include <cmath>
#include <iostream>
#include <array>
#include <algorithm>


void speck::SPECK2D::assign_coeffs( double* ptr )
{
    m_coeff_buf.reset( ptr );
}

template<typename T>
void speck::SPECK2D::copy_coeffs( const T* p )
{
    static_assert( std::is_floating_point<T>::value, 
                   "!! Only floating point values are supported !!" );
    assert( m_dim_x > 0 && m_dim_y > 0 );

    long num_of_vals = m_dim_x * m_dim_y;
    m_coeff_buf = std::make_unique<double[]>( num_of_vals );
    for( long i = 0; i < num_of_vals; i++ )
        m_coeff_buf[i] = p[i];
}
template void speck::SPECK2D::copy_coeffs( const float*  ); 
template void speck::SPECK2D::copy_coeffs( const double* );

void speck::SPECK2D::assign_mean_dims( double m, long dx, long dy )
{
    m_data_mean = m;
    m_dim_x     = dx;
    m_dim_y     = dy;
}
    
int speck::SPECK2D::speck2d()
{
    assert( m_coeff_buf != nullptr );               // sanity check
    assert( m_dim_x > 0 && m_dim_y > 0 );           // sanity check

    // Let's do some preparation: gather some values
    long num_of_vals = m_dim_x * m_dim_y;
    auto max_coeff   = speck::make_positive( m_coeff_buf.get(), num_of_vals, m_sign_array );
    long max_coefficient_bits = long(std::log2(max_coeff));
    long num_of_parts         = m_num_of_partitions();
    long num_of_xforms        = speck::calc_num_of_xforms( std::min( m_dim_x, m_dim_y) );

    // Still preparing: lists and sets
    m_LIS.clear();
    m_LIS.resize( num_of_parts + 1 );
    for( auto& v : m_LIS )  // Avoid frequent memory allocations.
        v.reserve( 8 );
    m_LIS_garbage_cnt.assign( num_of_parts + 1, 0 );
    m_LSP.reserve( 8 );
    SPECKSet2D S( SPECKSetType::TypeS );
    S.part_level = num_of_xforms;
    m_calc_set_size( S, 0 );      // Populate other data fields of S.
    m_LIS[ S.part_level ].push_back( S );

    m_I.part_level = num_of_xforms;
    m_I.start_x    = S.length_x;
    m_I.start_y    = S.length_y;
    m_I.length_x   = m_dim_x;
    m_I.length_y   = m_dim_y;

    // Get ready for the quantization loop!  L1598 of speck.c
    m_threshold = std::pow( 2.0, double(max_coefficient_bits) );



    return 0;
}
    
    
//
// Private methods
//
void speck::SPECK2D::m_sorting_pass( )
{
    // Update the significance map based on the current threshold
    speck::update_significance_map( m_coeff_buf.get(), m_dim_x * m_dim_y, m_threshold, 
                                    m_significance_map );

    for( long idx1 = m_LIS.size() - 1; idx1 >= 0; idx1-- )
        for( long idx2  = 0; idx2 < m_LIS[idx1].size(); idx2++ )
        {
            if( !m_LIS[idx1][idx2].garbage )
                m_process_S( idx1, idx2 );
        }

    // ProcessI()
}


void speck::SPECK2D::m_process_S( long idx1, long idx2 )
{
    auto& set = m_LIS[idx1][idx2];

    m_output_set_significance( set );   // It also assigns the significance value to the set

    if( set.signif == Significance::Sig || set.signif == Significance::NewlySig )
    {
        if( set.is_pixel() )
        {
            set.signif = Significance::NewlySig;
            m_output_pixel_sign( set );
            m_LSP.push_back( set ); // A copy is saved to m_LSP.
            set.garbage = true;     // This particular object will be discarded.
        }
        else
        {
            m_code_S( idx1, idx2 );
            set.garbage = true;         // This particular object will be discarded.
        }
    }
}


void speck::SPECK2D::m_code_S( long idx1, long idx2 )
{
    const auto& set = m_LIS[idx1][idx2];

    std::array< SPECKSet2D, 4 > subsets;
    m_partition_S( set, subsets );
    for( auto& s : subsets )
    {
        m_LIS[ s.part_level ].push_back( s );
        m_process_S( s.part_level, m_LIS[s.part_level].size() - 1 );
    }
}


void speck::SPECK2D::m_partition_S( const SPECKSet2D& set, std::array<SPECKSet2D, 4>& list ) const
{
    // The top-left set will have these bigger dimensions in case that 
    // the current set has odd dimensions.
    const auto bigger_x = set.length_x - (set.length_x / 2);
    const auto bigger_y = set.length_y - (set.length_y / 2);

    // Put generated subsets in the list the same order as did in QccPack.
    auto& TL      = list[3];               // Top left set
    TL.part_level = set.part_level + 1;
    TL.start_x    = set.start_x;
    TL.start_y    = set.start_y;
    TL.length_x   = bigger_x;
    TL.length_y   = bigger_y;

    auto& TR      = list[2];               // Top right set
    TR.part_level = set.part_level + 1;
    TR.start_x    = set.start_x    + bigger_x;
    TR.start_y    = set.start_y;
    TR.length_x   = set.length_x   - bigger_x;
    TR.length_y   = bigger_y;

    auto& BL      = list[1];               // Bottom left set
    BL.part_level = set.part_level + 1;
    BL.start_x    = set.start_x;
    BL.start_y    = set.start_y    + bigger_x;
    BL.length_x   = set.length_x;
    BL.length_y   = set.length_y   - bigger_y;

    auto& BR      = list[0];               // Bottom right set
    BR.part_level = set.part_level + 1;
    BR.start_x    = set.start_x    + bigger_x;
    BR.start_y    = set.start_y    + bigger_x;
    BR.length_x   = set.length_x   - bigger_x;
    BR.length_y   = set.length_y   - bigger_y;
}


void speck::SPECK2D::m_partition_I( std::array<SPECKSet2D, 3>& subsets )
{
    const auto current_lev = m_I.part_level;
    long approx_len_x, detail_len_x;
    long approx_len_y, detail_len_y;
    speck::calc_approx_detail_len( m_dim_x, current_lev, approx_len_x, detail_len_x );
    speck::calc_approx_detail_len( m_dim_y, current_lev, approx_len_y, detail_len_y );

    // specify the subsets following the same order in QccPack
    auto& BR      = subsets[0];         // Bottom right
    BR.part_level = current_lev;
    BR.start_x    = approx_len_x;
    BR.start_y    = approx_len_y;
    BR.length_x   = detail_len_x;
    BR.length_y   = detail_len_y;

    auto& TR      = subsets[1];         // Top right
    TR.part_level = current_lev;
    TR.start_x    = approx_len_x;
    TR.start_y    = 0;
    TR.length_x   = detail_len_x;
    TR.length_y   = approx_len_y;

    auto& BL      = subsets[2];         // Bottom left
    BL.part_level = current_lev;
    BL.start_x    = 0;
    BL.start_y    = approx_len_y;
    BL.length_x   = approx_len_x;
    BL.length_y   = detail_len_y; 

    // Also update m_I
    m_I.part_level--;
    m_I.start_x += detail_len_x;
    m_I.start_y += detail_len_y;
}


void speck::SPECK2D::m_process_I()
{
}


// It outputs by printing out the value right now.
void speck::SPECK2D::m_output_set_significance( SPECKSet2D& set ) const
{
    set.signif = Significance::Insig;

    // For TypeS sets, we test an obvious rectangle specified by this set.
    if( set.type() == SPECKSetType::TypeS )
    {
        for( long y = set.start_y; y < (set.start_y + set.length_y) &&
                              set.signif == Significance::Insig; y++ )
            for( long x = set.start_x; x < (set.start_x + set.length_x) &&
                                  set.signif == Significance::Insig; x++ )
            {
                auto idx = y * m_dim_x + x;
                if( m_significance_map[ idx ] )
                    set.signif = Significance::Sig;
            }
    }
    else    // For TypeI sets, we need to test two rectangles!
    {
        // First rectangle: directly to the right of the missing top-left corner
        for( long y = 0; y < set.start_y && set.signif == Significance::Insig; y++ )
            for( long x = set.start_x; x < m_dim_x && set.signif == Significance::Insig; x++ )
            {
                auto idx = y * m_dim_x + x;
                if( m_significance_map[ idx ] )
                    set.signif = Significance::Sig;
            }

        // Second rectangle: the rest area at the bottom
        // Note: this rectangle is stored in a contiguous chunk of memory :)
        for( long i = set.start_y * m_dim_x; i < m_dim_x * m_dim_y && 
                                             set.signif == Significance::Insig; i++ )
        {
            if( m_significance_map[ i ] )
                set.signif = Significance::Sig;
        }
    }

    if( set.signif == Significance::Sig )
        std::cout << "sorting: set significance = 1" << std::endl;
    else
        std::cout << "sorting: set significance = 0" << std::endl;
    
}


// It outputs by printing out the value right now.
void speck::SPECK2D::m_output_pixel_sign( const SPECKSet2D& pixel ) const
{
    auto x   = pixel.start_x;
    auto y   = pixel.start_y;
    auto idx = y * m_dim_x * x;
    if( m_sign_array[ idx ] )
        std::cout << "sorting: pixel sign = 1" << std::endl;
    else
        std::cout << "sorting: pixel sign = 0" << std::endl;

    m_coeff_buf[ idx ] -= m_threshold;
}

    
// Calculate the number of partitions able to be performed
long speck::SPECK2D::m_num_of_partitions() const
{
    long num_of_parts = 0;
    long dim_x = m_dim_x, dim_y = m_dim_y;
    while( dim_x > 1 || dim_y > 1 )
    {
        num_of_parts++;
        dim_x -= dim_x / 2;
        dim_y -= dim_y / 2;
    }
    return num_of_parts;
}



void speck::SPECK2D::m_calc_set_size( SPECKSet2D& set, long subband ) const
{
    assert( subband >= 0 && subband <= 3 );
    long part_level = set.part_level;
    long low_len_x, high_len_x;
    long low_len_y, high_len_y;
    speck::calc_approx_detail_len( m_dim_x, part_level, low_len_x, high_len_x );
    speck::calc_approx_detail_len( m_dim_y, part_level, low_len_y, high_len_y );
    
    // Note: the index of subbands (0, 1, 2, 3) follows what's used in QccPack,
    //       and is different from what is described in Figure 4 of the Pearlman paper.
    if( subband == 0 )      // top left
    {
        set.start_x  = 0;
        set.length_x = low_len_x;
        set.start_y  = 0;
        set.length_y = low_len_y;
    }
    else if( subband == 1 ) // bottom left
    {
        set.start_x  = 0;
        set.length_x = low_len_x;
        set.start_y  = low_len_y;
        set.length_y = high_len_y;
    }
    else if( subband == 2 ) // top right
    {
        set.start_x  = low_len_x;
        set.length_x = high_len_x;
        set.start_y  = 0;
        set.length_y = low_len_y;
    }
    else                    // bottom right
    {
        set.start_x  = low_len_x;
        set.length_x = high_len_x;
        set.start_y  = low_len_y;
        set.length_y = high_len_y;
    }
}


void speck::SPECK2D::m_clean_LIS()
{
    std::vector<SPECKSet2D> tmp;
    tmp.reserve( 8 );
    const auto is_garbage = []( const SPECKSet2D& s ){ return s.garbage; };

    for( long i = 0; i < m_LIS_garbage_cnt.size(); i++ )
    {
        if( m_LIS_garbage_cnt[i] > 0 )
        {
            std::remove_copy_if( m_LIS[i].begin(), m_LIS[i].end(), tmp.begin(), is_garbage );
            // Now tmp has all the non-garbage elements, let's do a swap!
            std::swap( m_LIS[i], tmp );
            // m_LIS[i] does not have garbage anymore!
            m_LIS_garbage_cnt[i] = 0;
        }
    }
}


//
// Class SPECKSet2D
//
bool speck::SPECKSet2D::is_pixel() const
{
    return ( length_x == 1 && length_y == 1 );
}

speck::SPECKSetType speck::SPECKSet2D::type() const
{
    return m_type;
}

// Constructor
speck::SPECKSet2D::SPECKSet2D( SPECKSetType t )
                 : m_type( t )
{ }
