#include "SPECK2D.h"
#include <cassert>
#include <cmath>
#include <iostream>


void speck::SPECK2D::assign_coeffs( double* ptr )
{
    m_coeff_buf.reset( ptr );
}

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
    long num_of_part_levels   = m_num_of_part_levels();
    long num_of_xform_levels  = speck::calc_num_of_xform_levels( std::min( m_dim_x, m_dim_y) );

    // Still preparing: lists and sets
    m_LIS.clear();
    m_LIS.resize( num_of_part_levels );
    SPECKSet2D root( SPECKSetType::TypeS );
    root.part_level = num_of_xform_levels - 1;
    m_calc_set_size( root, 0 );      // Populate other data fields of root.
    m_LIS[ root.part_level ].push_back( root );

    m_I.part_level = num_of_xform_levels - 1;
    m_I.start_x    = root.length_x;
    m_I.start_y    = root.length_y;
    m_I.length_x   = m_dim_x;
    m_I.length_y   = m_dim_y;

    // Get ready for the quantization loop!  L1598 of speck.c
    m_significance_map.resize( num_of_vals );
    double threshold  = std::pow( 2.0, double(max_coefficient_bits) );



    return 0;
}
    
    
//
// Private methods
//
void speck::SPECK2D::m_sorting_pass( const double threshold )
{


}


void speck::SPECK2D::m_process_S( SPECKSet2D& set )
{
    m_output_set_significance( set );   // It also keeps the significance value in the set
    if( set.sig != Significance::Insignificant )
    {
        if( set.is_pixel() )
        {
            set.sig = Significance::Newly_Significant;
        }
    }
}


// It outputs by printing out the value right now.
void speck::SPECK2D::m_output_set_significance( SPECKSet2D& set ) const
{
    // Sanity check
    assert( set.type == SPECKSetType::TypeS );
    assert( m_significance_map.size() == m_dim_x * m_dim_y );

    set.sig = Significance::Insignificant;
    for( long y = set.start_y; y < (set.start_y + set.length_y); y++ )
    {
        for( long x = set.start_x; x < (set.start_x + set.length_x); x++ )
        {
            long idx = y * m_dim_x + x;
            if( m_significance_map[ idx ] )
            {
                set.sig = Significance::Significant;
                break;
            }
        }
        if( set.sig == Significance::Significant )
            break;
    }

    if( set.sig == Significance::Significant )
        std::cout << "sorting: set significance = 1" << std::endl;
    else
        std::cout << "sorting: set significance = 0" << std::endl;
    
}

    
// Calculate the number of partition levels in a plane.
long speck::SPECK2D::m_num_of_part_levels() const
{
    long num_of_lev = 1;    // Even no partition is performed, there's already one level.
    long dim_x = m_dim_x, dim_y = m_dim_y;
    while( dim_x > 1 || dim_y > 1 )
    {
        num_of_lev++;
        dim_x -= dim_x / 2;
        dim_y -= dim_y / 2;
    }
    return num_of_lev;
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


//
// Class SPECKSet2D
//
bool speck::SPECKSet2D::is_pixel() const
{
    return ( length_x == 1 && length_y == 1 );
}

// Constructor
speck::SPECKSet2D::SPECKSet2D( SPECKSetType t )
                 : type( t )
{ }
