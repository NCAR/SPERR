#include "SPECK_FLT.h"

#include <algorithm>
#include <cassert>
#include <cfenv>
#include <cfloat>  // FLT_ROUNDS
#include <cmath>
#include <cstring>
#include <numeric>

template <typename T>
void sperr::SPECK_FLT::copy_data(const T* p, size_t len)
{
  static_assert(std::is_floating_point<T>::value, "!! Only floating point values are supported !!");

  m_vals_d.resize(len);
  std::copy(p, p + len, m_vals_d.begin());
}
template void sperr::SPECK_FLT::copy_data(const double*, size_t);
template void sperr::SPECK_FLT::copy_data(const float*, size_t);

void sperr::SPECK_FLT::take_data(sperr::vecd_type&& buf)
{
  m_vals_d = std::move(buf);
}

auto sperr::SPECK_FLT::use_bitstream(const void* p, size_t len) -> RTNType
{
  // So let's clean up everything at the very beginning of this routine.
  m_vals_d.clear();
  m_sign_array.clear();
  std::visit([](auto&& vec) { vec.clear(); }, m_vals_ui);
  m_q = 0.0;
  m_has_outlier = false;
  m_quality = 0.0;        // doesn't matter in decompression
  m_is_pwe_mode = false;  // doesn't matter in decompression

  const auto* const ptr = static_cast<const uint8_t*>(p);

  // Bitstream parser 1: extract conditioner stream
  if (len < m_condi_bitstream.size())
    return RTNType::BitstreamWrongLen;
  std::copy(ptr, ptr + m_condi_bitstream.size(), m_condi_bitstream.begin());

  // `m_condi_bitstream` might be indicating that the field is a constant field.
  //    In that case, there will be no more speck or sperr streams.
  //    Let's detect that case here and return early if it is true.
  //    It will be up to the decompress() routine to restore the actual constant field.
  if (m_conditioner.is_constant(m_condi_bitstream[0])) {
    if (len == m_condi_bitstream.size())
      return RTNType::Good;
    else
      return RTNType::BitstreamWrongLen;
  }
  else {
    m_q = m_conditioner.retrieve_q(m_condi_bitstream);
    assert(m_q > 0.0);
  }

  // Bitstream parser 2.1: based on the number of bitplanes, decide on an integer length to use,
  // and instantiate the proper decoder. It will be the decoder who parses the SPECK bitstream.
  size_t pos = m_condi_bitstream.size();
  size_t remaining_len = len - pos;
  assert(remaining_len >= SPECK_INT<uint8_t>::header_size);
  const uint8_t* const speck_p = ptr + pos;
  const auto num_bitplanes = speck_int_get_num_bitplanes(speck_p);
  if (num_bitplanes <= 8)
    m_uint_flag = UINTType::UINT8;
  else if (num_bitplanes <= 16)
    m_uint_flag = UINTType::UINT16;
  else if (num_bitplanes <= 32)
    m_uint_flag = UINTType::UINT32;
  else
    m_uint_flag = UINTType::UINT64;

  m_instantiate_int_vec();
  m_instantiate_decoder();

  // Bitstream parser 2.2: extract and parse SPECK stream.
  //    A situation to be considered here is that the speck bitstream is only partially available
  //    as the result of progressive access. In that case, the available speck stream is simply
  //    shorter than what the header reports.
  auto speck_suppose_len =
      std::visit([speck_p](auto&& dec) { return dec->get_stream_full_len(speck_p); }, m_decoder);
  auto speck_len = std::min(speck_suppose_len, remaining_len);
  std::visit([speck_p, speck_len](auto&& dec) { return dec->use_bitstream(speck_p, speck_len); },
             m_decoder);
  pos += speck_len;
  assert(pos <= len);

  // Bitstream parser 3: extract Outlier Coder stream if there's any.
  if (pos < len) {
    m_has_outlier = true;
    const uint8_t* const out_p = ptr + pos;
    assert(m_out_coder.get_stream_full_len(out_p) == len - pos);
    auto rtn = m_out_coder.use_bitstream(out_p, len - pos);
    if (rtn != RTNType::Good)
      return rtn;
  }
  else
    m_has_outlier = false;

  return RTNType::Good;
}

