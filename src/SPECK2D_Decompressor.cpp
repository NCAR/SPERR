#include "SPECK2D_Decompressor.h"

#include <cassert>
#include <cstring>

#ifdef USE_ZSTD
#include "zstd.h"
#endif

auto SPECK2D_Decompressor::use_bitstream(const void* p, size_t len) -> RTNType
{
  // This method parses the metadata of a bitstream and performs the following tasks:
  // 1) Verify if the major version number is consistant.
  // 2) Make sure that the bitstream is for 2D encoding, and consistent in QZ_TERM.
  // 3) Verify that the application of ZSTD is consistant, and apply ZSTD if needed.
  // 4) disassemble conditioner, SPECK, and possibly SPERR streams.
  // 5) Extract dimensions from the SPECK stream.

  m_condi_stream.fill(0);
  m_speck_stream.clear();
  m_val_buf.clear();
#ifdef QZ_TERM
  m_sperr_stream.clear();
#endif

  const uint8_t* u8p = static_cast<const uint8_t*>(p);

  auto meta = std::array<uint8_t, 2>{0, 0};
  assert(meta.size() == m_meta_size);
  std::copy(u8p, u8p + m_meta_size, meta.begin());
  auto metabool = sperr::unpack_8_booleans(meta[1]);

  // Task 1)
  if (meta[0] != uint8_t(SPERR_VERSION_MAJOR))
    return RTNType::VersionMismatch;

  // Task 2)
  if (metabool[1] == true)  // true means it's 3D
    return RTNType::SliceVolumeMismatch;

#ifdef QZ_TERM
  if (metabool[2] == false) // false means it's in fixed-size mode
    return RTNType::QzModeMismatch;
  const auto sperr_empty = metabool[3];
#else
  if (metabool[2] == true) // true means it's in fixed-error mode
    return RTNType::QzModeMismatch;
#endif

  u8p += m_meta_size;
  size_t plen = 0;
  if (len >= m_meta_size)
    plen = len - m_meta_size;
  else
    return RTNType::BitstreamWrongLen;

    // Task 3)
#ifdef USE_ZSTD
  if (metabool[0] == false)
    return RTNType::ZSTDMismatch;

  const auto content_size = ZSTD_getFrameContentSize(u8p, plen);
  if (content_size == ZSTD_CONTENTSIZE_ERROR || content_size == ZSTD_CONTENTSIZE_UNKNOWN)
    return RTNType::ZSTDError;

  auto content_buf = std::make_unique<uint8_t[]>(content_size);
  const auto decomp_size = ZSTD_decompress(content_buf.get(), content_size, u8p, plen);
  if (ZSTD_isError(decomp_size) || decomp_size != content_size)
    return RTNType::ZSTDError;

  // Redirect `u8p` to point to the beginning of conditioning stream
  u8p = content_buf.get();
  plen = content_size;
#else
  if (metabool[0] == true)
    return RTNType::ZSTDMismatch;
#endif

  // Task 4)
  const auto condi_size = m_condi_stream.size();
  if (condi_size > plen)
    return RTNType::BitstreamWrongLen;
  std::copy(u8p, u8p + condi_size, m_condi_stream.begin());
  u8p += condi_size;
  plen -= condi_size;

  // `m_condi_stream` could indicate that the field is constant, in which case,
  // there is no speck stream anymore.  Let's detect that case and return early.
  // It will be the responsibility of `decompress()` to actually restore the constant field.
  auto [constant, tmp1, tmp2] = m_conditioner.parse_constant(m_condi_stream);
  if (constant) {
    if (plen == 0)
      return RTNType::Good;
    else
      return RTNType::BitstreamWrongLen;
  }

  const auto speck_size = m_decoder.get_speck_stream_size(u8p);
  if (speck_size > plen)
    return RTNType::BitstreamWrongLen;
#ifndef QZ_TERM
  if (speck_size != plen)
    return RTNType::BitstreamWrongLen;
#endif
  m_speck_stream.resize(speck_size);
  std::copy(u8p, u8p + speck_size, m_speck_stream.begin());
  u8p += speck_size;
  plen -= speck_size;

#ifdef QZ_TERM
  if (sperr_empty){
    if (plen != 0)
      return RTNType::BitstreamWrongLen;
  }
  else {
    const auto sperr_size = m_sperr.get_sperr_stream_size(u8p);
    if (sperr_size != plen)
      return RTNType::BitstreamWrongLen;
    m_sperr_stream.resize(sperr_size);
    std::copy(u8p, u8p + sperr_size, m_sperr_stream.begin());
  }
#endif

  // Task 5)
  m_dims = m_decoder.get_speck_stream_dims(m_speck_stream.data());
  if (m_dims[2] != 1)
    return RTNType::SliceVolumeMismatch;

  return RTNType::Good;
}

