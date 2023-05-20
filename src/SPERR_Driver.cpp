#include "SPERR_Driver.h"

#include <algorithm>
#include <cassert>
#include <cfenv>
#include <cfloat>  // FLT_ROUNDS
#include <cmath>
#include <cstring>

template <typename T>
void sperr::SPERR_Driver::copy_data(const T* p, size_t len)
{
  static_assert(std::is_floating_point<T>::value, "!! Only floating point values are supported !!");

  m_vals_d.resize(len);
  std::copy(p, p + len, m_vals_d.begin());
}
template void sperr::SPERR_Driver::copy_data(const double*, size_t);
template void sperr::SPERR_Driver::copy_data(const float*, size_t);

void sperr::SPERR_Driver::take_data(sperr::vecd_type&& buf)
{
  m_vals_d = std::move(buf);
}

auto sperr::SPERR_Driver::use_bitstream(const void* p, size_t len) -> RTNType
{
  // So let's clean up everything at the very beginning of this routine.
  m_vals_d.clear();
  m_sign_array.clear();
  m_condi_bitstream.fill(0);
  m_speck_bitstream.clear();
  std::visit([](auto& vec) { vec.clear(); }, m_vals_ui);

  const uint8_t* const ptr = static_cast<const uint8_t*>(p);

  // Bitstream parser 1: extract conditioner stream
  const auto condi_size = m_condi_bitstream.size();
  if (condi_size > len)
    return RTNType::BitstreamWrongLen;
  std::copy(ptr, ptr + condi_size, m_condi_bitstream.begin());
  size_t pos = condi_size;

  // `m_condi_bitstream` might be indicating that the field is a constant field.
  //  In that case, there will be no more speck or sperr streams.
  //  Let's detect that case here and return early if it is true.
  //  It will be up to the decompress() routine to restore the actual constant field.
  auto constant = m_conditioner.parse_constant(m_condi_bitstream);
  if (std::get<0>(constant)) {
    if (condi_size == len)
      return RTNType::Good;
    else
      return RTNType::BitstreamWrongLen;
  }

  // Bitstream parser 2: extract SPECK stream from it
  const uint8_t* const speck_p = ptr + pos;
  const auto speck_full_len = std::visit(
      [speck_p](const auto& decoder) { return decoder->get_stream_full_len(speck_p); }, m_decoder);
  if (speck_full_len != len - pos)
    return RTNType::BitstreamWrongLen;
  m_speck_bitstream.resize(speck_full_len);
  std::copy(speck_p, speck_p + speck_full_len, m_speck_bitstream.begin());

  // Integer length decision 1: decide the integer length to use
  const uint32_t num_bitplanes = std::visit(
      [speck_p](const auto& decoder) { return decoder->get_num_bitplanes(speck_p); }, m_decoder);
  if (num_bitplanes <= 8)
    m_uint_flag = UINTType::UINT8;
  else if (num_bitplanes <= 16)
    m_uint_flag = UINTType::UINT16;
  else if (num_bitplanes <= 32)
    m_uint_flag = UINTType::UINT32;
  else
    m_uint_flag = UINTType::UINT64;

  // Integer length decision 2: make sure `m_vals_ui` and `m_decoder` use the decided length
  m_instantiate_int_vec();
  m_instantiate_decoder();

  return RTNType::Good;
}

void sperr::SPERR_Driver::toggle_conditioning(sperr::Conditioner::settings_type settings)
{
  m_conditioning_settings = settings;
}

auto sperr::SPERR_Driver::get_encoded_bitstream() -> vec8_type
{
  const auto total_len = m_condi_bitstream.size() + m_speck_bitstream.size();
  auto tmp = vec8_type(total_len);
  std::copy(m_condi_bitstream.cbegin(), m_condi_bitstream.cend(), tmp.begin());
  std::copy(m_speck_bitstream.cbegin(), m_speck_bitstream.cend(),
            tmp.begin() + m_condi_bitstream.size());
  return tmp;
}

auto sperr::SPERR_Driver::release_decoded_data() -> vecd_type&&
{
  return std::move(m_vals_d);
}

void sperr::SPERR_Driver::set_q(double q)
{
  m_q = std::max(0.0, q);
}

void sperr::SPERR_Driver::set_dims(dims_type dims)
{
  m_dims = dims;
}

void sperr::SPERR_Driver::m_instantiate_int_vec()
{
  switch (m_uint_flag) {
    case UINTType::UINT64:
      if (m_vals_ui.index() != 0)
        m_vals_ui = std::vector<uint64_t>();
      break;
    case UINTType::UINT32:
      if (m_vals_ui.index() != 1)
        m_vals_ui = std::vector<uint32_t>();
      break;
    case UINTType::UINT16:
      if (m_vals_ui.index() != 2)
        m_vals_ui = std::vector<uint16_t>();
      break;
    default:
      if (m_vals_ui.index() != 3)
        m_vals_ui = std::vector<uint8_t>();
  }
}

