#include "SPECK3D_Compressor.h"

#include <cassert>
#include <cstring>

extern "C" {
  #include "libQccPack.h"
}

template <typename T>
auto SPECK3D_Compressor::copy_data(const T* p, size_t len, sperr::dims_type dims) -> RTNType
{
  static_assert(std::is_floating_point<T>::value, "!! Only floating point values are supported !!");

  if (len != dims[0] * dims[1] * dims[2])
    return RTNType::WrongDims;

  m_val_buf.resize(len);
  std::copy(p, p + len, m_val_buf.begin());

  m_dims = dims;

  return RTNType::Good;
}
template auto SPECK3D_Compressor::copy_data(const double*, size_t, sperr::dims_type) -> RTNType;
template auto SPECK3D_Compressor::copy_data(const float*, size_t, sperr::dims_type) -> RTNType;

auto SPECK3D_Compressor::take_data(sperr::vecd_type&& buf, sperr::dims_type dims) -> RTNType
{
  if (buf.size() != dims[0] * dims[1] * dims[2])
    return RTNType::WrongDims;

  m_val_buf = std::move(buf);
  m_dims = dims;

  return RTNType::Good;
}

auto SPECK3D_Compressor::view_encoded_bitstream() const -> const std::vector<uint8_t>&
{
  return m_encoded_stream;
}

auto SPECK3D_Compressor::release_encoded_bitstream() -> std::vector<uint8_t>&&
{
  return std::move(m_encoded_stream);
}

