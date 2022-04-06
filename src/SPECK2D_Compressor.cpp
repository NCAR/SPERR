#include "SPECK2D_Compressor.h"

#include <cassert>
#include <cstring>

#ifdef USE_ZSTD
#include "zstd.h"
#endif

template <typename T>
auto SPECK2D_Compressor::copy_data(const T* p, size_t len, sperr::dims_type dims) -> RTNType
{
  static_assert(std::is_floating_point<T>::value, "!! Only floating point values are supported !!");

  if (len != dims[0] * dims[1] || dims[2] != 1)
    return RTNType::WrongDims;

  m_val_buf.resize(len);
  std::copy(p, p + len, m_val_buf.begin());
  m_dims = dims;

  return RTNType::Good;
}
template auto SPECK2D_Compressor::copy_data(const double*, size_t, sperr::dims_type) -> RTNType;
template auto SPECK2D_Compressor::copy_data(const float*, size_t, sperr::dims_type) -> RTNType;

auto SPECK2D_Compressor::take_data(std::vector<double>&& buf, sperr::dims_type dims) -> RTNType
{
  if (buf.size() != dims[0] * dims[1] || dims[2] != 1)
    return RTNType::WrongDims;

  m_dims = dims;
  m_val_buf = std::move(buf);

  return RTNType::Good;
}

#ifdef QZ_TERM
void SPECK2D_Compressor::set_qz_level(int32_t q)
{
  m_qz_lev = q;
}
auto SPECK2D_Compressor::set_tolerance(double tol) -> RTNType
{
  if (tol <= 0.0)
    return RTNType::InvalidParam;
  else {
    m_tol = tol;
    return RTNType::Good;
  }
}
auto SPECK2D_Compressor::get_outlier_stats() const -> std::pair<size_t, size_t>
{
  return {m_num_outlier, m_sperr_stream.size()};
}
#else
auto SPECK2D_Compressor::set_bpp(double bpp) -> RTNType
{
  const auto total_vals = m_dims[0] * m_dims[1];
  if (bpp <= 0.0 || bpp > 64.0)
    return RTNType::InvalidParam;
  else if (size_t(bpp * total_vals) <= (m_meta_size + m_condi_stream.size()) * 8)
    return RTNType::InvalidParam;
  else {
    m_bpp = bpp;
    return RTNType::Good;
  }
}
#endif

void SPECK2D_Compressor::toggle_conditioning(sperr::Conditioner::settings_type settings)
{
  m_conditioning_settings = settings;
}

auto SPECK2D_Compressor::view_encoded_bitstream() const -> const std::vector<uint8_t>&
{
  return m_encoded_stream;
}

auto SPECK2D_Compressor::release_encoded_bitstream() -> std::vector<uint8_t>&&
{
  return std::move(m_encoded_stream);
}

auto SPECK2D_Compressor::compress() -> RTNType
{
  const auto total_vals = m_dims[0] * m_dims[1];
  if (m_val_buf.empty() || m_val_buf.size() != total_vals)
    return RTNType::Error;

  m_condi_stream.fill(0);
  m_speck_stream.clear();
  m_encoded_stream.clear();

#ifdef QZ_TERM
  m_sperr_stream.clear();
  m_num_outlier = 0;
#endif

  // Believe it or not, there are constant fields passed in for compression!
  // Let's detect that case and skip the rest of the compression routine if it occurs.
  auto constant = m_conditioner.test_constant(m_val_buf);
  if (constant.first) {
    m_condi_stream = constant.second;
    auto tmp = m_assemble_encoded_bitstream();
    return tmp;
  }

#ifdef QZ_TERM
  // We make a copy of the original buffer for the outlier calculation.
  m_val_buf2.resize(total_vals);
  std::copy(m_val_buf.begin(), m_val_buf.end(), m_val_buf2.begin());
#endif

  // Step 1: data goes through the conditioner
  m_conditioner.toggle_all_settings(m_conditioning_settings);
  auto [rtn, condi_meta] = m_conditioner.condition(m_val_buf);
  if (rtn != RTNType::Good)
    return rtn;
  m_condi_stream = condi_meta;  // Copy conditioner meta data to `m_condi_stream`

  // Step 2: wavelet transform
  rtn = m_cdf.take_data(std::move(m_val_buf), m_dims);
  if (rtn != RTNType::Good)
    return rtn;
  m_cdf.dwt2d();

  // Step 3: SPECK encoding
  rtn = m_encoder.take_data(m_cdf.release_data(), m_dims);
  if (rtn != RTNType::Good)
    return rtn;

#ifdef QZ_TERM
  m_encoder.set_quantization_term_level(m_qz_lev);
#else
  m_encoder.set_bit_budget(size_t(m_bpp * total_vals) - (m_meta_size + m_condi_stream.size()) * 8);
#endif

  rtn = m_encoder.encode();
  if (rtn != RTNType::Good)
    return rtn;
  // Copy SPECK bitstream to `m_speck_stream` so `m_encoder` can keep its internal storage.
  // The decision to copy rather than release is because that this stream is small so cheaper
  // to copy, and alos it's grown gradually inside of `m_encoder`.
  m_speck_stream = m_encoder.view_encoded_bitstream();
  if (m_speck_stream.empty())
    return RTNType::Error;

#ifdef QZ_TERM
  // Step 4: perform a decompression pass
  rtn = m_encoder.decode();
  if (rtn != RTNType::Good)
    return rtn;
  m_cdf.take_data(m_encoder.release_data(), m_dims);
  m_cdf.idwt2d();
  m_val_buf = m_cdf.release_data();
  m_conditioner.inverse_condition(m_val_buf, m_condi_stream);

  // Step 5: find all the outliers!
  // Note: the cumbersome calculations below prevents the situation where deviations
  // are below the threshold in double precision, but above in single precision!
  m_LOS.clear();
  auto new_tol = m_tol;
  m_diffv.resize(total_vals);
  for (size_t i = 0; i < total_vals; i++)
    m_diffv[i] = m_val_buf2[i] - m_val_buf[i];
  for (size_t i = 0; i < total_vals; i++) {
    auto f = std::abs(float(m_val_buf2[i]) - float(m_val_buf[i]));
    if (double(f) > m_tol && std::abs(m_diffv[i]) <= m_tol)
      new_tol = std::min(new_tol, std::abs(m_diffv[i]));
  }
  for (size_t i = 0; i < total_vals; i++) {
    if (std::abs(m_diffv[i]) > new_tol)
      m_LOS.emplace_back(i, m_diffv[i]);
  }

  // Step 6: actually encode located outliers
  if (!m_LOS.empty()) {
    m_num_outlier = m_LOS.size();
    m_sperr.set_tolerance(new_tol);
    m_sperr.set_length(total_vals);
    m_sperr.take_outlier_list(std::move(m_LOS));
    rtn = m_sperr.encode();
    if (rtn != RTNType::Good)
      return rtn;
    m_sperr_stream = m_sperr.get_encoded_bitstream();
    if (m_sperr_stream.empty())
      return RTNType::Error;
  }
#endif

  rtn = m_assemble_encoded_bitstream();

  return rtn;
}