auto sperr::SPERR_Driver::m_midtread_f2i() -> RTNType
{
  // Make sure that the rounding mode is what we wanted.
  // Here are two methods of querying the current rounding mode; not sure
  //  how they compare, so test both of them for now.
  assert(FE_TONEAREST == FLT_ROUNDS);
  assert(FE_TONEAREST == std::fegetround());

  const auto total_vals = m_vals_d.size();
  const auto q1 = 1.0 / m_q;
  auto vals_ll = std::vector<long long int>(total_vals);
  m_sign_array.resize(total_vals);
  std::feclearexcept(FE_INVALID);
  std::transform(m_vals_d.cbegin(), m_vals_d.cend(), vals_ll.begin(),
                 [q1](auto d) { return std::llrint(d * q1); });
  if (std::fetestexcept(FE_INVALID))
    return RTNType::FE_Invalid;

  // Extract signs from the quantized integers.
  std::transform(vals_ll.cbegin(), vals_ll.cend(), m_sign_array.begin(),
                 [](auto ll) { return ll >= 0; });

  // Decide integer length
  const auto maxmag = *std::max_element(vals_ll.cbegin(), vals_ll.cend(),
                                        [](auto a, auto b) { return std::abs(a) < std::abs(b); });
  if (maxmag <= std::numeric_limits<uint8_t>::max())
    m_uint_flag = UINTType::UINT8;
  else if (maxmag <= std::numeric_limits<uint16_t>::max())
    m_uint_flag = UINTType::UINT16;
  else if (maxmag <= std::numeric_limits<uint32_t>::max())
    m_uint_flag = UINTType::UINT32;
  else
    m_uint_flag = UINTType::UINT64;

  // Use the correct integer length for `m_vals_ui`, and keep the integer magnitude.
  m_instantiate_int_vec();
  std::visit([total_vals](auto& vec) { vec.resize(total_vals); }, m_vals_ui);
  switch (m_uint_flag) {
    case UINTType::UINT64:
      assert(m_vals_ui.index() == 0);
      std::transform(vals_ll.cbegin(), vals_ll.cend(), std::get_if<0>(&m_vals_ui)->begin(),
                     [](auto ll) { return static_cast<uint64_t>(std::abs(ll)); });
      break;
    case UINTType::UINT32:
      assert(m_vals_ui.index() == 1);
      std::transform(vals_ll.cbegin(), vals_ll.cend(), std::get_if<1>(&m_vals_ui)->begin(),
                     [](auto ll) { return static_cast<uint32_t>(std::abs(ll)); });
      break;
    case UINTType::UINT16:
      assert(m_vals_ui.index() == 2);
      std::transform(vals_ll.cbegin(), vals_ll.cend(), std::get_if<2>(&m_vals_ui)->begin(),
                     [](auto ll) { return static_cast<uint16_t>(std::abs(ll)); });
      break;
    default:
      assert(m_vals_ui.index() == 3);
      std::transform(vals_ll.cbegin(), vals_ll.cend(), std::get_if<3>(&m_vals_ui)->begin(),
                     [](auto ll) { return static_cast<uint8_t>(std::abs(ll)); });
  }

  return RTNType::Good;
}

void sperr::SPERR_Driver::m_midtread_i2f()
{
  assert(m_sign_array.size() == std::visit([](auto& vec) { return vec.size(); }, m_vals_ui));

  const auto tmpd = std::array<double, 2>{-1.0, 1.0};
  m_vals_d.resize(m_sign_array.size());
  std::visit(
      [&vals_d = m_vals_d, &sign_array = m_sign_array, tmpd, q = m_q](auto& vec) {
        std::transform(
            vec.cbegin(), vec.cend(), sign_array.cbegin(), vals_d.begin(),
            [tmpd, q](auto i, auto b) { return (q * static_cast<double>(i) * tmpd[b]); });
      },
      m_vals_ui);

  // std::visit() obscures the intention, but the task is really the same as the following:
  //
  // std::transform(m_vals_ui.cbegin(), m_vals_ui.cend(), m_sign_array.cbegin(), m_vals_d.begin(),
  //                [tmpd, q](auto i, auto b) { return q * static_cast<double>(i) * tmpd[b]; });
}

