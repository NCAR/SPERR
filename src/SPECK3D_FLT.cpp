#include "SPECK3D_FLT.h"
#include "SPECK3D_INT_DEC.h"
#include "SPECK3D_INT_ENC.h"

void sperr::SPECK3D_FLT::m_instantiate_encoder()
{
  switch (m_uint_flag) {
    case UINTType::UINT64:
      if (m_encoder.index() != 0 || std::get<0>(m_encoder) == nullptr)
        m_encoder = std::make_unique<SPECK3D_INT_ENC<uint64_t>>();
      break;
    case UINTType::UINT32:
      if (m_encoder.index() != 1 || std::get<1>(m_encoder) == nullptr)
        m_encoder = std::make_unique<SPECK3D_INT_ENC<uint32_t>>();
      break;
    case UINTType::UINT16:
      if (m_encoder.index() != 2 || std::get<2>(m_encoder) == nullptr)
        m_encoder = std::make_unique<SPECK3D_INT_ENC<uint16_t>>();
      break;
    default:
      if (m_encoder.index() != 3 || std::get<3>(m_encoder) == nullptr)
        m_encoder = std::make_unique<SPECK3D_INT_ENC<uint8_t>>();
  }
}

void sperr::SPECK3D_FLT::m_instantiate_decoder()
{
  switch (m_uint_flag) {
    case UINTType::UINT64:
      if (m_decoder.index() != 0 || std::get<0>(m_decoder) == nullptr)
        m_decoder = std::make_unique<SPECK3D_INT_DEC<uint64_t>>();
      break;
    case UINTType::UINT32:
      if (m_decoder.index() != 1 || std::get<1>(m_decoder) == nullptr)
        m_decoder = std::make_unique<SPECK3D_INT_DEC<uint32_t>>();
      break;
    case UINTType::UINT16:
      if (m_decoder.index() != 2 || std::get<2>(m_decoder) == nullptr)
        m_decoder = std::make_unique<SPECK3D_INT_DEC<uint16_t>>();
      break;
    default:
      if (m_decoder.index() != 3 || std::get<3>(m_decoder) == nullptr)
        m_decoder = std::make_unique<SPECK3D_INT_DEC<uint8_t>>();
  }
}

void sperr::SPECK3D_FLT::m_wavelet_xform()
{
  m_cdf.dwt3d();
}

void sperr::SPECK3D_FLT::m_inverse_wavelet_xform()
{
  m_cdf.idwt3d();
}

auto sperr::SPECK3D_FLT::m_quantize() -> RTNType
{
  auto rtn = m_midtread_f2i();
  return rtn;
}

auto sperr::SPECK3D_FLT::m_inverse_quantize() -> RTNType
{
  m_midtread_i2f();
  return RTNType::Good;
}

auto sperr::SPECK3D_FLT::m_estimate_mse(double q) const -> double
{
  return m_estimate_mse_midtread(q);
}
