#include "SPERR3D.h"
#include "SPECK3D_INT_ENC.h"
#include "SPECK3D_INT_DEC.h"

//
// Constructor
//
sperr::SPERR3D::SPERR3D()
{
  m_encoder = std::make_unique<SPECK3D_INT_ENC>();
  m_decoder = std::make_unique<SPECK3D_INT_DEC>();
}

void sperr::SPERR3D::m_wavelet_xform()
{
  m_cdf.dwt3d();
}

void sperr::SPERR3D::m_inverse_wavelet_xform()
{
  m_cdf.idwt3d();
}

auto sperr::SPERR3D::m_quantize() -> RTNType
{
  auto rtn = m_midtread_f2i();
  return rtn;
}

auto sperr::SPERR3D::m_inverse_quantize() -> RTNType
{
  m_midtread_i2f();
  return RTNType::Good;
}


