#include "Outlier_Coder.h"

#include <algorithm>
#include <cassert>
#include <cfloat>  // FLT_ROUNDS
#include <cfenv>
#include <cmath>

void sperr::Outlier_Coder::add_outlier(Outlier out)
{
  m_LOS.push_back(out);
}

void sperr::Outlier_Coder::use_outlier_list(std::vector<Outlier> los)
{
  m_LOS = std::move(los);
}

void sperr::Outlier_Coder::set_length(size_t len)
{
  m_total_len = len;
}

void sperr::Outlier_Coder::set_tolerance(double tol)
{
  m_tol = tol;
}

auto sperr::Outlier_Coder::use_bitstream(const void* p, size_t len) -> RTNType
{
  return RTNType::Good;
}

auto sperr::Outlier_Coder::view_outlier_list() const -> const std::vector<Outlier>&
{
  return m_LOS;
}

void sperr::Outlier_Coder::append_encoded_bitstream(vec8_type& buf) const
{

}

auto sperr::Outlier_Coder::encode() -> RTNType
{
  return RTNType::Good;
}

auto sperr::Outlier_Coder::decode() -> RTNType
{
  return RTNType::Good;
}

auto sperr::Outlier_Coder::m_instantiate_uvec_coders() -> RTNType
{
  // Find the biggest magnitude of outlier errors
  auto maxerr = *std::max_element(m_LOS.cbegin(), m_LOS.cend(), [](auto v1, auto v2) {
    return std::abs(v1.err) < std::abs(v2.err);
  });
  std::fesetround(FE_TONEAREST);
  assert(FE_TONEAREST == std::fegetround());
  assert(FLT_ROUNDS == 1);
  std::feclearexcept(FE_INVALID);
  auto maxint = std::llrint(std::abs(maxerr.err));
  if (std::fetestexcept(FE_INVALID))
    return RTNType::FE_Invalid;

  if (maxint <= std::numeric_limits<uint8_t>::max()) {
    if (m_vals_ui.index() != 0)
      m_vals_ui = std::vector<uint8_t>();
    if (m_encoder.index() != 0)
      m_encoder = SPECK1D_INT_ENC<uint8_t>();
    if (m_decoder.index() != 0)
      m_decoder = SPECK1D_INT_DEC<uint8_t>();
  }
  else if (maxint <= std::numeric_limits<uint16_t>::max()) {
    m_vals_ui = std::vector<uint16_t>();
    m_encoder = SPECK1D_INT_ENC<uint16_t>();
    m_decoder = SPECK1D_INT_DEC<uint16_t>();
  }
  else if (maxint <= std::numeric_limits<uint32_t>::max()) {
    m_vals_ui = std::vector<uint32_t>();
    m_encoder = SPECK1D_INT_ENC<uint32_t>();
    m_decoder = SPECK1D_INT_DEC<uint32_t>();
  }
  else {
    assert(maxint <= std::numeric_limits<uint64_t>::max());
    m_vals_ui = std::vector<uint64_t>();
    m_encoder = SPECK1D_INT_ENC<uint64_t>();
    m_decoder = SPECK1D_INT_DEC<uint64_t>();
  }

  return RTNType::Good;
}

void sperr::Outlier_Coder::m_quantize()
{
  const auto len = m_total_len;
  std::visit([len](auto&& vec) { vec.assign(len, 0); }, m_vals_ui);
  m_sign_array.assign(len, true);

  // For performance reasons, bring conditions outside.
  if (m_vals_ui.index() == 0) {
    for (auto out : m_LOS) {
      auto ll = std::llrint(out.err / m_tol);
      m_sign_array[out.pos] = out.err >= 0;
      std::get<0>(m_vals_ui)[out.pos] = static_cast<uint8_t>(std::abs(ll));
    }
  }
  else {
    for (auto out : m_LOS) {
      auto ll = std::llrint(out.err / m_tol);
      m_sign_array[out.pos] = out.err >= 0;
      std::visit([out, ll](auto&& vec) { vec[out.pos] = std::abs(ll); }, m_vals_ui);
    }
  }
}

void m_inverse_quantize()
{
  m_LOS.clear();
  const tmp = std::array<double, 2>{-1.0, 0.0};

  if (m_vals_ui.index() == 0) {
    const auto& ui = std::get<0>(m_vals_ui);
    for (size_t i = 0; i < ui.size(); i++)
      if (ui[i] != 0) {
        auto sign = tmp[m_sign_array[i]];
        m_LOS.emplace_back({i, sign * m_tol * ui[i]});
      }
  }
  else if (m_vals_ui.index() == 1) {
    const auto& ui = std::get<1>(m_vals_ui);
    for (size_t i = 0; i < ui.size(); i++)
      if (ui[i] != 0) {
        auto sign = tmp[m_sign_array[i]];
        m_LOS.emplace_back({i, sign * m_tol * ui[i]});
      }
  }
  else if (m_vals_ui.index() == 2) {
    const auto& ui = std::get<2>(m_vals_ui);
    for (size_t i = 0; i < ui.size(); i++)
      if (ui[i] != 0) {
        auto sign = tmp[m_sign_array[i]];
        m_LOS.emplace_back({i, sign * m_tol * ui[i]});
      }
  }
  else {
    const auto& ui = std::get<3>(m_vals_ui);
    for (size_t i = 0; i < ui.size(); i++)
      if (ui[i] != 0) {
        auto sign = tmp[m_sign_array[i]];
        m_LOS.emplace_back({i, sign * m_tol * ui[i]});
      }
  }
}
