#include "SPECK3D_INT_Driver.h"

#include <algorithm>
#include <cassert>
#include <cfenv>
#include <cmath>
#include <cstring>

template <typename T>
void sperr::SPECK3D_INT_Driver::copy_data(const T* p, size_t len)
{
  static_assert(std::is_floating_point<T>::value, "!! Only floating point values are supported !!");

  m_vals_d.resize(len);
  std::copy(p, p + len, m_vals_d.begin());
}
template void sperr::SPECK3D_INT_Driver::copy_data(const double*, size_t);
template void sperr::SPECK3D_INT_Driver::copy_data(const float*, size_t);

void sperr::SPECK3D_INT_Driver::take_data(sperr::vecd_type&& buf)
{
  m_vals_d = std::move(buf);
}

#if 0
auto sperr::SPECK3D_INT_Driver::use_bitstream(const void* p, size_t len) -> RTNType
{
  // It'd be bad to have some buffers updated, and some others not.
  // So let's clean up everything at the very beginning of this routine
  m_condi_stream.fill(0);
  m_vals_d.clear();
  m_vals_ui.clear();
  m_vals_ll.clear();

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
#endif


auto sperr::SPECK3D_INT_Driver::release_encoded_bitstream() -> vec8_type&&
{
  return std::move(m_speck_bitstream);
}

void sperr::SPECK3D_INT_Driver::set_q(double q)
{
  m_q = std::max(0.0, q);
}

void sperr::SPECK3D_INT_Driver::set_dims(dims_type dims)
{
  m_dims = dims;
}

auto sperr::SPECK3D_INT_Driver::release_decoded_data() -> vecd_type&&
{
  return std::move(m_vals_d);
}

auto sperr::SPECK3D_INT_Driver::m_translate_f2i(const vecd_type& arrf) -> RTNType
{
#pragma STDC FENV_ACCESS ON

  const auto total_vals = arrf.size();
  const auto q1 = 1.0 / m_q;
  m_vals_ll.resize(total_vals);
  m_vals_ui.resize(total_vals);
  m_sign_array.resize(total_vals);
  std::fesetround(FE_TONEAREST);
  std::feclearexcept(FE_ALL_EXCEPT);
  std::transform(arrf.cbegin(), arrf.cend(), m_vals_ll.begin(),
                 [q1](auto d){ return std::llrint(d * q1); });
  if (std::fetestexcept(FE_INVALID))
    return RTNType::FE_Invalid;
  std::transform(m_vals_ll.cbegin(), m_vals_ll.cend(), m_vals_ui.begin(),
                 [](auto ll){ return static_cast<int_t>(std::abs(ll)); });
  std::transform(m_vals_ll.cbegin(), m_vals_ll.cend(), m_sign_array.begin(),
                 [](auto ll){ return ll >= 0ll; });

  return RTNType::Good;
}

void sperr::SPECK3D_INT_Driver::m_translate_i2f(const veci_t& arri, const vecb_type& signs)
{
  assert(signs.size() == arri.size());

  const auto tmpd = std::array<double, 2>{-1.0, 1.0};
  const auto q = m_q;
  m_vals_d.resize(signs.size());
  std::transform(arri.cbegin(), arri.cend(), signs.cbegin(), m_vals_d.begin(),
                 [tmpd, q](auto i, auto b){ return q * static_cast<double>(i) * tmpd[b]; });

}

auto sperr::SPECK3D_INT_Driver::encode() -> RTNType
{
  const auto total_vals = m_dims[0] * m_dims[1] * m_dims[2];
  if (m_vals_d.empty() || m_vals_d.size() != total_vals)
    return RTNType::Error;

  // Step 1: quantize floating-point values to integers.
  //         Results are saved in `m_vals_ui` and `m_sign_array`.
  auto rtn = m_translate_f2i(m_vals_d);
  if (rtn != RTNType::Good)
    return rtn;

  // Step 2: Integer SPECK encoding
  m_encoder.set_dims(m_dims);
  m_encoder.use_coeffs(std::move(m_vals_ui), std::move(m_sign_array));
  m_encoder.encode();
  m_speck_bitstream = m_encoder.view_encoded_bitstream();

  return RTNType::Good;
}

#if 0
auto sperr::SPECK3D_INT_Driver::decompress() -> RTNType
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
#endif