#ifdef QZ_TERM
auto SPECK3D_Compressor::compress() -> RTNType
{
  const auto total_vals = m_dims[0] * m_dims[1] * m_dims[2];
  if (m_val_buf.empty() || m_val_buf.size() != total_vals)
    return RTNType::Error;

  m_condi_stream.fill(0);
  m_speck_stream.clear();
  m_sperr_stream.clear();
  m_encoded_stream.clear();
  m_LOS.clear();

  // Believe it or not, there are constant fields passed in for compression!
  // Let's detect that case and skip the rest of the compression routine if it occurs.
  auto constant = m_conditioner.test_constant(m_val_buf);
  if (constant.first) {
    m_condi_stream = constant.second;
    auto tmp = m_assemble_encoded_bitstream();
    return tmp;
  }

  // Note: in the case where a positive error tolerance is given, this
  // method also performs error detection and correction.
  // In the case where a zero or negative error tolerance is given, this
  // method simply performs SPECK encoding and terminates at the specifie
  // quantization level.

  if (m_tol > 0.0) {
    m_val_buf2.resize(m_val_buf.size());
    std::copy(m_val_buf.begin(), m_val_buf.end(), m_val_buf2.begin());
  }
  else
    m_val_buf2.clear();

  // Step 1: data goes through the conditioner
  m_conditioner.toggle_all_settings(m_conditioning_settings);
  auto [rtn, condi_meta] = m_conditioner.condition(m_val_buf);
  if (rtn != RTNType::Good)
    return rtn;
  m_condi_stream = condi_meta;

  //
  // Debug steps: use Qcc DWT to test energy
  //
  // Debug Step 1: create Subband Pyramid
  QccWAVSubbandPyramid3D    coeff3d;
  QccWAVSubbandPyramid3DInitialize( &coeff3d );
  coeff3d.num_cols = m_dims[0];
  coeff3d.num_rows = m_dims[1];
  coeff3d.num_frames = m_dims[2];
  QccWAVSubbandPyramid3DAlloc( &coeff3d );
  size_t counter = 0;
  for (size_t z = 0; z < m_dims[2]; z++)
    for (size_t y = 0; y < m_dims[1]; y++)
      for (size_t x = 0; x < m_dims[0]; x++)
        coeff3d.volume[z][y][x] = m_val_buf[counter++];

  // Debug Step 2: create a wavelet
  QccString       WaveletFilename = "CohenDaubechiesFeauveau.5-3.fbk";
  QccString       Boundary = "symmetric";
  QccWAVWavelet   cdf97;
  if( QccWAVWaveletInitialize( &cdf97 ) ) {
    QccErrorAddMessage(" Error calling QccWAVWaveletInitialize()" );
    QccErrorExit();
  }
  if (QccWAVWaveletCreate( &cdf97, WaveletFilename, Boundary )) {
    QccErrorAddMessage(" Error calling QccWAVWaveletCreate()");
    QccErrorExit();
  }

  // Debug Step 3: forward and inverse transforms
  QccWAVSubbandPyramid3DDWT( &coeff3d, QCCWAVSUBBANDPYRAMID3D_DYADIC,
                             5, 5, &cdf97 );
  //QccWAVSubbandPyramid3DInverseDWT( &coeff3d, &cdf97 );

  // Debug Step 4: copy back the transformed coefficients
  auto qcc_vec = std::vector<double>(m_dims[0] * m_dims[1] * m_dims[2]);
  counter = 0;
  for (size_t z = 0; z < m_dims[2]; z++)
    for (size_t y = 0; y < m_dims[1]; y++)
      for (size_t x = 0; x < m_dims[0]; x++)
        qcc_vec[counter++] = coeff3d.volume[z][y][x];

  // Debug Step 5: calculate energy of transformed coefficients
  rtn = m_cdf.take_data(std::move(qcc_vec), m_dims);
  if (rtn != RTNType::Good)
    return rtn;
  auto qcc_energy = m_cdf.calc_energy();

  //
  // Finish Debug steps
  //

  // Step 2: wavelet transform
  rtn = m_cdf.take_data(std::move(m_val_buf), m_dims);
  if (rtn != RTNType::Good)
    return rtn;
  // Figure out which dwt3d strategy to use.
  // Note: this strategy needs to be consistent with SPECK3D_Decompressor.
  auto xforms_xy = sperr::num_of_xforms(std::min(m_dims[0], m_dims[1]));
  auto xforms_z = sperr::num_of_xforms(m_dims[2]);
  auto data_energy = m_cdf.calc_energy();
  if (xforms_xy == xforms_z)
    m_cdf.dwt3d_dyadic();
  else
    m_cdf.dwt3d_wavelet_packet();
  auto coeff_energy = m_cdf.calc_energy();
  auto diff = std::abs(data_energy - coeff_energy);
  //std::printf("--> data energy = %.2e, coeff energy = %.2e, diff = %.2e, pct = %.2f\n",
  //            data_energy, coeff_energy, diff, diff / data_energy * 100.0);
  auto qcc_diff = std::abs(qcc_energy - data_energy);
  std::printf("                            qcc   energy = %.2e, diff = %.2e, pct = %.2f\n",
              qcc_energy, qcc_diff, qcc_diff / data_energy * 100.0);

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

  if (m_tol <= 0.0) {
    // Not doing outlier correction, directly return the memory block to `m_val_buf`.
    m_val_buf = m_encoder.release_data();
  }
  else {
    // Optional steps: outlier detection and correction
    //
    // Step 4: perform a decompression pass
    rtn = m_encoder.decode();
    if (rtn != RTNType::Good)
      return rtn;
    m_cdf.take_data(m_encoder.release_data(), m_dims);
    if (xforms_xy == xforms_z)
      m_cdf.idwt3d_dyadic();
    else
      m_cdf.idwt3d_wavelet_packet();
    m_val_buf = m_cdf.release_data();
    m_conditioner.inverse_condition(m_val_buf, m_condi_stream);

    // Step 5: we find all the outliers!
    //
    // Observation: for some data points, the reconstruction error in double falls
    // below `m_tol`, while in float would fall above `m_tol`. (Github issue #78).
    // Solution: find those data points, and use their slightly reduced error as
    // the new tolerance.
    //
    auto new_tol = m_tol;
    for (size_t i = 0; i < total_vals; i++) {
      const auto d = std::abs(m_val_buf2[i] - m_val_buf[i]);
      const float f = std::abs(float(m_val_buf2[i]) - float(m_val_buf[i]));
      if (double(f) > m_tol && d <= m_tol)
        new_tol = std::min(new_tol, d);
    }
    for (size_t i = 0; i < total_vals; i++) {
      const auto diff = m_val_buf2[i] - m_val_buf[i];
      if (std::abs(diff) > new_tol)
        m_LOS.emplace_back(i, diff);
    }

    // Step 6: encode any outlier that's found.
    if (!m_LOS.empty()) {
      m_sperr.set_tolerance(new_tol);
      m_sperr.set_length(total_vals);
      m_sperr.copy_outlier_list(m_LOS);
      rtn = m_sperr.encode();
      if (rtn != RTNType::Good)
        return rtn;
      m_sperr_stream = m_sperr.get_encoded_bitstream();
      if (m_sperr_stream.empty())
        return RTNType::Error;
    }
  }  // Finish outlier detection and correction

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
auto SPECK3D_Compressor::compress() -> RTNType
{
  const auto total_vals = m_dims[0] * m_dims[1] * m_dims[2];
  if (m_val_buf.empty() || m_val_buf.size() != total_vals)
    return RTNType::Error;
  m_condi_stream.fill(0);
  m_speck_stream.clear();

  // Believe it or not, there are constant fields passed in for compression!
  // Let's detect that case and skip the rest of the compression routine if it occurs.
  auto constant = m_conditioner.test_constant(m_val_buf);
  if (constant.first) {
    m_condi_stream = constant.second;
    auto tmp = m_assemble_encoded_bitstream();
    return tmp;
  }

  // Step 1: data goes through the conditioner
  m_conditioner.toggle_all_settings(m_conditioning_settings);
  auto [rtn, condi_meta] = m_conditioner.condition(m_val_buf);
  if (rtn != RTNType::Good)
    return rtn;
  m_condi_stream = condi_meta;

  // Step 2: wavelet transform
  rtn = m_cdf.take_data(std::move(m_val_buf), m_dims);
  if (rtn != RTNType::Good)
    return rtn;
  // Figure out which dwt3d strategy to use.
  // Note: this strategy needs to be consistent with SPECK3D_Decompressor.
  auto xforms_xy = sperr::num_of_xforms(std::min(m_dims[0], m_dims[1]));
  auto xforms_z = sperr::num_of_xforms(m_dims[2]);
  if (xforms_xy == xforms_z)
    m_cdf.dwt3d_dyadic();
  else
    m_cdf.dwt3d_wavelet_packet();
  auto cdf_out = m_cdf.release_data();

  // Step 3: SPECK encoding
  rtn = m_encoder.take_data(std::move(cdf_out), m_dims);
  if (rtn != RTNType::Good)
    return rtn;
  m_encoder.set_bit_budget(size_t(m_bpp * total_vals) - m_condi_stream.size() * 8);
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
auto SPECK3D_Compressor::m_assemble_encoded_bitstream() -> RTNType
{
#ifdef QZ_TERM
  const size_t total_size = m_condi_stream.size() + m_speck_stream.size() + m_sperr_stream.size();
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

  // If `m_zstd_buf` is not big enough for the total buffer, we re-size it.
  m_zstd_buf.resize(total_size);
  auto zstd_itr = m_zstd_buf.begin();
  std::copy(m_condi_stream.begin(), m_condi_stream.end(), zstd_itr);
  zstd_itr += m_condi_stream.size();
  std::copy(m_speck_stream.begin(), m_speck_stream.end(), zstd_itr);
  zstd_itr += m_speck_stream.size();
#ifdef QZ_TERM
  std::copy(m_sperr_stream.begin(), m_sperr_stream.end(), zstd_itr);
  zstd_itr += m_sperr_stream.size();
#endif
  assert(zstd_itr == m_zstd_buf.end());

  const size_t comp_buf_size = ZSTD_compressBound(total_size);
  m_encoded_stream.resize(comp_buf_size);
  const size_t comp_size =
      ZSTD_compressCCtx(m_cctx.get(), m_encoded_stream.data(), comp_buf_size, m_zstd_buf.data(),
                        total_size, ZSTD_CLEVEL_DEFAULT + 6);
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
auto SPECK3D_Compressor::m_assemble_encoded_bitstream() -> RTNType
{
#ifdef QZ_TERM
  const size_t total_size = m_condi_stream.size() + m_speck_stream.size() + m_sperr_stream.size();
#else
  const size_t total_size = m_condi_stream.size() + m_speck_stream.size();
#endif

  m_encoded_stream.resize(total_size);
  std::copy(m_condi_stream.begin(), m_condi_stream.end(), m_encoded_stream.begin());
  auto buf_itr = m_encoded_stream.begin() + m_condi_stream.size();
  std::copy(m_speck_stream.begin(), m_speck_stream.end(), buf_itr);
  buf_itr += m_speck_stream.size();
#ifdef QZ_TERM
  std::copy(m_sperr_stream.begin(), m_sperr_stream.end(), buf_itr);
  buf_itr += m_sperr_stream.size();
#endif
  assert(buf_itr == m_encoded_stream.end());

  return RTNType::Good;
}
#endif

#ifdef QZ_TERM
void SPECK3D_Compressor::set_qz_level(int32_t q)
{
  m_qz_lev = q;
}
void SPECK3D_Compressor::set_tolerance(double tol)
{
  m_tol = tol;
}
auto SPECK3D_Compressor::get_outlier_stats() const -> std::pair<size_t, size_t>
{
  return {m_LOS.size(), m_sperr_stream.size()};
}
#else
auto SPECK3D_Compressor::set_bpp(double bpp) -> RTNType
{
  const auto total_vals = m_dims[0] * m_dims[1] * m_dims[2];
  if (bpp < 0.0 || bpp > 64.0)
    return RTNType::InvalidParam;
  else if (size_t(bpp * total_vals) <= m_condi_stream.size() * 8)
    return RTNType::InvalidParam;
  else {
    m_bpp = bpp;
    return RTNType::Good;
  }
}
#endif

void SPECK3D_Compressor::toggle_conditioning(sperr::Conditioner::settings_type b4)
{
  m_conditioning_settings = b4;
}