void sperr::SPECK_FLT::append_encoded_bitstream(vec8_type& buf) const
{
  // Append `m_condi_bitstream` no matter what.
  std::copy(m_condi_bitstream.cbegin(), m_condi_bitstream.cend(), std::back_inserter(buf));

  if (!m_conditioner.is_constant(m_condi_bitstream[0])) {
    // Append SPECK_INT bitstream.
    std::visit([&buf](auto&& enc) { enc->append_encoded_bitstream(buf); }, m_encoder);

    // Append outlier coder bitstream.
    if (m_has_outlier)
      m_out_coder.append_encoded_bitstream(buf);
  }
}

auto sperr::SPECK_FLT::release_decoded_data() -> vecd_type&&
{
  return std::move(m_vals_d);
}

auto sperr::SPECK_FLT::view_decoded_data() const -> const vecd_type&
{
  return m_vals_d;
}

void sperr::SPECK_FLT::set_psnr(double psnr)
{
  assert(psnr > 0.0);
  m_quality = psnr;
  m_is_pwe_mode = false;

  m_q = 0.0;  // The real m_q needs to be calculated later
  m_data_range = 0.0;
  m_has_outlier = false;
}

void sperr::SPECK_FLT::set_tolerance(double tol)
{
  assert(tol > 0.0);
  m_quality = tol;
  m_is_pwe_mode = true;

  m_q = 0.0;  // Though `m_q` is simple in this case, calculate it the same place as in PSNR mode.
  m_has_outlier = false;
  m_data_range = 0.0;
}

void sperr::SPECK_FLT::set_dims(dims_type dims)
{
  m_dims = dims;
}

auto sperr::SPECK_FLT::integer_len() const -> size_t
{
  switch (m_uint_flag) {
    case UINTType::UINT8:
      assert(m_vals_ui.index() == 0);
      // Either this is an encoder, or this is a decoder.
      assert(m_encoder.index() == 0 || m_decoder.index() == 0);
      return sizeof(uint8_t);
    case UINTType::UINT16:
      assert(m_vals_ui.index() == 1);
      assert(m_encoder.index() == 1 || m_decoder.index() == 1);
      return sizeof(uint16_t);
    case UINTType::UINT32:
      assert(m_vals_ui.index() == 2);
      assert(m_encoder.index() == 2 || m_decoder.index() == 2);
      return sizeof(uint32_t);
    default:
      assert(m_vals_ui.index() == 3);
      assert(m_encoder.index() == 3 || m_decoder.index() == 3);
      return sizeof(uint64_t);
  }
}

void sperr::SPECK_FLT::m_instantiate_int_vec()
{
  switch (m_uint_flag) {
    case UINTType::UINT8:
      if (m_vals_ui.index() != 0)
        m_vals_ui = std::vector<uint8_t>();
      break;
    case UINTType::UINT16:
      if (m_vals_ui.index() != 1)
        m_vals_ui = std::vector<uint16_t>();
      break;
    case UINTType::UINT32:
      if (m_vals_ui.index() != 2)
        m_vals_ui = std::vector<uint32_t>();
      break;
    default:
      if (m_vals_ui.index() != 3)
        m_vals_ui = std::vector<uint64_t>();
  }
}

auto sperr::SPECK_FLT::m_estimate_mse_midtread(double q) const -> double
{
  assert(!m_vals_d.empty());

  const auto len = m_vals_d.size();
  const size_t stride_size = 4096;
  const size_t num_strides = len / stride_size;
  auto tmp_buf = vecd_type(num_strides + 1);

  for (size_t i = 0; i < num_strides; i++) {
    const auto beg = m_vals_d.cbegin() + i * stride_size;
    tmp_buf[i] = std::accumulate(beg, beg + stride_size, 0.0, [q](auto init, auto v) {
      auto diff = std::remainder(v, q);
      return init + diff * diff;
    });
  }

  // Let's also process the last stride.
  tmp_buf[num_strides] = 0.0;
  tmp_buf[num_strides] = std::accumulate(m_vals_d.cbegin() + num_strides * stride_size,
                                         m_vals_d.cend(), 0.0, [q](auto init, auto v) {
                                           auto diff = std::remainder(v, q);
                                           return init + diff * diff;
                                         });
  const auto total_sum = std::accumulate(tmp_buf.cbegin(), tmp_buf.cend(), 0.0);
  const auto mse = total_sum / static_cast<double>(len);

  return mse;
}

