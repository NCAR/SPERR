#include "SPECK3D_Err.h"

void speck::SPECK3D_Err::reserve(size_t num)
{
    m_LOS.reserve(num);
    m_q.reserve(num);
    m_err_hat.reserve(num);
    m_LSP.reserve(num);
    m_LSP_newly.reserve(num);
}

void speck::SPECK3D_Err::add_outlier(uint32_t x, uint32_t y, uint32_t z, float e)
{
    m_LOS.emplace_back(x, y, z, e);
}

void speck::SPECK3D_Err::add_outlier_list( std::vector<Outlier> list )
{
    m_LOS = std::move( list );
}

void speck::SPECK3D_Err::set_dims(size_t x, size_t y, size_t z)
{
    m_dim_x = x;
    m_dim_y = y;
    m_dim_z = z;
}

void speck::SPECK3D_Err::set_tolerance(float t)
{
    m_tolerance = t;
}
