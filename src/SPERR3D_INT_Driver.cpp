#include "SPERR3D_INT_Driver.h"

#include <algorithm>
#include <cassert>
#include <cfenv>
#include <cstring>

template <typename T>
auto sperr::SPERR3D_INT_Driver::copy_data(const T* p, size_t len, sperr::dims_type dims) -> RTNType
{
  static_assert(std::is_floating_point<T>::value, "!! Only floating point values are supported !!");

  if (len != dims[0] * dims[1] * dims[2])
    return RTNType::WrongDims;

  m_vals_d.resize(len);
  std::copy(p, p + len, m_vals_d.begin());

  m_dims = dims;

  return RTNType::Good;
}
template auto sperr::SPERR3D_INT_Driver::copy_data(const double*, size_t, dims_type) -> RTNType;
template auto sperr::SPERR3D_INT_Driver::copy_data(const float*, size_t, dims_type) -> RTNType;

auto sperr::SPERR3D_INT_Driver::take_data(sperr::vecd_type&& buf, sperr::dims_type dims) -> RTNType
{
  if (buf.size() != dims[0] * dims[1] * dims[2])
    return RTNType::WrongDims;

  m_vals_d = std::move(buf);
  m_dims = dims;

  return RTNType::Good;
}

void sperr::SPERR3D_INT_Driver::toggle_conditioning(sperr::Conditioner::settings_type b4)
{
  m_conditioning_settings = b4;
}

void sperr::SPERR3D_INT_Driver::set_pwe(double pwe)
{
  m_target_pwe = std::max(0.0, pwe);
}

auto sperr::SPERR3D_INT_Driver::release_encoded_bitstream() -> vec8_type&&
{
  return std::move(m_encoded_bitstream);
}

auto sperr::SPERR3D_INT_Driver::compress() -> RTNType
{
#pragma STDC FENV_ACCESS ON

  const auto total_vals = m_dims[0] * m_dims[1] * m_dims[2];
  if (m_vals_d.empty() || m_vals_d.size() != total_vals)
    return RTNType::Error;

  m_condi_stream.fill(0);
  m_encoded_bitstream.clear();

  // Believe it or not, there are constant fields passed in for compression!
  // Let's detect that case and skip the rest of the compression routine if it occurs.
  auto constant = m_conditioner.test_constant(m_vals_d);
  if (constant.first) {
    m_condi_stream = constant.second;
    m_encoded_bitstream.resize(constant.second.size());
    std::copy(constant.second.cbegin(), constant.second.cend(), m_encoded_bitstream.begin());
    return RTNType::Good;
  }

  // Step 1: data goes through the conditioner
  m_conditioner.toggle_all_settings(m_conditioning_settings);
  auto [rtn, condi_meta] = m_conditioner.condition(m_vals_d);
  if (rtn != RTNType::Good)
    return rtn;
  m_condi_stream = condi_meta;

  // Step 2: wavelet transform
  rtn = m_cdf.take_data(std::move(m_vals_d), m_dims);
  if (rtn != RTNType::Good)
    return rtn;
  m_cdf.dwt3d();
  m_vals_d = m_cdf.release_data();

  // Step 3: quantize floating-point coefficients to integers
  const auto q = m_target_pwe * 1.5;
  m_vals_ll.resize(total_vals);
  m_vals_ui.resize(total_vals);
  m_sign_array.resize(total_vals);
  std::fesetround(FE_TONEAREST);
  std::transform(m_vals_d.cbegin(), m_vals_d.cend(), m_vals_ll.begin(),
                 [q](auto d){ return std::llrint(d/q); });
  std::transform(m_vals_ll.cbegin(), m_vals_ll.cend(), m_vals_ui.begin(),
                 [](auto ll){ return static_cast<uint64_t>(std::abs(ll)); });
  std::transform(m_vals_ll.cbegin(), m_vals_ll.cend(), m_sign_array.begin(),
                 [](auto ll){ return ll >= 0ll; });

  // Step 3: Integer SPECK encoding
  m_encoder.set_dims(m_dims);
  m_encoder.use_coeffs(std::move(m_vals_ui), std::move(m_sign_array));
  m_encoder.encode();
  const auto& speck_stream = m_encoder.view_encoded_bitstream();
  if (speck_stream.empty())
    return RTNType::Error;

  // Step 4: Put together conditioner and SPECK bitstreams
  const size_t total_len = m_condi_stream.size() + speck_stream.size();
  m_encoded_bitstream.resize(total_len);
  std::copy(m_condi_stream.cbegin(), m_condi_stream.cend(), m_encoded_bitstream.begin());
  std::copy(speck_stream.cbegin(), speck_stream.cend(),
            m_encoded_bitstream.begin() + m_condi_stream.size());

  return RTNType::Good;
}

