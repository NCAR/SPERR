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
    else
        m_budget = budget + 8 - mod;
}

uint16_t speck::SPECK3D::get_max_coeff_bits()   const
{
    return m_max_coefficient_bits;
}
