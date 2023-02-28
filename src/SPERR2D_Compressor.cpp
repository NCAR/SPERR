#include "SPERR2D_Compressor.h"

#include <algorithm>
#include <cassert>
#include <cstring>

#ifdef USE_ZSTD
#include "zstd.h"
#endif

template <typename T>
auto SPERR2D_Compressor::copy_data(const T* p, size_t len, sperr::dims_type dims) -> RTNType
{
  static_assert(std::is_floating_point<T>::value, "!! Only floating point values are supported !!");

  if constexpr (std::is_same<T, float>::value)
    m_orig_is_float = true;
  else
    m_orig_is_float = false;

  if (len != dims[0] * dims[1] || dims[2] != 1)
    return RTNType::WrongDims;

  m_val_buf.resize(len);
  std::copy(p, p + len, m_val_buf.begin());
  m_dims = dims;

  return RTNType::Good;
}
template auto SPERR2D_Compressor::copy_data(const double*, size_t, sperr::dims_type) -> RTNType;
template auto SPERR2D_Compressor::copy_data(const float*, size_t, sperr::dims_type) -> RTNType;

auto SPERR2D_Compressor::take_data(sperr::vecd_type&& buf, sperr::dims_type dims) -> RTNType
{
  if (buf.size() != dims[0] * dims[1] || dims[2] != 1)
    return RTNType::WrongDims;

  m_dims = dims;
  m_val_buf = std::move(buf);
  m_orig_is_float = false;

  return RTNType::Good;
}

auto SPERR2D_Compressor::get_outlier_stats() const -> std::pair<size_t, size_t>
{
  return {m_LOS.size(), m_sperr_stream.size()};
}

auto SPERR2D_Compressor::set_target_bpp(double bpp) -> RTNType
{
  const auto total_vals = m_dims[0] * m_dims[1];
  if (total_vals == 0)
    return RTNType::SetBPPBeforeDims;

  if (bpp <= 0.0 || bpp > 64.0)
    return RTNType::InvalidParam;
  else if (size_t(bpp * total_vals) <= (m_meta_size + m_condi_stream.size()) * 8)
    return RTNType::InvalidParam;

  m_bit_budget = static_cast<size_t>(bpp * double(total_vals));
  while (m_bit_budget % 8 != 0)
    --m_bit_budget;

  // Also set other termination criteria to be "never terminate."
  m_target_psnr = sperr::max_d;
  m_target_pwe = 0.0;

  return RTNType::Good;
}

void SPERR2D_Compressor::set_target_psnr(double psnr)
{
  m_target_psnr = std::max(psnr, 0.0);
  m_bit_budget = sperr::max_size;
  m_target_pwe = 0.0;
}

void SPERR2D_Compressor::set_target_pwe(double pwe)
{
  m_bit_budget = sperr::max_size;
  m_target_psnr = sperr::max_d;
  m_target_pwe = std::max(pwe, 0.0);
}

auto SPERR2D_Compressor::view_encoded_bitstream() const -> const sperr::vec8_type&
{
  return m_encoded_stream;
}

auto SPERR2D_Compressor::release_encoded_bitstream() -> sperr::vec8_type&&
{
  return std::move(m_encoded_stream);
}

