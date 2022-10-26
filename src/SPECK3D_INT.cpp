#include "SPECK3D_INT.h"

auto sperr::SPECK3D_INT::take_coeffs(veci_t&& coeffs, dims_type dims) -> RTNType
{
  if (coeffs.size() != dims[0] * dims[1] * dims[2])
    return RTNType::WrongDims;

  m_coeff_buf = std::move(coeffs);
  m_dims = dims;
  return RTNType::Good;
}

void sperr::SPECK3D_INT::m_clean_LIS()
{
  for (auto& list : m_LIS) {
    auto it = std::remove_if(list.begin(), list.end(),
                             [](const auto& s) { return s.type == SetType::Garbage; });
    list.erase(it, list.end());
  }

  // Let's also clean up m_LIP.
  auto it = std::remove(m_LIP.begin(), m_LIP.end(), m_u64_garbage_val);
  m_LIP.erase(it, m_LIP.end());
}