auto sperr::SPERR_Driver::compress() -> RTNType
{
  const auto total_vals = uint64_t(m_dims[0]) * m_dims[1] * m_dims[2];
  if (m_vals_d.empty() || m_vals_d.size() != total_vals)
    return RTNType::Error;

  m_condi_bitstream.fill(0);
  m_speck_bitstream.clear();

  // Believe it or not, there are constant fields passed in for compression!
  // Let's detect that case and skip the rest of the compression routine if it occurs.
  auto constant = m_conditioner.test_constant(m_vals_d);
  if (constant.first) {
    m_condi_bitstream = constant.second;
    return RTNType::Good;
  }

  // Step 1: data goes through the conditioner
  m_conditioner.toggle_all_settings(m_conditioning_settings);
  auto [rtn, condi_meta] = m_conditioner.condition(m_vals_d);
  if (rtn != RTNType::Good)
    return rtn;
  m_condi_bitstream = condi_meta;

  // Step 2: wavelet transform
  rtn = m_cdf.take_data(std::move(m_vals_d), m_dims);
  if (rtn != RTNType::Good)
    return rtn;
  m_wavelet_xform();
  m_vals_d = m_cdf.release_data();

  // Step 3: quantize floating-point coefficients to integers
  rtn = m_quantize();
  if (rtn != RTNType::Good)
    return rtn;

  // Step 4: Integer SPECK encoding
  m_instantiate_encoder();
  std::visit([&dims = m_dims](auto& encoder) { encoder->set_dims(dims); }, m_encoder);
  switch (m_uint_flag) {
    case UINTType::UINT64:
      assert(m_vals_ui.index() == 0);
      assert(m_encoder.index() == 0);
      std::get<0>(m_encoder)->use_coeffs(std::move(std::get<0>(m_vals_ui)),
                                         std::move(m_sign_array));
      break;
    case UINTType::UINT32:
      assert(m_vals_ui.index() == 1);
      assert(m_encoder.index() == 1);
      std::get<1>(m_encoder)->use_coeffs(std::move(std::get<1>(m_vals_ui)),
                                         std::move(m_sign_array));
      break;
    case UINTType::UINT16:
      assert(m_vals_ui.index() == 2);
      assert(m_encoder.index() == 2);
      std::get<2>(m_encoder)->use_coeffs(std::move(std::get<2>(m_vals_ui)),
                                         std::move(m_sign_array));
      break;
    default:
      assert(m_vals_ui.index() == 3);
      assert(m_encoder.index() == 3);
      std::get<3>(m_encoder)->use_coeffs(std::move(std::get<3>(m_vals_ui)),
                                         std::move(m_sign_array));
  }
  std::visit([](auto& encoder) { encoder->encode(); }, m_encoder);
  std::visit([&speck_bitstream = m_speck_bitstream](
                 auto& encoder) { encoder->write_encoded_bitstream(speck_bitstream); },
             m_encoder);

  return RTNType::Good;
}

auto sperr::SPERR_Driver::decompress() -> RTNType
{
  m_vals_d.clear();
  std::visit([](auto& vec) { vec.clear(); }, m_vals_ui);
  m_sign_array.clear();
  const auto total_vals = uint64_t(m_dims[0]) * m_dims[1] * m_dims[2];

  // `m_condi_bitstream` might be indicating a constant field, so let's see if that's
  //  the case, and if it is, we don't need to go through wavelet and speck stuff anymore.
  auto constant = m_conditioner.parse_constant(m_condi_bitstream);
  if (std::get<0>(constant)) {
    auto val = std::get<1>(constant);
    auto nval = std::get<2>(constant);
    m_vals_d.assign(nval, val);
    return RTNType::Good;
  }

  // Step 1: Integer SPECK decode.
  assert(!m_speck_bitstream.empty());
  std::visit([&dims = m_dims](auto& decoder) { decoder->set_dims(dims); }, m_decoder);
  std::visit([&speck_bitstream =
                  m_speck_bitstream](auto& decoder) { decoder->use_bitstream(speck_bitstream); },
             m_decoder);
  std::visit([](auto& decoder) { decoder->decode(); }, m_decoder);
  switch (m_uint_flag) {  // `m_uint_flag` was properly set during `use_bitstream()`.
    case UINTType::UINT64:
      assert(m_decoder.index() == 0);
      m_vals_ui = std::get<0>(m_decoder)->release_coeffs();
      break;
    case UINTType::UINT32:
      assert(m_decoder.index() == 1);
      m_vals_ui = std::get<1>(m_decoder)->release_coeffs();
      break;
    case UINTType::UINT16:
      assert(m_decoder.index() == 2);
      m_vals_ui = std::get<2>(m_decoder)->release_coeffs();
      break;
    default:
      assert(m_decoder.index() == 3);
      m_vals_ui = std::get<3>(m_decoder)->release_coeffs();
      break;
  }
  m_sign_array = std::visit([](auto& decoder) { return decoder->release_signs(); }, m_decoder);

  // Step 2: Inverse quantization
  auto rtn = m_inverse_quantize();
  if (rtn != RTNType::Good)
    return rtn;

  // Step 3: Inverse wavelet transform
  rtn = m_cdf.take_data(std::move(m_vals_d), m_dims);
  if (rtn != RTNType::Good)
    return rtn;
  m_inverse_wavelet_xform();
  m_vals_d = m_cdf.release_data();

  // Step 4: Inverse Conditioning
  rtn = m_conditioner.inverse_condition(m_vals_d, m_condi_bitstream);
  if (rtn != RTNType::Good)
    return rtn;

  return RTNType::Good;
}