auto SPERR2D_Compressor::compress() -> RTNType
{
  const auto total_vals = m_dims[0] * m_dims[1];
  if (m_val_buf.empty() || m_val_buf.size() != total_vals)
    return RTNType::Error;

  m_condi_stream.clear();
  m_speck_stream.clear();
  m_encoded_stream.clear();

  m_sperr_stream.clear();
  m_val_buf2.clear();
  m_LOS.clear();

  // Keep track of data range before and after the conditioning step, in case they change.
  // This is only used in `FixedPWE` mode though.
  auto range_before = double{0.0};
  auto range_after = double{0.0};

  // Find out the compression mode, and initialize data members accordingly.
  const auto mode = sperr::compression_mode(m_bit_budget, m_target_psnr, m_target_pwe);
  assert(mode != sperr::CompMode::Unknown);
  if (mode == sperr::CompMode::FixedPWE) {
    // Make a copy of the original data for outlier correction use.
    m_val_buf2.resize(total_vals);
    std::copy(m_val_buf.cbegin(), m_val_buf.cend(), m_val_buf2.begin());
    auto [min, max] = std::minmax_element(m_val_buf.cbegin(), m_val_buf.cend());
    range_before = *max - *min;
  }

  // Step 1: data goes through the conditioner
  m_condi_stream = m_conditioner.condition(m_val_buf, m_dims);
  // Step 1.1: Believe it or not, there are constant fields passed in for compression!
  // Let's detect that case and skip the rest of the compression routine if it occurs.
  if (m_conditioner.is_constant(m_condi_stream[0])) {
    auto rtn = m_assemble_encoded_bitstream();
    return rtn;
  }

  if (mode == sperr::CompMode::FixedPSNR) {
    // Calculate data range using the conditioned data, and pass it to the encoder.
    auto [min, max] = std::minmax_element(m_val_buf.cbegin(), m_val_buf.cend());
    auto range = *max - *min;
    m_encoder.set_data_range(range);
  }
  else if (mode == sperr::CompMode::FixedPWE &&
           m_conditioner.has_custom_filter(m_condi_stream[0])) {
    // Only re-calculate data range when there's custom filter enabled in the conditioner.
    auto [min, max] = std::minmax_element(m_val_buf.cbegin(), m_val_buf.cend());
    range_after = *max - *min;
  }

  // Step 2: wavelet transform
  auto rtn = m_cdf.take_data(std::move(m_val_buf), m_dims);
  if (rtn != RTNType::Good)
    return rtn;
  m_cdf.dwt2d();

  // Step 3: SPECK encoding
  rtn = m_encoder.take_data(m_cdf.release_data(), m_dims);
  if (rtn != RTNType::Good)
    return rtn;

  auto speck_bit_budget = size_t{0};
  if (m_bit_budget == sperr::max_size)
    speck_bit_budget = sperr::max_size;
  else
    speck_bit_budget = m_bit_budget - (m_meta_size + m_condi_stream.size()) * 8;

  // In the FixedPWE mode, in case there's custom filter, we scale the PWE tolerance
  auto speck_pwe = m_target_pwe;
  if (mode == sperr::CompMode::FixedPWE && m_conditioner.has_custom_filter(m_condi_stream[0])) {
    assert(range_before != 0.0);
    speck_pwe *= range_after / range_before;
  }

  m_encoder.set_comp_params(speck_bit_budget, m_target_psnr, speck_pwe);
  if (rtn != RTNType::Good)
    return rtn;

  rtn = m_encoder.encode();
  if (rtn != RTNType::Good)
    return rtn;

  m_speck_stream = m_encoder.view_encoded_bitstream();
  if (m_speck_stream.empty())
    return RTNType::Error;

  // Step 4: Outlier correction if in FixedPWE mode.
  if (mode == sperr::CompMode::FixedPWE) {
    // Step 4.1: IDWT using quantized coefficients to have a reconstruction.
    auto qz_coeff = m_encoder.release_quantized_coeff();
    assert(!qz_coeff.empty());
    m_cdf.take_data(std::move(qz_coeff), m_dims);
    m_cdf.idwt2d();
    m_val_buf = m_cdf.release_data();
    m_conditioner.inverse_condition(m_val_buf, m_dims, m_condi_stream);

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

auto SPERR2D_Compressor::m_assemble_encoded_bitstream() -> RTNType
{
  // This method does 3 things:
  // 1) prepend a proper header containing meta data.
  // 2) assemble conditioner, SPECK, and possibly SPERR bitstreams together.
  // 3) potentially apply ZSTD on the entire memory block except the meta data.

  // Meta data definition:
  // the 1st byte records the current major version of SPERR,
  // the 2nd byte records 8 booleans, with their meanings documented below, and
  // the next 8 bytes record X and Y dimension, 4 bytes each (in uint32_t type).
  //
  // bool_byte[0]  : if the rest of the stream is zstd compressed.
  // bool_byte[1]  : if this bitstream is for 3D (true) or 2D (false) data.
  // bool_byte[2]  : if the original data is float (true) or double (false).
  // bool_byte[3]  : has SPERR stream (true) or not (false).
  // bool_byte[3-7]: unused
  //
  auto meta = std::vector<uint8_t>(m_meta_size, 0);
  meta[0] = static_cast<uint8_t>(SPERR_VERSION_MAJOR);
  auto metabool = std::array<bool, 8>{false, false, false, false, false, false, false, false};

#ifdef USE_ZSTD
  metabool[0] = true;
#endif

  metabool[2] = m_orig_is_float;
  metabool[3] = !m_sperr_stream.empty();

  meta[1] = sperr::pack_8_booleans(metabool);

  // Copy X and Y dimensions to `meta`.
  const uint32_t dims[2] = {static_cast<uint32_t>(m_dims[0]), static_cast<uint32_t>(m_dims[1])};
  std::memcpy(&meta[2], dims, sizeof(dims));

  const auto total_size =
      m_meta_size + m_condi_stream.size() + m_speck_stream.size() + m_sperr_stream.size();

  // Task 2: assemble pieces of info together.
  m_encoded_stream.resize(total_size);
  auto itr = m_encoded_stream.begin();
  std::copy(std::cbegin(meta), std::cend(meta), itr);
  itr += m_meta_size;
  std::copy(m_condi_stream.cbegin(), m_condi_stream.cend(), itr);
  itr += m_condi_stream.size();
  std::copy(m_speck_stream.cbegin(), m_speck_stream.cend(), itr);
  itr += m_speck_stream.size();
  std::copy(m_sperr_stream.cbegin(), m_sperr_stream.cend(), itr);
  itr += m_sperr_stream.size();
  assert(itr == m_encoded_stream.end());

#ifdef USE_ZSTD
  // Task 3: apply ZSTD compression.
  const auto uncomp_size = total_size - m_meta_size;
  const auto comp_buf_len = ZSTD_compressBound(uncomp_size);
  auto comp_buf = sperr::vec8_type(m_meta_size + comp_buf_len);
  std::copy(std::cbegin(meta), std::cend(meta), comp_buf.begin());

  auto comp_size =
      ZSTD_compress(comp_buf.data() + m_meta_size, comp_buf_len,
                    m_encoded_stream.data() + m_meta_size, uncomp_size, ZSTD_CLEVEL_DEFAULT + 3);
  if (ZSTD_isError(comp_size)) {
    return RTNType::ZSTDError;
  }
  else {
    // Note: when the encoded stream is only a few kilobytes or smaller, the ZSTD compressed
    //       output can be larger.
    comp_buf.resize(m_meta_size + comp_size);
    m_encoded_stream = std::move(comp_buf);
  }
#endif

  return RTNType::Good;
}
