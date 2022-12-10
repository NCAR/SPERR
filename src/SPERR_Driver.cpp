#include "SPERR_Driver.h"

#include <algorithm>
#include <cassert>
#include <cfenv>
#include <cmath>
#include <cstring>

template <typename T>
void sperr::SPERR_Driver::copy_data(const T* p, size_t len)
{
  static_assert(std::is_floating_point<T>::value, "!! Only floating point values are supported !!");

  m_vals_d.resize(len);
  std::copy(p, p + len, m_vals_d.begin());
}
template void sperr::SPERR_Driver::copy_data(const double*, size_t);
template void sperr::SPERR_Driver::copy_data(const float*, size_t);

void sperr::SPERR_Driver::take_data(sperr::vecd_type&& buf)
{
  m_vals_d = std::move(buf);
}

auto sperr::SPERR_Driver::use_bitstream(const void* p, size_t len) -> RTNType
{
  // So let's clean up everything at the very beginning of this routine.
  m_vals_d.clear();
  m_vals_ui.clear();
  m_vals_ll.clear();
  m_sign_array.clear();
  m_condi_bitstream.fill(0);
  m_encoded_bitstream.clear();

  const uint8_t* const ptr = static_cast<const uint8_t*>(p);

  // Step 1: extract conditioner stream
  const auto condi_size = m_condi_bitstream.size();
  if (condi_size > len)
    return RTNType::BitstreamWrongLen;
  std::copy(ptr, ptr + condi_size, m_condi_bitstream.begin());
  size_t pos = condi_size;

  // `m_condi_bitstream` might be indicating that the field is a constant field.
  // In that case, there will be no more speck or sperr streams.
  // Let's detect that case here and return early if it is true.
  // It will be up to the decompress() routine to restore the actual constant field.
  auto constant = m_conditioner.parse_constant(m_condi_bitstream);
  if (std::get<0>(constant)) {
    if (condi_size == len)
      return RTNType::Good;
    else
      return RTNType::BitstreamWrongLen;
  }

  // Step 2: extract SPECK stream from it
  const uint8_t* const speck_p = ptr + pos;
  const auto speck_full_len = m_decoder.get_speck_full_len(speck_p);
  if (speck_full_len != len - pos)
      return RTNType::BitstreamWrongLen;
  m_speck_bitstream.resize(speck_full_len);
  std::copy(speck_p, speck_p + speck_full_len, m_speck_bitstream.begin());

  return RTNType::Good;
}

void sperr::SPERR_Driver::toggle_conditioning(sperr::Conditioner::settings_type settings)
{
  m_conditioning_settings = settings;
}

auto sperr::SPERR_Driver::get_encoded_bitstream() -> vec8_type
{
  const auto total_len = m_condi_bitstream.size() + m_speck_bitstream.size();
  auto tmp = vec8_type(total_len);
  std::copy(m_condi_bitstream.cbegin(), m_condi_bitstream.cend(), tmp.begin());
  std::copy(m_speck_bitstream.cbegin(), m_speck_bitstream.cend(), 
            tmp.begin() + m_condi_bitstream.size());
  return tmp;
}

auto sperr::SPERR_Driver::release_decoded_data() -> vecd_type&&
{
  return std::move(m_vals_d);
}

void sperr::SPERR_Driver::set_q(double q)
{
  m_q = std::max(0.0, q);
}

void sperr::SPERR_Driver::set_dims(dims_type dims)
{
  m_dims = dims;
}

auto sperr::SPERR_Driver::num_coded_vals() const -> size_t
{
  return std::count_if(m_vals_ll.cbegin(), m_vals_ll.cend(), [](auto v){ return v != 0l; });
}

auto sperr::SPERR_Driver::m_midtread_f2i(const vecd_type& arrf) -> RTNType
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

void sperr::SPERR_Driver::m_midtread_i2f(const veci_t& arri, const vecb_type& signs)
{
  assert(signs.size() == arri.size());

  const auto tmpd = std::array<double, 2>{-1.0, 1.0};
  const auto q = m_q;
  m_vals_d.resize(signs.size());
  std::transform(arri.cbegin(), arri.cend(), signs.cbegin(), m_vals_d.begin(),
                 [tmpd, q](auto i, auto b){ return q * static_cast<double>(i) * tmpd[b]; });

}

auto sperr::SPERR_Driver::m_proc_1() -> RTNType
{
  return RTNType::Good;
}

auto sperr::SPERR_Driver::m_proc_2() -> RTNType
{
  return RTNType::Good;
}

auto sperr::SPERR_Driver::encode() -> RTNType
{
  const auto total_vals = m_dims[0] * m_dims[1] * m_dims[2];
  if (m_vals_d.empty() || m_vals_d.size() != total_vals)
    return RTNType::Error;

  m_condi_bitstream.fill(0);
  m_speck_bitstream.clear();

  // Step 1: quantize floating-point values to integers.
  //         Results are saved in `m_vals_ui` and `m_sign_array`.
  auto rtn = m_translate_f2i(m_vals_d);
  if (rtn != RTNType::Good)
    return rtn;

  // Step 2: Integer SPECK encoding
  m_encoder->set_dims(m_dims);
  m_encoder->use_coeffs(std::move(m_vals_ui), std::move(m_sign_array));
  m_encoder->encode();
  m_speck_bitstream = m_encoder->view_encoded_bitstream();

  return RTNType::Good;
}

auto sperr::SPERR_Driver::decode() -> RTNType
{
  m_vals_d.clear();
  m_vals_ui.clear();
  m_vals_ll.clear();
  m_sign_array.clear();
  const auto total_vals = m_dims[0] * m_dims[1] * m_dims[2];

  // Step 1: Integer SPECK decode.
  assert(!m_speck_bitstream.empty());
  m_decoder->set_dims(m_dims);
  m_decoder->use_bitstream(m_speck_bitstream);
  m_decoder->decode();

  // Step 2: Convert integers to doubles.
  const auto& vals_ui = m_decoder->view_coeffs();
  const auto& signs = m_decoder->view_signs();
  assert(vals_ui.size() == total_vals);
  assert(signs.size() == total_vals);
  m_translate_i2f(vals_ui, signs);

  return RTNType::Good;
}

