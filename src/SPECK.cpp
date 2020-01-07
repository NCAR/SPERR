#include "SPECK.h"
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

    // Let's do some preparation work
    long num_of_vals          = m_dim_x * m_dim_y * m_dim_z;
    std::vector<bool>  sign_array;
    auto max_coeff            = m_make_positive( sign_array );
    long max_coefficient_bits = long(std::log2(max_coeff));
    long num_of_part_levels   = m_num_of_part_levels_2d();

    // Still preparing: lists and sets
    std::vector< SPECKSet2D >               LSP;
    std::vector< std::vector<SPECKSet2D> >  LIS( num_of_part_levels );




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

long speck::SPECK::m_num_of_xform_levels_xy() const
{
    assert( m_dim_x > 0 && m_dim_y > 0 );
    const auto min_xy = std::min( m_dim_x, m_dim_y );
    float f     = std::log2(float(min_xy) / 9.0f);  // 9.0f for CDF 9/7 kernel
    long num_level_xy = f < 0.0f ? 0 : long(f) + 1;

    // Treat this special case which would occur with power of 2 lengths
    if( m_calc_approx_len( min_xy, num_level_xy - 1 ) == 8 )
        num_level_xy++;

    return num_level_xy;
}


long speck::SPECK::m_num_of_xform_levels_z() const
{
    assert( m_dim_z > 0 );
    float f      = std::log2( float(m_dim_z) / 9.0f ); // 9.0f for CDF 9/7 kernel
    long num_level_z = f < 0.0f ? 0 : long(f) + 1;

    // Treat this special case which would occur with power of 2 lengths
    if( m_calc_approx_len( m_dim_z, num_level_z - 1 ) == 8 )
        num_level_z++;

    return num_level_z;
}

long speck::SPECK::m_calc_approx_len( long orig_len, long lev ) const
{
    assert( lev >= 0 );
    long low_len = orig_len;
    for( long i = 0; i < lev; i++ )
        low_len = low_len % 2 == 0 ? low_len / 2 : (low_len + 1) / 2;
    
    return low_len;
}


//
// Class SPECKSet2D
//
bool speck::SPECKSet2D::is_single_pixel() const
{
    return ( num_rows == 1 && num_cols == 1 );
}

// Constructor
speck::SPECKSet2D::SPECKSet2D( SPECKSetType t, long o_r, long o_c, long n_r, long n_c )
{
    type       = t;
    origin_row = o_r;
    origin_col = o_c;
    num_rows   = n_r;
    num_cols   = n_c;
}
