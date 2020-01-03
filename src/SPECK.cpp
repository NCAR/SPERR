#include "SPECK.h"
#include <cassert>
#include <cmath>
    
int speck::SPECK::speck2d()
{
    auto max_coeff = m_make_positive();
    m_max_coefficient_bits = long(std::log2(max_coeff));

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
            m_coeff_buf[i]   = -m_coeff_buf[i];
            m_sign_array[i] = false;
        }
        if( m_coeff_buf[i] > max )
            max = m_coeff_buf[i];
    }

    return max;
}
