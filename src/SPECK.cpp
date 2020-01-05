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
    auto max_coeff            = m_make_positive();
    long max_coefficient_bits = long(std::log2(max_coeff));
    double threshold          = std::pow( 2.0, double(max_coefficient_bits) );
    m_significance_map.assign( num_of_vals, false );    // initialized to be insignificant

    return 0;
}
    
    
double speck::SPECK::m_make_positive()
{
    assert( m_coeff_buf.get() != nullptr );
    long num_of_vals = m_dim_x * m_dim_y * m_dim_z;
    assert( num_of_vals > 0 );
    m_sign_array.assign( num_of_vals, true ); // Initial to represent all being positive
    double max = std::fabs( m_coeff_buf[0] );
    for( long i = 0; i < num_of_vals; i++ )
    {
        if( m_coeff_buf[i] < 0.0 )
        {
            m_coeff_buf[i]  = -m_coeff_buf[i];
            m_sign_array[i] = false;
        }
        if( m_coeff_buf[i] > max )
            max = m_coeff_buf[i];
    }

    return max;
}


bool speck::SPECKSet2D::is_single_pixel() const
{
    return ( num_rows == 1 && num_cols == 1 );
}
