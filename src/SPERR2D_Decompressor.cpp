#include "SPERR2D_Decompressor.h"

#include <cassert>
#include <cstring>

#ifdef USE_ZSTD
#include "zstd.h"
#endif

auto SPERR2D_Decompressor::use_bitstream(const void* p, size_t len) -> RTNType
{
  // This method parses the metadata of a bitstream and performs the following tasks:
  // 1) Verify if the major version number is consistant.
  // 2) Make sure that the bitstream is for 2D encoding.
  // 3) Extract dimensions.
  // 4) Verify that the application of ZSTD is consistant, and apply ZSTD if needed.
  // 5) disassemble conditioner, SPECK, and possibly SPERR streams.

  m_condi_stream.clear();
  m_speck_stream.clear();
  m_val_buf.clear();
  m_sperr_stream.clear();

  const uint8_t* u8p = static_cast<const uint8_t*>(p);

  // Task 1)
  if (*u8p != static_cast<uint8_t>(SPERR_VERSION_MAJOR))
    return RTNType::VersionMismatch;
  u8p += 1;

  // Task 2)
  auto metabool = sperr::unpack_8_booleans(*u8p);
  u8p += 1;
  if (metabool[1] == true)  // true means it's 3D
    return RTNType::SliceVolumeMismatch;
  const auto has_sperr = metabool[3];

  // Task 3)
  uint32_t dims[2] = {0, 0};
  std::memcpy(dims, u8p, sizeof(dims));
  u8p += sizeof(dims);
  m_dims[0] = dims[0];
  m_dims[1] = dims[1];
  m_dims[2] = 1;
  assert(static_cast<const uint8_t*>(p) + m_meta_size == u8p);

  size_t plen = 0;
  if (len >= m_meta_size)
    plen = len - m_meta_size;
  else
    return RTNType::BitstreamWrongLen;

    // Task 4)
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

  // Task 5)
  const auto condi_size = m_conditioner.header_size(u8p);
  if (condi_size > plen)
    return RTNType::BitstreamWrongLen;
  m_condi_stream.resize(condi_size);
  std::copy(u8p, u8p + condi_size, m_condi_stream.begin());
  u8p += condi_size;
  plen -= condi_size;

  // `m_condi_stream` could indicate that the field is constant, in which case,
  // there is no speck stream anymore.  Let's detect that case and return early.
  // It will be the responsibility of `decompress()` to actually restore the constant field.
  if (m_conditioner.is_constant(m_condi_stream[0])) {
    if (plen == 0)
      return RTNType::Good;
    else
      return RTNType::BitstreamWrongLen;
  }

  const auto speck_size = m_decoder.get_speck_stream_size(u8p);
  if (speck_size > plen)
    return RTNType::BitstreamWrongLen;
  m_speck_stream.resize(speck_size);
  std::copy(u8p, u8p + speck_size, m_speck_stream.begin());
  u8p += speck_size;
  plen -= speck_size;

  if (has_sperr) {
    if (plen == 0)
      return RTNType::BitstreamWrongLen;

    const auto sperr_size = m_sperr.get_sperr_stream_size(u8p);
    if (sperr_size != plen)
      return RTNType::BitstreamWrongLen;
    m_sperr_stream.resize(sperr_size);
    std::copy(u8p, u8p + sperr_size, m_sperr_stream.begin());
  }
  else {
    if (plen != 0)
      return RTNType::BitstreamWrongLen;
  }

  return RTNType::Good;
}

template <typename T>
auto SPERR2D_Decompressor::get_data() const -> sperr::vec_type<T>
{
  auto out_buf = sperr::vec_type<T>(m_val_buf.size());
  std::copy(m_val_buf.cbegin(), m_val_buf.cend(), out_buf.begin());

  return out_buf;
}
template auto SPERR2D_Decompressor::get_data() const -> sperr::vecd_type;
template auto SPERR2D_Decompressor::get_data() const -> sperr::vecf_type;

auto SPERR2D_Decompressor::view_data() const -> const sperr::vecd_type&
{
  return m_val_buf;
}

auto SPERR2D_Decompressor::release_data() -> sperr::vecd_type&&
{
  m_dims = {0, 0, 0};
  return std::move(m_val_buf);
}

auto SPERR2D_Decompressor::get_dims() const -> std::array<size_t, 3>
{
  return m_dims;
}

auto SPERR2D_Decompressor::decompress() -> RTNType
{
  // `m_condi_stream` might be indicating a constant field, so let's test and handle that case.
  if (m_conditioner.is_constant(m_condi_stream[0])) {
    auto rtn = m_conditioner.inverse_condition(m_val_buf, m_dims, m_condi_stream);
    return rtn;
  }

  // Step 1: SPECK decode
  const auto total_vals = m_dims[0] * m_dims[1];
  if (m_speck_stream.empty())
    return RTNType::Error;
  auto rtn = m_decoder.parse_encoded_bitstream(m_speck_stream.data(), m_speck_stream.size());
  if (rtn != RTNType::Good)
    return rtn;
  m_decoder.set_dimensions(m_dims);
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
  m_val_buf = m_cdf.release_data();
  m_conditioner.inverse_condition(m_val_buf, m_dims, m_condi_stream);

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
      m_val_buf[out.location] += out.error;
  }

  return RTNType::Good;
}
