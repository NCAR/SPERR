#include "SPECK3D_Compressor.h"

#include <cassert>
#include <cstring>

template <typename T>
auto SPECK3D_Compressor::copy_data(const T* p,
                                   size_t len,
                                   speck::dims_type dims) -> RTNType {
  static_assert(std::is_floating_point<T>::value,
                "!! Only floating point values are supported !!");

  if (len != dims[0] * dims[1] * dims[2])
    return RTNType::WrongSize;

  m_val_buf.resize(len);
  std::copy(p, p + len, m_val_buf.begin());

  m_dims = dims;

  return RTNType::Good;
}
template auto SPECK3D_Compressor::copy_data(const double*,
                                            size_t,
                                            speck::dims_type) -> RTNType;
template auto SPECK3D_Compressor::copy_data(const float*,
                                            size_t,
                                            speck::dims_type) -> RTNType;

auto SPECK3D_Compressor::take_data(speck::vecd_type&& buf,
                                   speck::dims_type dims) -> RTNType {
  if (buf.size() != dims[0] * dims[1] * dims[2])
    return RTNType::WrongSize;

  m_val_buf = std::move(buf);
  m_dims = dims;

  return RTNType::Good;
}

auto SPECK3D_Compressor::view_encoded_bitstream() const
    -> const std::vector<uint8_t>& {
  return m_encoded_stream;
}

auto SPECK3D_Compressor::release_encoded_bitstream() -> std::vector<uint8_t>&& {
  return std::move(m_encoded_stream);
}