auto sperr::SPECK_FLT::m_estimate_q() const -> double
{
  // Note: based on Peter's estimation method, to achieved the target PSNR, the terminal
  //       quantization threshold should be (2.0 * sqrt(3.0) * rmse).
  assert(m_data_range > 0.0);
  assert(!m_is_pwe_mode);
  const auto t_mse = (m_data_range * m_data_range) * std::pow(10.0, -m_quality / 10.0);
  const auto t_rmse = std::sqrt(t_mse);
  auto q = 2.0 * std::sqrt(t_mse * 3.0);
  while (m_estimate_mse_midtread(q) > t_mse)
    q /= std::pow(2.0, 0.25);  // Four adjustments would effectively halve q.
  return q;
}

auto sperr::SPECK_FLT::m_midtread_quantize() -> RTNType
{
  // Make sure that the rounding mode is what we wanted.
  // Here are two methods of querying the current rounding mode; not sure
  //    how they compare, so test both of them for now.
  std::fesetround(FE_TONEAREST);
  assert(FE_TONEAREST == std::fegetround());
  assert(FLT_ROUNDS == 1);

  // Find the biggest floating point value, then get its quantized integer.
  auto maxd = *std::max_element(m_vals_d.cbegin(), m_vals_d.cend(),
                                [](auto a, auto b) { return std::abs(a) < std::abs(b); });
  std::feclearexcept(FE_INVALID);
  assert(m_q > 0.0);
  auto maxll = std::llrint(std::abs(maxd) / m_q);
  if (std::fetestexcept(FE_INVALID))
    return RTNType::FE_Invalid;

  // Decide integer length, and instantiate `m_vals_ui`.
  if (maxll <= std::numeric_limits<uint8_t>::max())
    m_uint_flag = UINTType::UINT8;
  else if (maxll <= std::numeric_limits<uint16_t>::max())
    m_uint_flag = UINTType::UINT16;
  else if (maxll <= std::numeric_limits<uint32_t>::max())
    m_uint_flag = UINTType::UINT32;
  else
    m_uint_flag = UINTType::UINT64;

  m_instantiate_int_vec();
  const auto total_vals = m_vals_d.size();
  std::visit([total_vals](auto&& vec) { vec.resize(total_vals); }, m_vals_ui);
  m_sign_array.resize(total_vals);

  // The following switch{} block functions the same as the following:
  //
  //    for (size_t i = 0; i < total_vals; i++) {
  //      auto ll = std::llrint(m_vals_d[i] / m_q);
  //      m_sign_array[i] = (ll >= 0);
  //      std::visit([i, ll](auto&& vec) { vec[i] = (std::abs(ll)); }, m_vals_ui);
  //    }
  //
  // but it's just not very efficient to put std::visit() inside of a pretty big loop,
  // so we use the switch block.

  switch (m_uint_flag) {
    case UINTType::UINT8: {
      auto& vec = std::get<0>(m_vals_ui);
      for (size_t i = 0; i < total_vals; i++) {
        auto ll = std::llrint(m_vals_d[i] / m_q);
        m_sign_array[i] = (ll >= 0);
        vec[i] = static_cast<uint8_t>(std::abs(ll));
      }
      break;
    }
    case UINTType::UINT16: {
      auto& vec = std::get<1>(m_vals_ui);
      for (size_t i = 0; i < total_vals; i++) {
        auto ll = std::llrint(m_vals_d[i] / m_q);
        m_sign_array[i] = (ll >= 0);
        vec[i] = static_cast<uint16_t>(std::abs(ll));
      }
      break;
    }
    case UINTType::UINT32: {
      auto& vec = std::get<2>(m_vals_ui);
      for (size_t i = 0; i < total_vals; i++) {
        auto ll = std::llrint(m_vals_d[i] / m_q);
        m_sign_array[i] = (ll >= 0);
        vec[i] = static_cast<uint32_t>(std::abs(ll));
      }
      break;
    }
    default: {
      auto& vec = std::get<3>(m_vals_ui);
      for (size_t i = 0; i < total_vals; i++) {
        auto ll = std::llrint(m_vals_d[i] / m_q);
        m_sign_array[i] = (ll >= 0);
        vec[i] = static_cast<uint64_t>(std::abs(ll));
      }
    }
  }

  return RTNType::Good;
}

