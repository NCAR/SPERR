#include "Outlier_Coder.h"

#include <algorithm>
#include <cassert>
#include <cfenv>
#include <cfloat>  // FLT_ROUNDS
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
  const auto num_bitplanes = speck_int_get_num_bitplanes(speck_p);
  if (num_bitplanes <= 8)
    m_instantiate_uvec_coders(UINTType::UINT8);
  else if (num_bitplanes <= 16)
    m_instantiate_uvec_coders(UINTType::UINT16);
  else if (num_bitplanes <= 32)
    m_instantiate_uvec_coders(UINTType::UINT32);
  else
    m_instantiate_uvec_coders(UINTType::UINT64);

  // TODO
  return RTNType::Good;
}

auto sperr::Outlier_Coder::view_outlier_list() const -> const std::vector<Outlier>&
{
  return m_LOS;
}

void sperr::Outlier_Coder::append_encoded_bitstream(vec8_type& buf) const
{
  // Just append the bitstream produced by `m_encoder` is fine.
  std::visit([&buf](auto&& enc) { enc.append_encoded_bitstream(buf); }, m_encoder);
}

auto sperr::Outlier_Coder::encode() -> RTNType
{
  // TODO

  // Find the biggest magnitude of outlier errors, and then instantiate data structures.
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

  if (maxint <=  std::numeric_limits<uint8_t>::max())
    m_instantiate_uvec_coders(UINTType::UINT8);
  else if (maxint <=  std::numeric_limits<uint16_t>::max())
    m_instantiate_uvec_coders(UINTType::UINT16);
  else if (maxint <=  std::numeric_limits<uint32_t>::max())
    m_instantiate_uvec_coders(UINTType::UINT32);
  else
    m_instantiate_uvec_coders(UINTType::UINT64);


  return RTNType::Good;
}

auto sperr::Outlier_Coder::decode() -> RTNType
{
  // TODO
  return RTNType::Good;
}

void sperr::Outlier_Coder::m_instantiate_uvec_coders(UINTType type);
{
  if (type == UINTType::UINT8) {
    if (m_vals_ui.index() != 0)
      m_vals_ui = std::vector<uint8_t>();
    if (m_encoder.index() != 0)
      m_encoder.emplace<0>();
    if (m_decoder.index() != 0)
      m_decoder.emplace<0>();
  }
  else if (type == UINTType::UINT16) {
    m_vals_ui.emplace<1>();
    m_encoder.emplace<1>();
    m_decoder.emplace<1>();
  }
  else if (type == UINTType::UINT32) {
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

  // First, bring all non-zero integer correctors to `m_LOS`.
  if (m_vals_ui.index() == 0) {
    const auto& ui = std::get<0>(m_vals_ui);
    for (size_t i = 0; i < ui.size(); i++)
      if (ui[i] != 0)
        m_LOS.emplace_back(i, double(ui[i]));
  }
  else {
    for (size_t i = 0; i < m_total_len; i++) {
      auto ui = std::visit([i](auto&& vec) { return uint64_t{vec[i]}; }, m_vals_ui);
      if (ui != 0)
        m_LOS.emplace_back(i, double(ui));
    }
  }

  // Second, restore the floating-point correctors.
  const auto tmp = std::array<double, 2>{-1.0, 0.0};
  std::transform(m_LOS.begin(), m_LOS.end(), m_sign_array.begin(), m_LOS.begin(),
                 [q = m_tol, tmp](auto los, auto s) {
                   los.err *= q * tmp[s];
                   return los;
                 });
}