#ifdef QZ_TERM
auto SPECK3D_Compressor::compress() -> RTNType {
  if (m_val_buf.empty())
    return RTNType::Error;
  m_condi_stream.clear();
  m_speck_stream.clear();
  m_sperr_stream.clear();
  m_num_outlier = 0;
  const auto total_vals = m_dims[0] * m_dims[1] * m_dims[2];

  // Note that we keep the original buffer untouched for outlier calculations
  // later. The buffer `m_val_buf2` will be recycled and reused though.
  m_val_buf2.resize(m_val_buf.size());
  std::copy(m_val_buf.begin(), m_val_buf.end(), m_val_buf2.begin());

  // Step 1: data (m_val_buf2) goes through the conditioner
  m_conditioner.toggle_all_settings(m_conditioning_settings);
  auto [rtn, condi_meta] = m_conditioner.condition(m_val_buf2);
  if (rtn != RTNType::Good)
    return rtn;
  m_condi_stream.resize(condi_meta.size());
  std::copy(condi_meta.begin(), condi_meta.end(), m_condi_stream.begin());

  // Step 2: wavelet transform
  rtn = m_cdf.take_data(std::move(m_val_buf2), m_dims);
  if (rtn != RTNType::Good)
    return rtn;
  // Figure out which dwt3d strategy to use.
  // Note: this strategy needs to be consistent with SPECK3D_Decompressor.
  auto xforms_xy = speck::num_of_xforms(std::min(m_dims[0], m_dims[1]));
  auto xforms_z = speck::num_of_xforms(m_dims[2]);
  if (xforms_xy == xforms_z)
    m_cdf.dwt3d_dyadic();
  else
    m_cdf.dwt3d_wavelet_packet();

  // Step 3: SPECK encoding
  rtn = m_encoder.take_data(m_cdf.release_data(), m_dims);
  if (rtn != RTNType::Good)
    return rtn;
  m_encoder.set_quantization_term_level(m_qz_lev);
  rtn = m_encoder.encode();
  if (rtn != RTNType::Good)
    return rtn;
  m_speck_stream = m_encoder.view_encoded_bitstream();
  if (m_speck_stream.empty())
    return RTNType::Error;

  // Step 4: perform a decompression pass (reusing the same object states and
  // memory blocks).
  rtn = m_encoder.decode();
  if (rtn != RTNType::Good)
    return rtn;
  m_cdf.take_data(m_encoder.release_data(), m_dims);
  if (xforms_xy == xforms_z)
    m_cdf.idwt3d_dyadic();
  else
    m_cdf.idwt3d_wavelet_packet();
  m_val_buf2 = m_cdf.release_data();
  m_conditioner.inverse_condition(m_val_buf2, m_condi_stream.data());

  // Step 5: we find all the outliers!
  //
  // Observation: for some data points, the reconstruction error in double falls
  // below `m_tol`, while in float would fall above `m_tol`. (Github issue #78).
  // Solution: find those data points, and use their slightly reduced error as
  // the new tolerance.
  //
  m_LOS.clear();
  auto new_tol = m_tol;
  m_diffv.resize(total_vals);
  for (size_t i = 0; i < total_vals; i++) {
    m_diffv[i] = m_val_buf[i] - m_val_buf2[i];
    auto f = std::abs(float(m_val_buf[i]) - float(m_val_buf2[i]));
    if (f > m_tol && std::abs(m_diffv[i]) <= m_tol)
      new_tol = std::min(new_tol, std::abs(m_diffv[i]));
  }
  for (size_t i = 0; i < total_vals; i++) {
    if (std::abs(m_diffv[i]) >= new_tol)
      m_LOS.emplace_back(i, m_diffv[i]);
  }
  m_sperr.set_tolerance(
      new_tol);  // Don't forget to pass in the new tolerance value!

  // Now we encode any outlier that's found.
  if (!m_LOS.empty()) {
    m_num_outlier = m_LOS.size();
    m_sperr.set_length(total_vals);
    m_sperr.copy_outlier_list(m_LOS);
    rtn = m_sperr.encode();
    if (rtn != RTNType::Good)
      return rtn;
    m_sperr_stream = m_sperr.get_encoded_bitstream();
    if (m_sperr_stream.empty())
      return RTNType::Error;
  }

  rtn = m_assemble_encoded_bitstream();

  return rtn;
}
//
// Finish QZ_TERM mode
//
#else
//
// Start fixed-size mode
//
auto SPECK3D_Compressor::compress() -> RTNType {
  const auto total_vals = m_dims[0] * m_dims[1] * m_dims[2];
  if (m_val_buf.size() != total_vals)
    return RTNType::Error;
  m_condi_stream.clear();
  m_speck_stream.clear();

  // Step 1: data goes through the conditioner
  m_conditioner.toggle_all_settings(m_conditioning_settings);
  auto [rtn, condi_meta] = m_conditioner.condition(m_val_buf);
  if (rtn != RTNType::Good)
    return rtn;
  m_condi_stream.resize(condi_meta.size());
  std::copy(condi_meta.begin(), condi_meta.end(), m_condi_stream.begin());

  // Step 2: wavelet transform
  rtn = m_cdf.take_data(std::move(m_val_buf), m_dims);
  if (rtn != RTNType::Good)
    return rtn;
  // Figure out which dwt3d strategy to use.
  // Note: this strategy needs to be consistent with SPECK3D_Decompressor.
  auto xforms_xy = speck::num_of_xforms(std::min(m_dims[0], m_dims[1]));
  auto xforms_z = speck::num_of_xforms(m_dims[2]);
  if (xforms_xy == xforms_z)
    m_cdf.dwt3d_dyadic();
  else
    m_cdf.dwt3d_wavelet_packet();
  auto cdf_out = m_cdf.release_data();

  // Step 3: SPECK encoding
  rtn = m_encoder.take_data(std::move(cdf_out), m_dims);
  if (rtn != RTNType::Good)
    return rtn;
  m_encoder.set_bit_budget(size_t(m_bpp * total_vals));
  rtn = m_encoder.encode();
  if (rtn != RTNType::Good)
    return rtn;
  m_speck_stream = m_encoder.view_encoded_bitstream();
  if (m_speck_stream.empty())
    return RTNType::Error;

  rtn = m_assemble_encoded_bitstream();

  return rtn;
}
#endif

