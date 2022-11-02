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

auto sperr::SPERR3D_INT_Driver::release_speck_bitstream() -> std::vector<uint8_t>&&
{
  return std::move(m_speck_stream);
}

auto sperr::SPERR3D_INT_Driver::compress() -> RTNType
{
  const auto total_vals = m_dims[0] * m_dims[1] * m_dims[2];
  if (m_vals_d.empty() || m_vals_d.size() != total_vals)
    return RTNType::Error;

  m_condi_stream.fill(0);
  m_speck_stream.clear();
  m_encoded_stream.clear();

  // Believe it or not, there are constant fields passed in for compression!
  // Let's detect that case and skip the rest of the compression routine if it occurs.
  auto constant = m_conditioner.test_constant(m_vals_d);
  if (constant.first) {
    m_condi_stream = constant.second;
    auto tmp = m_assemble_encoded_bitstream();
    return tmp;
  }

  // Find out the compression mode, and initialize data members accordingly.
  //const auto mode = sperr::compression_mode(m_bit_budget, m_target_psnr, m_target_pwe);
  //assert(mode != CompMode::Unknown);
  //if (mode == sperr::CompMode::FixedPSNR) {
  //  auto [min, max] = std::minmax_element(m_val_buf.cbegin(), m_val_buf.cend());
  //  auto range = *max - *min;
  //  m_encoder.set_data_range(range);
  //}
  //else if (mode == sperr::CompMode::FixedPWE) {
  //  m_val_buf2.resize(total_vals);
  //  std::copy(m_val_buf.cbegin(), m_val_buf.cend(), m_val_buf2.begin());
  //}

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
  m_vals_i.resize(total_vals);
  m_sign_array.resize(total_vals);
#pragma STDC FENV_ACCESS ON
  std::fesetround(FE_TONEAREST);
  std::transform(m_vals_d.cbegin(), m_vals_d.cend(), m_vals_ll.begin(),
                 [](auto d){ return std::llrint(d/q); });
  std::transform(m_vals_ll.cbegin(), m_vals_ll.cend(), m_vals_i.begin(),
                 [](auto ll){ return static_cast<uint64_t>(std::abs(ll)); });
  std::transform(m_vals_ll.cbegin(), m_vals_ll.cend(), m_sign_array.begin(),
                 [](auto ll){ return ll >= 0ll; });

  // Step 3: Integer SPECK encoding
  rtn = m_encoder.take_data(m_cdf.release_data(), m_dims);
  if (rtn != RTNType::Good)
    return rtn;

  auto speck_budget = size_t{0};
  if (m_bit_budget == sperr::max_size)
    speck_budget = sperr::max_size;
  else
    speck_budget = m_bit_budget - m_condi_stream.size() * 8;
  rtn = m_encoder.set_comp_params(speck_budget, m_target_psnr, m_target_pwe);
  if (rtn != RTNType::Good)
    return rtn;

  rtn = m_encoder.encode();
  if (rtn != RTNType::Good)
    return rtn;

  // We can copy the encoded speck stream to `m_encoded_stream` at the appropriate position.
  const auto& speck_stream = m_encoder.view_encoded_bitstream();
  if (speck_stream.empty())
    return RTNType::Error;
  const size_t condi_speck_len = m_condi_stream.size() + speck_stream.size();
  m_encoded_stream.resize(condi_speck_len);
  std::copy(speck_stream.cbegin(), speck_stream.cend(),
            m_encoded_stream.begin() + m_condi_stream.size());

  // Step 4: Outlier correction if in FixedPWE mode.
  if (mode == sperr::CompMode::FixedPWE) {
    // Step 4.1: IDWT using quantized coefficients to have a reconstruction.
    auto qz_coeff = m_encoder.release_quantized_coeff();
    assert(!qz_coeff.empty());
    m_cdf.take_data(std::move(qz_coeff), m_dims);
    m_cdf.idwt3d();
    m_val_buf = m_cdf.release_data();
    m_conditioner.inverse_condition(m_val_buf, m_condi_stream);

    // Step 4.2: Find all outliers
    for (size_t i = 0; i < total_vals; i++) {
      const auto diff = m_val_buf2[i] - m_val_buf[i];
      if (std::abs(diff) >= m_target_pwe)
        m_LOS.emplace_back(i, diff);
    }

    // Step 4.3: Code located outliers
    if (!m_LOS.empty()) {
      m_sperr.set_tolerance(m_target_pwe);
      m_sperr.set_length(total_vals);
      m_sperr.copy_outlier_list(m_LOS);
      rtn = m_sperr.encode();
      if (rtn != RTNType::Good)
        return rtn;
      m_sperr_stream = m_sperr.get_encoded_bitstream();
      if (m_sperr_stream.empty())
        return RTNType::Error;
    }
  }

  rtn = m_assemble_encoded_bitstream();

  return rtn;
}

#if 0
auto sperr::SPERR3D_Compressor::m_assemble_encoded_bitstream() -> RTNType
{
  // Copy over the condi stream.
  // `m_encoded_stream` is either empty, or is already allocated space.
  if (m_encoded_stream.empty())
    m_encoded_stream.resize(m_condi_stream.size());
  std::copy(m_condi_stream.begin(), m_condi_stream.end(), m_encoded_stream.begin());

  // If there's outlier correction stream, copy it over too.
  if (!m_sperr_stream.empty()) {
    const auto condi_speck_len = m_encoded_stream.size();
    assert(condi_speck_len > m_condi_stream.size());
    m_encoded_stream.resize(condi_speck_len + m_sperr_stream.size());
    std::copy(m_sperr_stream.cbegin(), m_sperr_stream.cend(),
              m_encoded_stream.begin() + condi_speck_len);
  }

#ifdef USE_ZSTD
  if (m_cctx == nullptr) {
    auto* ctx_p = ZSTD_createCCtx();
    if (ctx_p == nullptr)
      return RTNType::ZSTDError;
    else
      m_cctx.reset(ctx_p);
  }

  const size_t comp_upper_size = ZSTD_compressBound(m_encoded_stream.size());
  m_zstd_buf.resize(comp_upper_size);
  const size_t comp_size =
      ZSTD_compressCCtx(m_cctx.get(), m_zstd_buf.data(), comp_upper_size, m_encoded_stream.data(),
                        m_encoded_stream.size(), ZSTD_CLEVEL_DEFAULT + 6);
  if (ZSTD_isError(comp_size))
    return RTNType::ZSTDError;
  else {
    // Note: when the encoded stream is only a few kilobytes or smaller, the ZSTD compressed
    //       output can be larger.
    m_encoded_stream.resize(comp_size);
    std::copy(m_zstd_buf.cbegin(), m_zstd_buf.cbegin() + comp_size, m_encoded_stream.begin());
  }
#endif

  return RTNType::Good;
}
#endif