auto SPECK2D_Compressor::m_assemble_encoded_bitstream() -> RTNType
{
  // This method does 3 things:
  // 1) prepend a proper header containing meta data.
  // 2) assemble conditioner, SPECK, and possibly SPERR bitstreams together.
  // 3) potentially apply ZSTD on the entire memory block except the meta data.

  // Meta data definition:
  // the 1st byte records the current major version of SPECK, and
  // the 2nd byte records 8 booleans, with their meanings documented below:
  //
  // bool_byte[0]  : if the rest of the stream is zstd compressed.
  // bool_byte[1]  : if this bitstream is for 3D (true) or 2D (false) data.
  // bool_byte[2]  : if this bitstream is in QZ_TERM mode (true) or fixed-size mode (false).
  // bool_byte[3]  : if bool_byte[2]==true, the SPERR stream is empty (true) or not (false).
  // bool_byte[4-7]: unused
  //
  auto meta = std::array<uint8_t, 2>{uint8_t(SPERR_VERSION_MAJOR), 0};
  assert(sizeof(meta) == m_meta_size);
  auto metabool = std::array<bool, 8>{false, false, false, false, false, false, false, false};

#ifdef USE_ZSTD
  metabool[0] = true;
#endif

#ifdef QZ_TERM
  metabool[2] = true;
  metabool[3] = m_sperr_stream.empty();
#endif

  meta[1] = sperr::pack_8_booleans(metabool);

#ifdef QZ_TERM
  const auto total_size =
      m_meta_size + m_condi_stream.size() + m_speck_stream.size() + m_sperr_stream.size();
#else
  const auto total_size = m_meta_size + m_condi_stream.size() + m_speck_stream.size();
#endif

  // Task 2: assemble pieces of info together.
  m_encoded_stream.resize(total_size);
  auto itr = m_encoded_stream.begin();
  std::copy(std::begin(meta), std::end(meta), itr);
  itr += m_meta_size;
  std::copy(m_condi_stream.begin(), m_condi_stream.end(), itr);
  itr += m_condi_stream.size();
  std::copy(m_speck_stream.begin(), m_speck_stream.end(), itr);
  itr += m_speck_stream.size();
#ifdef QZ_TERM
  std::copy(m_sperr_stream.begin(), m_sperr_stream.end(), itr);
  itr += m_sperr_stream.size();
#endif
  assert(itr == m_encoded_stream.end());

#ifdef USE_ZSTD
  // Task 3: apply ZSTD compression.
  const auto uncomp_size = total_size - m_meta_size;
  const auto comp_buf_len = ZSTD_compressBound(uncomp_size);
  auto comp_buf = sperr::vec8_type(m_meta_size + comp_buf_len);
  std::copy(std::begin(meta), std::end(meta), comp_buf.begin());

  auto comp_size =
      ZSTD_compress(comp_buf.data() + m_meta_size, comp_buf_len,
                    m_encoded_stream.data() + m_meta_size, uncomp_size, ZSTD_CLEVEL_DEFAULT + 3);
  if (ZSTD_isError(comp_size)) {
    return RTNType::ZSTDError;
  }
  else {
    comp_buf.resize(m_meta_size + comp_size);
    m_encoded_stream = std::move(comp_buf);
  }
#endif

  return RTNType::Good;
}
