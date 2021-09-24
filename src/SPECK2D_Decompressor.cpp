#include "SPECK2D_Decompressor.h"

#include <cassert>
#include <cstring>

#ifdef USE_ZSTD
#include "zstd.h"
#endif

auto SPECK2D_Decompressor::use_bitstream(const void* p, size_t len) -> RTNType
{
  // This method parses the metadata of a bitstream and performs the following
  // tasks: 1) Verify if the major version number is consistant. 2) Make sure if
  // the bitstream is for 2D encoding. 3) Verify that the application of ZSTD is
  // consistant, and apply ZSTD if needed. 4) disassemble conditioner and SPECK
  // streams. 5) Extract dimensions from the SPECK stream.

  assert(len > m_meta_size);

  uint8_t meta[2];
  assert(sizeof(meta) == m_meta_size);
  std::memcpy(meta, p, sizeof(meta));
  auto metabool = sperr::unpack_8_booleans(meta[1]);

  // Task 1)
  if (meta[0] != uint8_t(SPERR_VERSION_MAJOR))
    return RTNType::VersionMismatch;

  // Task 2)
  if (metabool[1] != false)  // false means it's 2D
    return RTNType::SliceVolumeMismatch;

  size_t loc = m_meta_size;
  const uint8_t* u8p = static_cast<const uint8_t*>(p) + loc;
  size_t plen = len - m_meta_size;  // NOLINT

  // Task 3)
#ifdef USE_ZSTD
  if (metabool[0] == false)
    return RTNType::ZSTDMismatch;

  const auto content_size = ZSTD_getFrameContentSize(u8p, len - m_meta_size);
  if (content_size == ZSTD_CONTENTSIZE_ERROR || content_size == ZSTD_CONTENTSIZE_UNKNOWN)
    return RTNType::ZSTDError;

  auto content_buf = std::make_unique<uint8_t[]>(content_size);
  const auto decomp_size = ZSTD_decompress(content_buf.get(), content_size, u8p, len - m_meta_size);
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
  m_condi_stream.fill(0);
  const auto condi_size = m_condi_stream.size();
  if (condi_size > plen)
    return RTNType::WrongSize;
  std::copy(u8p, u8p + condi_size, m_condi_stream.begin());
  u8p += condi_size;
  plen -= condi_size;

  m_speck_stream.clear();
  const auto speck_size = m_decoder.get_speck_stream_size(u8p);
  if (speck_size > plen)
    return RTNType::WrongSize;
  m_speck_stream.resize(speck_size, 0);
  std::copy(u8p, u8p + speck_size, m_speck_stream.begin());
  u8p += speck_size;   // NOLINT
  plen -= speck_size;  // NOLINT
  assert(plen == 0);   // True if no other streams behind SPECK stream

  // Task 5)
  m_dims = m_decoder.get_speck_stream_dims(m_speck_stream.data());
  if (m_dims[2] != 1)
    return RTNType::SliceVolumeMismatch;

  return RTNType::Good;
}

auto SPECK2D_Decompressor::set_bpp(float bpp) -> RTNType
{
  if (bpp < 0.0 || bpp > 64.0)
    return RTNType::InvalidParam;
  else {
    m_bpp = bpp;
    return RTNType::Good;
  }
}

template <typename T>
auto SPECK2D_Decompressor::get_data() const -> std::vector<T>
{
  auto out_buf = std::vector<T>(m_val_buf.size());
  std::copy(m_val_buf.begin(), m_val_buf.end(), out_buf.begin());

  return std::move(out_buf);
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
  // Step 1: SPECK decode
  if (m_speck_stream.empty())
    return RTNType::Error;

  auto rtn = m_decoder.parse_encoded_bitstream(m_speck_stream.data(), m_speck_stream.size());
  if (rtn != RTNType::Good)
    return rtn;

  auto total_vals = m_dims[0] * m_dims[1];
  m_decoder.set_bit_budget(size_t(m_bpp * total_vals));
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

  m_val_buf = std::move(cdf_out);

  return RTNType::Good;
}
