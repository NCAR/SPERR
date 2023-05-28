#include "Outlier_Coder.h"

#include <algorithm>
#include <cassert>
#include <cfloat>  // FLT_ROUNDS
#include <cfenv>
#include <cmath>

sperr::Outlier::Outlier(size_t p, double e) : pos(p), err(e) {}

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
      m_encoder.emplace<0>(); 
    if (m_decoder.index() != 0)
      m_decoder.emplace<0>(); 
  }
  else if (maxint <= std::numeric_limits<uint16_t>::max()) {
    m_vals_ui.emplace<1>(); 
    m_encoder.emplace<1>();
    m_decoder.emplace<1>();
  }
  else if (maxint <= std::numeric_limits<uint32_t>::max()) {
    m_vals_ui.emplace<2>(); 
    m_encoder.emplace<2>();
    m_decoder.emplace<2>();
  }
  else {
    assert(maxint <= std::numeric_limits<uint64_t>::max());
    m_vals_ui.emplace<3>(); 
    m_encoder.emplace<3>();
    m_decoder.emplace<3>();
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

void sperr::Outlier_Coder::m_inverse_quantize()
{
  m_LOS.clear();
  const auto tmp = std::array<double, 2>{-1.0, 0.0};

  if (m_vals_ui.index() == 0) {
    const auto& ui = std::get<0>(m_vals_ui);
    for (size_t i = 0; i < ui.size(); i++)
      if (ui[i] != 0) {
        auto sign = tmp[m_sign_array[i]];
        m_LOS.emplace_back(i, sign * m_tol * ui[i]);
      }
  }
  else if (m_vals_ui.index() == 1) {
    const auto& ui = std::get<1>(m_vals_ui);
    for (size_t i = 0; i < ui.size(); i++)
      if (ui[i] != 0) {
        auto sign = tmp[m_sign_array[i]];
        m_LOS.emplace_back(i, sign * m_tol * ui[i]);
      }
  }
  else if (m_vals_ui.index() == 2) {
    const auto& ui = std::get<2>(m_vals_ui);
    for (size_t i = 0; i < ui.size(); i++)
      if (ui[i] != 0) {
        auto sign = tmp[m_sign_array[i]];
        m_LOS.emplace_back(i, sign * m_tol * ui[i]);
      }
  }
  else {
    const auto& ui = std::get<3>(m_vals_ui);
    for (size_t i = 0; i < ui.size(); i++)
      if (ui[i] != 0) {
        auto sign = tmp[m_sign_array[i]];
        m_LOS.emplace_back(i, sign * m_tol * ui[i]);
      }
  }
}
