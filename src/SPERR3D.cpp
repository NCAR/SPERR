#include "SPERR3D_INT_Driver.h"

#include <algorithm>
#include <cassert>
#include <cfenv>
#include <cstring>

template <typename T>
void sperr::SPERR3D_INT_Driver::copy_data(const T* p, size_t len)
{
  static_assert(std::is_floating_point<T>::value, "!! Only floating point values are supported !!");

  m_vals_d.resize(len);
  std::copy(p, p + len, m_vals_d.begin());
}
template void sperr::SPERR3D_INT_Driver::copy_data(const double*, size_t);
template void sperr::SPERR3D_INT_Driver::copy_data(const float*, size_t);

void sperr::SPERR3D_INT_Driver::take_data(sperr::vecd_type&& buf)
{
  m_vals_d = std::move(buf);
}

auto sperr::SPERR3D_INT_Driver::use_bitstream(const void* p, size_t len) -> RTNType
{
  // It'd be bad to have some buffers updated, and some others not.
  // So let's clean up everything at the very beginning of this routine
  m_condi_stream.fill(0);
  m_vals_d.clear();
  m_vals_ui.clear();
  m_vals_ll.clear();
  m_sign_array.clear();

  const uint8_t* const ptr = static_cast<const uint8_t*>(p);
  const size_t ptr_len = len;

  // Step 1: extract conditioner stream from it
  const auto condi_size = m_condi_stream.size();
  if (condi_size > ptr_len)
    return RTNType::BitstreamWrongLen;
  std::copy(ptr, ptr + condi_size, m_condi_stream.begin());
  size_t pos = condi_size;

  // `m_condi_stream` might be indicating that the field is a constant field.
  // In that case, there will be no more speck or sperr streams.
  // Let's detect that case here and return early if it is true.
  // It will be up to the decompress() routine to restore the actual constant field.
  auto constant = m_conditioner.parse_constant(m_condi_stream);
  if (std::get<0>(constant)) {
    if (condi_size == ptr_len)
      return RTNType::Good;
    else
      return RTNType::BitstreamWrongLen;
  }

  // Step 2: extract SPECK stream from it
  const uint8_t* const speck_p = ptr + pos;
  const auto speck_full_len = m_decoder.get_speck_full_len(speck_p);
  m_speck_bitstream.resize(speck_full_len);
  std::copy(speck_p, speck_p + speck_full_len, m_speck_bitstream.begin());

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

void sperr::SPERR3D_INT_Driver::set_dims(dims_type dims)
{
  m_dims = dims;
}

auto sperr::SPERR3D_INT_Driver::release_encoded_bitstream() -> vec8_type&&
{
  return std::move(m_encoded_bitstream);
}

auto sperr::SPERR3D_INT_Driver::release_decoded_data() -> vecd_type&&
{
  return std::move(m_vals_d);
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
  const auto q1 = 1.0 / (1.5 * m_target_pwe);
  m_vals_ll.resize(total_vals);
  m_vals_ui.resize(total_vals);
  m_sign_array.resize(total_vals);
  std::fesetround(FE_TONEAREST);
  std::feclearexcept(FE_ALL_EXCEPT);
  std::transform(m_vals_d.cbegin(), m_vals_d.cend(), m_vals_ll.begin(),
                 [q1](auto d){ return std::llrint(d * q1); });
  if (std::fetestexcept(FE_INVALID))
    return RTNType::FE_Invalid;
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

  // Step 4: Put together the conditioner and SPECK bitstreams.
  const size_t total_len = m_condi_stream.size() + sizeof(double) + speck_stream.size();
  m_encoded_bitstream.resize(total_len);
  std::copy(m_condi_stream.cbegin(), m_condi_stream.cend(), m_encoded_bitstream.begin());
  size_t pos = m_condi_stream.size();
  std::copy(speck_stream.cbegin(), speck_stream.cend(), m_encoded_bitstream.begin() + pos);

  return RTNType::Good;
}

auto sperr::SPERR3D_INT_Driver::decompress() -> RTNType
{
  m_vals_d.clear();
  m_vals_ui.clear();
  m_vals_ll.clear();
  m_sign_array.clear();

  const auto total_vals = m_dims[0] * m_dims[1] * m_dims[2];

  // `m_condi_stream` might be indicating a constant field, so let's see if that's
  // the case, and if it is, we don't need to go through dwt and speck stuff anymore.
  auto constant = m_conditioner.parse_constant(m_condi_stream);
  if (std::get<0>(constant)) {
    auto val = std::get<1>(constant);
    auto nval = std::get<2>(constant);
    m_vals_d.assign(nval, val);
    return RTNType::Good;
  }

  // Step 1: Integer SPECK decode.
  if (m_speck_bitstream.empty())
    return RTNType::Error;

  m_decoder.set_dims(m_dims);
  m_decoder.use_bitstream(m_speck_bitstream);
  m_decoder.decode();

  // Step 2: Convert integers to doubles.
  const auto& vals_ui = m_decoder.view_coeffs();
  const auto& signs = m_decoder.view_signs();
  assert(vals_ui.size() == total_vals);
  assert(signs.size() == total_vals);

  const auto tmpd = std::array<double, 2>{-1.0, 1.0};
  const auto q = m_target_pwe * 1.5;
  m_vals_d.resize(total_vals);
  std::transform(vals_ui.cbegin(), vals_ui.cend(), signs.cbegin(), m_vals_d.begin(),
                 [tmpd, q](auto ui, auto b){ return tmpd[b] * static_cast<double>(ui) * q; });

  // Step 3: Inverse Wavelet Transform
  m_cdf.take_data(std::move(m_vals_d), m_dims);
  m_cdf.idwt3d();

  // Step 4: Inverse Conditioning
  m_vals_d = m_cdf.release_data();
  m_conditioner.inverse_condition(m_vals_d, m_condi_stream);

  return RTNType::Good;
}