void sperr::SPECK_FLT::m_midtread_inv_quantize()
{
  assert(m_sign_array.size() == std::visit([](auto&& vec) { return vec.size(); }, m_vals_ui));
  assert(m_q > 0.0);

  const auto tmpd = std::array<double, 2>{-1.0, 1.0};
  m_vals_d.resize(m_sign_array.size());
  std::visit(
      [&vals_d = m_vals_d, &signs = m_sign_array, q = m_q, tmpd](auto&& vec) {
        std::transform(
            vec.cbegin(), vec.cend(), signs.cbegin(), vals_d.begin(),
            [tmpd, q](auto i, auto b) { return (q * static_cast<double>(i) * tmpd[b]); });
      },
      m_vals_ui);
}

auto sperr::SPECK_FLT::compress() -> RTNType
{
  const auto total_vals = uint64_t(m_dims[0]) * m_dims[1] * m_dims[2];
  if (m_vals_d.empty() || m_vals_d.size() != total_vals)
    return RTNType::Error;

  m_has_outlier = false;

  // Step 1: data goes through the conditioner
  //    Believe it or not, there are constant fields passed in for compression!
  //    Let's detect that case and skip the rest of the compression routine if it occurs.
  m_condi_bitstream = m_conditioner.condition(m_vals_d, m_dims);
  if (m_conditioner.is_constant(m_condi_bitstream[0]))
    return RTNType::Good;

  // Side step for outlier coding, or `m_q` calculation based on PSNR.
  if (m_is_pwe_mode) {
    m_vals_orig.resize(total_vals);
    std::copy(m_vals_d.cbegin(), m_vals_d.cend(), m_vals_orig.begin());
  }
  else {
    auto [min, max] = std::minmax_element(m_vals_d.cbegin(), m_vals_d.cend());
    m_data_range = *max - *min;
  }

  // Step 2: wavelet transform
  m_cdf.take_data(std::move(m_vals_d), m_dims);
  m_wavelet_xform();
  m_vals_d = m_cdf.release_data();

  // Calculate `m_q`, and store it as part of `m_condi_stream`.
  if (m_is_pwe_mode)
    m_q = m_quality * 1.5;
  else
    m_q = m_estimate_q();
  assert(m_q > 0.0);
  m_conditioner.save_q(m_condi_bitstream, m_q);

  // Step 3: quantize floating-point coefficients to integers
  auto rtn = m_midtread_quantize();
  if (rtn != RTNType::Good)
    return rtn;

  // Side step for outlier coding: find out all the outliers, and encode them!
  if (m_is_pwe_mode) {
    m_midtread_inv_quantize();
    rtn = m_cdf.take_data(std::move(m_vals_d), m_dims);
    if (rtn != RTNType::Good)
      return rtn;
    m_inverse_wavelet_xform();
    m_vals_d = m_cdf.release_data();
    auto LOS = std::vector<Outlier>();
    LOS.reserve(0.04 * total_vals);  // Reserve space to hold about 4% of total values.
    for (size_t i = 0; i < total_vals; i++) {
      auto diff = m_vals_orig[i] - m_vals_d[i];
      if (std::abs(diff) > m_quality)
        LOS.emplace_back(i, diff);
    }
    if (LOS.empty())
      m_has_outlier = false;
    else {
      m_has_outlier = true;
      m_out_coder.set_length(total_vals);
      m_out_coder.set_tolerance(m_quality);
      m_out_coder.use_outlier_list(std::move(LOS));
      rtn = m_out_coder.encode();
      if (rtn != RTNType::Good)
        return rtn;
    }
  }

  // Step 4: Integer SPECK encoding
  m_instantiate_encoder();
  std::visit([&dims = m_dims](auto&& encoder) { encoder->set_dims(dims); }, m_encoder);
  switch (m_uint_flag) {
    case UINTType::UINT8:
      assert(m_vals_ui.index() == 0);
      assert(m_encoder.index() == 0);
      rtn = std::get<0>(m_encoder)->use_coeffs(std::move(std::get<0>(m_vals_ui)),
                                               std::move(m_sign_array));
      break;
    case UINTType::UINT16:
      assert(m_vals_ui.index() == 1);
      assert(m_encoder.index() == 1);
      rtn = std::get<1>(m_encoder)->use_coeffs(std::move(std::get<1>(m_vals_ui)),
                                               std::move(m_sign_array));
      break;
    case UINTType::UINT32:
      assert(m_vals_ui.index() == 2);
      assert(m_encoder.index() == 2);
      rtn = std::get<2>(m_encoder)->use_coeffs(std::move(std::get<2>(m_vals_ui)),
                                               std::move(m_sign_array));
      break;
    default:
      assert(m_vals_ui.index() == 3);
      assert(m_encoder.index() == 3);
      rtn = std::get<3>(m_encoder)->use_coeffs(std::move(std::get<3>(m_vals_ui)),
                                               std::move(m_sign_array));
  }
  if (rtn != RTNType::Good)
    return rtn;

  std::visit([](auto&& encoder) { encoder->encode(); }, m_encoder);

  return RTNType::Good;
}