#ifndef QZ_TERM
auto SPECK2D_Decompressor::set_bpp(double bpp) -> RTNType
{
  if (bpp < 0.0 || bpp > 64.0)
    return RTNType::InvalidParam;
  else {
    m_bpp = bpp;
    return RTNType::Good;
  }
}
#endif

template <typename T>
auto SPECK2D_Decompressor::get_data() const -> std::vector<T>
{
  auto out_buf = std::vector<T>(m_val_buf.size());
  std::copy(m_val_buf.begin(), m_val_buf.end(), out_buf.begin());

  return out_buf;
}
template auto SPECK2D_Decompressor::get_data() const -> std::vector<double>;
template auto SPECK2D_Decompressor::get_data() const -> std::vector<float>;

auto SPECK2D_Decompressor::view_data() const -> const std::vector<double>&
{
  return m_val_buf;
}

auto SPECK2D_Decompressor::release_data() -> std::vector<double>&&
{
  m_dims = {0, 0, 0};
  return std::move(m_val_buf);
}

auto SPECK2D_Decompressor::get_dims() const -> std::array<size_t, 3>
{
  return m_dims;
}

auto SPECK2D_Decompressor::decompress() -> RTNType
{
  // `m_condi_stream` might be indicating a constant field, so let's test and handle that case.
  auto [constant, const_val, nconst_vals] = m_conditioner.parse_constant(m_condi_stream);
  if (constant) {
    m_val_buf.assign(nconst_vals, const_val);
    return RTNType::Good;
  }

  // Step 1: SPECK decode
  const auto total_vals = m_dims[0] * m_dims[1];
  if (m_speck_stream.empty())
    return RTNType::Error;
  auto rtn = m_decoder.parse_encoded_bitstream(m_speck_stream.data(), m_speck_stream.size());
  if (rtn != RTNType::Good)
    return rtn;
#ifndef QZ_TERM
  m_decoder.set_bit_budget(size_t(m_bpp * double(total_vals)));
#endif
  rtn = m_decoder.decode();
  if (rtn != RTNType::Good)
    return rtn;

  // Step 2: Inverse Wavelet transform
  auto decoder_out = m_decoder.release_data();
  if (decoder_out.size() != total_vals)
    return RTNType::Error;
  m_cdf.take_data(std::move(decoder_out), m_dims);
  m_cdf.idwt2d();

  // Step 3: Inverse Conditioning
  auto cdf_out = m_cdf.release_data();
  m_conditioner.inverse_condition(cdf_out, m_condi_stream);

#ifdef QZ_TERM
  // Step 4: if there's SPERR stream, then do outlier correction.
  if (!m_sperr_stream.empty()) {
    rtn = m_sperr.parse_encoded_bitstream(m_sperr_stream.data(), m_sperr_stream.size());
    if (rtn != RTNType::Good)
      return rtn;
    rtn = m_sperr.decode();
    if (rtn != RTNType::Good)
      return rtn;

    const auto& los = m_sperr.view_outliers();
    for (const auto& out : los)
      cdf_out[out.location] += out.error;
  }
#endif

  m_val_buf = std::move(cdf_out);

  return RTNType::Good;
}