#ifdef USE_ZSTD
auto SPECK3D_Compressor::m_assemble_encoded_bitstream() -> RTNType {
#ifdef QZ_TERM
  const size_t total_size =
      m_condi_stream.size() + m_speck_stream.size() + m_sperr_stream.size();
#else
  const size_t total_size = m_condi_stream.size() + m_speck_stream.size();
#endif

  // Need to have a ZSTD Compression Context first
  if (m_cctx == nullptr) {
    auto* ctx_p = ZSTD_createCCtx();
    if (ctx_p == nullptr)
      return RTNType::ZSTDError;
    else
      m_cctx.reset(ctx_p);
  }

  // If `m_zstd_buf` is not big enough for the decompressed buffer, we re-size
  // it.
  if (total_size > m_zstd_buf_len) {
    m_zstd_buf_len = std::max(total_size, m_zstd_buf_len * 2);
    m_zstd_buf = std::make_unique<uint8_t[]>(m_zstd_buf_len);
  }
  auto zstd_itr = m_zstd_buf.get();
  std::copy(m_condi_stream.begin(), m_condi_stream.end(), zstd_itr);
  zstd_itr += m_condi_stream.size();
  std::copy(m_speck_stream.begin(), m_speck_stream.end(), zstd_itr);
  zstd_itr += m_speck_stream.size();  // NOLINT

#ifdef QZ_TERM
  std::copy(m_sperr_stream.begin(), m_sperr_stream.end(), zstd_itr);
#endif

  const size_t comp_buf_size = ZSTD_compressBound(total_size);
  m_encoded_stream.resize(comp_buf_size);
  const size_t comp_size =
      ZSTD_compressCCtx(m_cctx.get(), m_encoded_stream.data(), comp_buf_size,
                        m_zstd_buf.get(), total_size, ZSTD_CLEVEL_DEFAULT + 6);
  if (ZSTD_isError(comp_size))
    return RTNType::ZSTDError;
  else {
    m_encoded_stream.resize(comp_size);
    return RTNType::Good;
  }
}
//
// Finish the USE_ZSTD case
//
#else
//
// Start the no-ZSTD case
//
auto SPECK3D_Compressor::m_assemble_encoded_bitstream() -> RTNType {
#ifdef QZ_TERM
  const size_t total_size =
      m_condi_stream.size() + m_speck_stream.size() + m_sperr_stream.size();
#else
  const size_t total_size = m_condi_stream.size() + m_speck_stream.size();
#endif

  m_encoded_stream.resize(total_size);
  std::copy(m_condi_stream.begin(), m_condi_stream.end(),
            m_encoded_stream.begin());
  auto buf_itr = m_encoded_stream.begin() + m_condi_stream.size();
  std::copy(m_speck_stream.begin(), m_speck_stream.end(), buf_itr);
  buf_itr += m_speck_stream.size();

#ifdef QZ_TERM
  std::copy(m_sperr_stream.begin(), m_sperr_stream.end(), buf_itr);
  buf_itr += m_sperr_stream.size();
#endif

  return RTNType::Good;
}
#endif

#ifdef QZ_TERM
void SPECK3D_Compressor::set_qz_level(int32_t q) {
  m_qz_lev = q;
}
auto SPECK3D_Compressor::set_tolerance(double tol) -> RTNType {
  if (tol <= 0.0)
    return RTNType::InvalidParam;
  else {
    m_tol = tol;
    m_sperr.set_tolerance(tol);
    return RTNType::Good;
  }
}
auto SPECK3D_Compressor::get_outlier_stats() const
    -> std::pair<size_t, size_t> {
  return {m_num_outlier, m_sperr_stream.size()};
}
#else
auto SPECK3D_Compressor::set_bpp(float bpp) -> RTNType {
  if (bpp < 0.0 || bpp > 64.0)
    return RTNType::InvalidParam;
  else {
    m_bpp = bpp;
    return RTNType::Good;
  }
}
#endif

void SPECK3D_Compressor::toggle_conditioning(std::array<bool, 8> b8) {
  m_conditioning_settings = b8;
}
