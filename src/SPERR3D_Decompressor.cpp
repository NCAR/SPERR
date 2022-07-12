#include "SPERR3D_Decompressor.h"

#include <cassert>
#include <cstring>

auto sperr::SPERR3D_Decompressor::use_bitstream(const void* p, size_t len) -> RTNType
{
  // It'd be bad to have some buffers updated, and some others not.
  // So let's clean up everything at the very beginning of this routine
  m_condi_stream.fill(0);
  m_speck_stream.clear();
  m_val_buf.clear();
  m_sperr_stream.clear();

#ifdef USE_ZSTD
  // Make sure that we have a ZSTD Decompression Context first
  if (m_dctx == nullptr) {
    auto* ctx_p = ZSTD_createDCtx();
    if (ctx_p == nullptr)
      return RTNType::ZSTDError;
    else
      m_dctx.reset(ctx_p);
  }

  const size_t content_size = ZSTD_getFrameContentSize(p, len);
  if (content_size == ZSTD_CONTENTSIZE_ERROR || content_size == ZSTD_CONTENTSIZE_UNKNOWN)
    return RTNType::ZSTDError;

  m_zstd_buf.resize(content_size);

  auto decomp_size =
      ZSTD_decompressDCtx(m_dctx.get(), m_zstd_buf.data(), m_zstd_buf.size(), p, len);
  if (ZSTD_isError(decomp_size) || decomp_size != content_size)
    return RTNType::ZSTDError;
  const uint8_t* const ptr = m_zstd_buf.data();
  const size_t ptr_len = m_zstd_buf.size();
#else
  const uint8_t* const ptr = static_cast<const uint8_t*>(p);
  const size_t ptr_len = len;
#endif

  // Step 1: extract conditioner stream from it
  const auto condi_size = m_condi_stream.size();
  if (condi_size > ptr_len)
    return RTNType::BitstreamWrongLen;
  std::copy(ptr, ptr + condi_size, m_condi_stream.begin());
  size_t pos = condi_size;

  // `m_condi_stream` might be indicating that the field is a constant field.
  // In that case, there will be no more speck or sperr streams.
  // Let's detect that case here and return early if it is true.
  // It will be up to the decompress() routine to restore the actual constant field.
  auto constant = m_conditioner.parse_constant(m_condi_stream);
  if (std::get<0>(constant)) {
    if (condi_size == ptr_len)
      return RTNType::Good;
    else
      return RTNType::BitstreamWrongLen;
  }

  // Step 2: extract SPECK stream from it
  const uint8_t* const speck_p = ptr + pos;
  const auto speck_size = m_decoder.get_speck_stream_size(speck_p);
  if (pos + speck_size > ptr_len)
    return RTNType::BitstreamWrongLen;
  m_speck_stream.resize(speck_size);
  std::copy(speck_p, speck_p + speck_size, m_speck_stream.begin());
  pos += speck_size;

  // Step 3: extract SPERR stream
  if (pos < ptr_len) {
    const uint8_t* const sperr_p = ptr + pos;
    const auto sperr_size = m_sperr.get_sperr_stream_size(sperr_p);
    if (pos + sperr_size != ptr_len)
      return RTNType::BitstreamWrongLen;
    m_sperr_stream.resize(sperr_size);
    std::copy(sperr_p, sperr_p + sperr_size, m_sperr_stream.begin());
  }

  return RTNType::Good;
}

void sperr::SPERR3D_Decompressor::set_dims(dims_type dims)
{
  m_dims = dims;
}

auto sperr::SPERR3D_Decompressor::decompress() -> RTNType
{
  // `m_condi_stream` might be indicating a constant field, so let's see if that's
  // the case, and if it is, we don't need to go through dwt and speck stuff anymore.
  auto constant = m_conditioner.parse_constant(m_condi_stream);
  if (std::get<0>(constant)) {
    auto val = std::get<1>(constant);
    auto nval = std::get<2>(constant);
    m_val_buf.assign(nval, val);
    return RTNType::Good;
  }

  // Step 1: SPECK decode.
  if (m_speck_stream.empty())
    return RTNType::Error;

  auto rtn = m_decoder.parse_encoded_bitstream(m_speck_stream.data(), m_speck_stream.size());
  if (rtn != RTNType::Good)
    return rtn;
  m_decoder.set_dimensions(m_dims);
  rtn = m_decoder.decode();
  if (rtn != RTNType::Good)
    return rtn;

  // Step 2: Inverse Wavelet Transform
  //
  // (Here we ask `m_cdf` to make a copy of coefficients from `m_decoder` instead of
  //  transfer the ownership, because `m_decoder` will reuse that memory when
  //  processing the next chunk. For the same reason, `m_cdf` keeps its memory.)
  const auto& decoder_out = m_decoder.view_data();
  m_cdf.copy_data(decoder_out.data(), decoder_out.size(), m_dims);

  // Figure out which dwt3d strategy to use.
  // Note: this strategy needs to be consistent with SPERR3D_Compressor.
  auto xforms_xy = sperr::num_of_xforms(std::min(m_dims[0], m_dims[1]));
  auto xforms_z = sperr::num_of_xforms(m_dims[2]);
  if (xforms_xy == xforms_z)
    m_cdf.idwt3d_dyadic();
  else
    m_cdf.idwt3d_wavelet_packet();

  // Step 3: Inverse Conditioning
  const auto& cdf_out = m_cdf.view_data();
  m_val_buf.resize(cdf_out.size());
  std::copy(cdf_out.begin(), cdf_out.end(), m_val_buf.begin());
  m_conditioner.inverse_condition(m_val_buf, m_condi_stream);

  // Step 4: If there's SPERR data, then do the correction.
  if (!m_sperr_stream.empty()) {
    rtn = m_sperr.parse_encoded_bitstream(m_sperr_stream.data(), m_sperr_stream.size());
    if (rtn != RTNType::Good)
      return rtn;
    rtn = m_sperr.decode();
    if (rtn != RTNType::Good)
      return rtn;

    const auto& los = m_sperr.view_outliers();
    for (const auto& outlier : los)
      m_val_buf[outlier.location] += outlier.error;
  }

  return RTNType::Good;
}

template <typename T>
auto sperr::SPERR3D_Decompressor::get_data() const -> std::vector<T>
{
  auto out_buf = std::vector<T>(m_val_buf.size());
  std::copy(m_val_buf.begin(), m_val_buf.end(), out_buf.begin());

  return out_buf;
}
template auto sperr::SPERR3D_Decompressor::get_data() const -> std::vector<double>;
template auto sperr::SPERR3D_Decompressor::get_data() const -> std::vector<float>;

auto sperr::SPERR3D_Decompressor::view_data() const -> const std::vector<double>&
{
  return m_val_buf;
}

auto sperr::SPERR3D_Decompressor::release_data() -> std::vector<double>&&
{
  m_dims = {0, 0, 0};
  return std::move(m_val_buf);
}
