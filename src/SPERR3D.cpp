#include "SPERR3D.h"

#include <algorithm>
#include <cassert>
#include <cfenv>
#include <cstring>

auto sperr::SPERR3D::use_bitstream(const void* p, size_t len) -> RTNType
{
  // It'd be bad to have some buffers updated, and some others not.
  // So let's clean up everything at the very beginning of this routine
  m_vals_d.clear();
  m_vals_ui.clear();
  m_vals_ll.clear();
  m_sign_array.clear();
  m_condi_bitstream.fill(0);
  m_encoded_bitstream.clear();

  const uint8_t* const ptr = static_cast<const uint8_t*>(p);
  const size_t ptr_len = len;

  // Step 1: extract conditioner stream from it
  const auto condi_size = m_condi_bitstream.size();
  if (condi_size > ptr_len)
    return RTNType::BitstreamWrongLen;
  std::copy(ptr, ptr + condi_size, m_condi_bitstream.begin());
  size_t pos = condi_size;

  // `m_condi_bitstream` might be indicating that the field is a constant field.
  // In that case, there will be no more speck or sperr streams.
  // Let's detect that case here and return early if it is true.
  // It will be up to the decompress() routine to restore the actual constant field.
  auto constant = m_conditioner.parse_constant(m_condi_bitstream);
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

void sperr::SPERR3D::toggle_conditioning(sperr::Conditioner::settings_type b4)
{
  m_conditioning_settings = b4;
}

void sperr::SPERR3D::set_target_psnr(double psnr)
{
  m_target_psnr = psnr;
}

auto sperr::SPERR3D::release_encoded_bitstream() -> vec8_type&&
{
  return std::move(m_encoded_bitstream);
}

auto sperr::SPERR3D::encode() -> RTNType
{
  const auto total_vals = m_dims[0] * m_dims[1] * m_dims[2];
  if (m_vals_d.empty() || m_vals_d.size() != total_vals)
    return RTNType::Error;

  m_condi_bitstream.fill(0);
  m_encoded_bitstream.clear();

  // Believe it or not, there are constant fields passed in for compression!
  // Let's detect that case and skip the rest of the compression routine if it occurs.
  auto constant = m_conditioner.test_constant(m_vals_d);
  if (constant.first) {
    m_condi_bitstream = constant.second;
    m_encoded_bitstream.resize(constant.second.size());
    std::copy(constant.second.cbegin(), constant.second.cend(), m_encoded_bitstream.begin());
    return RTNType::Good;
  }

  // Step 1: data goes through the conditioner
  m_conditioner.toggle_all_settings(m_conditioning_settings);
  auto [rtn, condi_meta] = m_conditioner.condition(m_vals_d);
  if (rtn != RTNType::Good)
    return rtn;
  m_condi_bitstream = condi_meta;

  // Step 2: wavelet transform
  rtn = m_cdf.take_data(std::move(m_vals_d), m_dims);
  if (rtn != RTNType::Good)
    return rtn;
  m_cdf.dwt3d();
  m_vals_d = m_cdf.release_data();

  // Step 3: quantize floating-point coefficients to integers
  rtn = m_translate_f2i(m_vals_d);
  if (rtn != RTNType::Good)
    return rtn;

  // Step 4: Integer SPECK encoding
  m_encoder.set_dims(m_dims);
  m_encoder.use_coeffs(std::move(m_vals_ui), std::move(m_sign_array));
  m_encoder.encode();
  const auto& speck_stream = m_encoder.view_encoded_bitstream();
  if (speck_stream.empty())
    return RTNType::Error;

  // Step 4: Put together the conditioner and SPECK bitstreams.
  const size_t total_len = m_condi_bitstream.size() + speck_stream.size();
  m_encoded_bitstream.resize(total_len);
  std::copy(m_condi_bitstream.cbegin(), m_condi_bitstream.cend(), m_encoded_bitstream.begin());
  size_t pos = m_condi_bitstream.size();
  std::copy(speck_stream.cbegin(), speck_stream.cend(), m_encoded_bitstream.begin() + pos);

  return RTNType::Good;
}

auto sperr::SPERR3D::decode() -> RTNType
{
  m_vals_d.clear();
  m_vals_ui.clear();
  m_vals_ll.clear();
  m_sign_array.clear();

  const auto total_vals = m_dims[0] * m_dims[1] * m_dims[2];

  // `m_condi_bitstream` might be indicating a constant field, so let's see if that's
  // the case, and if it is, we don't need to go through wavelet and speck stuff anymore.
  auto constant = m_conditioner.parse_constant(m_condi_bitstream);
  if (std::get<0>(constant)) {
    auto val = std::get<1>(constant);
    auto nval = std::get<2>(constant);
    m_vals_d.assign(nval, val);
    return RTNType::Good;
  }

  // Step 1: Integer SPECK decode.
  assert(!m_speck_bitstream.empty());
  m_decoder.set_dims(m_dims);
  m_decoder.use_bitstream(m_speck_bitstream);
  m_decoder.decode();

  // Step 2: Convert integers to doubles.
  const auto& vals_ui = m_decoder.view_coeffs();
  const auto& signs = m_decoder.view_signs();
  assert(vals_ui.size() == total_vals);
  assert(signs.size() == total_vals);
  m_translate_i2f(vals_ui, signs);
  assert(m_vals_d.size() == total_vals);

  // Step 3: Inverse Wavelet Transform
  m_cdf.take_data(std::move(m_vals_d), m_dims);
  m_cdf.idwt3d();

  // Step 4: Inverse Conditioning
  m_vals_d = m_cdf.release_data();
  m_conditioner.inverse_condition(m_vals_d, m_condi_bitstream);

  return RTNType::Good;
}