auto sperr::SPECK_FLT::decompress() -> RTNType
{
  m_vals_d.clear();
  std::visit([](auto&& vec) { vec.clear(); }, m_vals_ui);
  m_sign_array.clear();

  // `m_condi_bitstream` might be indicating a constant field, so let's see if that's
  // the case, and if it is, we don't need to go through wavelet and speck stuff anymore.
  if (m_conditioner.is_constant(m_condi_bitstream[0])) {
    auto rtn = m_conditioner.inverse_condition(m_vals_d, m_dims, m_condi_bitstream);
    return rtn;
  }

  // Step 1: Integer SPECK decode.
  // Note: the decoder has already parsed the bitstream in function `use_bitstream()`.
  assert(m_q > 0.0);
  std::visit([&dims = m_dims](auto&& decoder) { decoder->set_dims(dims); }, m_decoder);
  std::visit([](auto&& decoder) { decoder->decode(); }, m_decoder);
  std::visit([&vec = m_vals_ui](auto&& dec) { vec = dec->release_coeffs(); }, m_decoder);
  m_sign_array = std::visit([](auto&& dec) { return dec->release_signs(); }, m_decoder);

  // Step 2: Inverse quantization
  m_midtread_inv_quantize();

  // Step 3: Inverse wavelet transform
  auto rtn = m_cdf.take_data(std::move(m_vals_d), m_dims);
  if (rtn != RTNType::Good)
    return rtn;
  m_inverse_wavelet_xform();
  m_vals_d = m_cdf.release_data();

  // Side step: outlier correction, if needed
  if (m_has_outlier) {
    m_out_coder.set_length(m_dims[0] * m_dims[1] * m_dims[2]);
    m_out_coder.set_tolerance(m_q / 1.5);  // `m_quality` is not set during decompression.
    rtn = m_out_coder.decode();
    if (rtn != RTNType::Good)
      return rtn;
    const auto& recovered = m_out_coder.view_outlier_list();
    for (auto out : recovered)
      m_vals_d[out.pos] += out.err;
  }

  // Step 4: Inverse Conditioning
  rtn = m_conditioner.inverse_condition(m_vals_d, m_dims, m_condi_bitstream);
  if (rtn != RTNType::Good)
    return rtn;

  return RTNType::Good;
}
