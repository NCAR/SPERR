#include "SPECK.h"
#include "speck_helper.h"
#include <cassert>
#include <cmath>


void speck::SPECK::assign_coeffs( double* ptr )
{
    m_coeff_buf.reset( ptr );
}

void speck::SPECK::assign_mean_dims( double m, long dx, long dy, long dz )
{
    m_data_mean = m;
    m_dim_x     = dx;
    m_dim_y     = dy;
    m_dim_z     = dz;
}
    
int speck::SPECK::speck2d()
{
    assert( m_coeff_buf != nullptr );                     // sanity check
    assert( m_dim_x > 0 && m_dim_y > 0 && m_dim_z > 0 );  // sanity check

    // Let's do some preparation: gather some values
    long num_of_vals          = m_dim_x * m_dim_y * m_dim_z;
    std::vector<bool>  sign_array;
    auto max_coeff            = m_make_positive( sign_array );
    long max_coefficient_bits = long(std::log2(max_coeff));
    long num_of_part_levels   = m_num_of_part_levels_2d();
    long num_of_xform_levels  = speck::calc_num_of_xform_levels( std::min( m_dim_x, m_dim_y) );

    // Still preparing: lists and sets
    std::vector< SPECKSet2D >               LSP;
    std::vector< std::vector<SPECKSet2D> >  LIS( num_of_part_levels );

    SPECKSet2D   root( SPECKSetType::TypeS );
    root.part_level = num_of_xform_levels - 1;
    m_calc_set_size_2d( root, 0 );
    LIS[ root.part_level ].push_back( root );

    SPECKSet2D   I( SPECKSetType::TypeI );
    I.part_level = num_of_xform_levels - 1;
    I.start_x    = root.length_x;
    I.start_y    = root.length_y;
    I.length_x   = m_dim_x;
    I.length_y   = m_dim_y;

    // Get ready for the quantization loop! 
    std::vector<bool>  significance_map( num_of_vals, false );  // initialized to be insignificant
    double threshold   = std::pow( 2.0, double(max_coefficient_bits) );



    return 0;
}
    
    
double speck::SPECK::m_make_positive( std::vector<bool>& sign_array ) const
{
    assert( m_coeff_buf.get() != nullptr );
    long num_of_vals = m_dim_x * m_dim_y * m_dim_z;
    assert( num_of_vals > 0 );
    sign_array.assign( num_of_vals, true ); // Initial to represent all being positive
    double max = std::fabs( m_coeff_buf[0] );
    for( long i = 0; i < num_of_vals; i++ )
    {
        if( m_coeff_buf[i] < 0.0 )
        {
            m_coeff_buf[i]  = -m_coeff_buf[i];
            sign_array[i] = false;
        }
        if( m_coeff_buf[i] > max )
            max = m_coeff_buf[i];
    }

    return max;
}

    
// Calculate the number of partition levels in a plane.
long speck::SPECK::m_num_of_part_levels_2d() const
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


void speck::SPECK::m_calc_set_size_2d( SPECKSet2D& set, long subband ) const
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
bool speck::SPECKSet2D::is_single_pixel() const
{
    return ( length_x == 1 && length_y == 1 );
}

// Constructor
speck::SPECKSet2D::SPECKSet2D( SPECKSetType t )
{
    type = t;
}
