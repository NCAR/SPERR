#include "SPERR3D.h"
#include "SPECK3D_INT_DEC.h"
#include "SPECK3D_INT_ENC.h"

//
// Constructor
//
//sperr::SPERR3D::SPERR3D()
//{
//  m_encoder = std::make_unique<SPECK3D_INT_ENC>();
//  m_decoder = std::make_unique<SPECK3D_INT_DEC>();
//}

void sperr::SPERR3D::m_instantiate_coders()
{
  switch (m_uint_flag) {
    case UINTType::UINT64:
      if (m_encoder.index() != 0 || m_encoder.get_if<0> == nullptr)
        m_encoder = std::make_unique<SPECK3D_INT_ENC<uint64_t>>();
      if (m_decoder.index() != 0 || m_decoder.get_if<0> == nullptr)
        m_decoder = std::make_unique<SPECK3D_INT_DEC<uint64_t>>();
      break;
    case UINTType::UINT32:
      if (m_encoder.index() != 1 || m_encoder.get_if<1> == nullptr)
        m_encoder = std::make_unique<SPECK3D_INT_ENC<uint32_t>>();
      if (m_decoder.index() != 1 || m_decoder.get_if<1> == nullptr)
        m_decoder = std::make_unique<SPECK3D_INT_DEC<uint32_t>>();
      break;
    case UINTType::UINT16:
      if (m_encoder.index() != 2 || m_encoder.get_if<2> == nullptr)
        m_encoder = std::make_unique<SPECK3D_INT_ENC<uint16_t>>();
      if (m_decoder.index() != 2 || m_decoder.get_if<2> == nullptr)
        m_decoder = std::make_unique<SPECK3D_INT_DEC<uint16_t>>();
      break;
    default:
      if (m_encoder.index() != 3 || m_encoder.get_if<3> == nullptr)
        m_encoder = std::make_unique<SPECK3D_INT_ENC<uint8_t>>();
      if (m_decoder.index() != 3 || m_decoder.get_if<3> == nullptr)
        m_decoder = std::make_unique<SPECK3D_INT_DEC<uint8_t>>();
  }
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
