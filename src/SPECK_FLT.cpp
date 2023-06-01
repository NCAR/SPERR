#include "SPECK_FLT.h"

#include <algorithm>
#include <cassert>
#include <cfenv>
#include <cfloat>  // FLT_ROUNDS
#include <cmath>
#include <cstring>

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
  m_condi_bitstream.clear();
  std::visit([](auto&& vec) { vec.clear(); }, m_vals_ui);

  const uint8_t* const ptr = static_cast<const uint8_t*>(p);

  // Bitstream parser 1: extract conditioner stream
  const auto condi_size = m_conditioner.header_size(ptr);
  if (condi_size > len)
    return RTNType::BitstreamWrongLen;
  m_condi_bitstream.resize(condi_size);
  std::copy(ptr, ptr + condi_size, m_condi_bitstream.begin());
  size_t pos = condi_size;

  // `m_condi_bitstream` might be indicating that the field is a constant field.
  //    In that case, there will be no more speck or sperr streams.
  //    Let's detect that case here and return early if it is true.
  //    It will be up to the decompress() routine to restore the actual constant field.
  if (m_conditioner.is_constant(m_condi_bitstream[0])) {
    if (condi_size == len)
      return RTNType::Good;
    else
      return RTNType::BitstreamWrongLen;
  }

  // Bitstream parser 2: extract SPECK stream from it.
  // Based on the number of bitplanes, decide on an integer length to use, and then
  //    instantiate the proper decoder, and ask the decoder to parse the SPECK stream.
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
  const auto speck_len = len - pos;  // Assume that the bitstream length is what's left.
  std::visit([speck_p, speck_len](auto&& dec) { dec->use_bitstream(speck_p, speck_len); },
             m_decoder);

  return RTNType::Good;
}

void sperr::SPECK_FLT::append_encoded_bitstream(vec8_type& buf) const
{
  const auto orig_size = buf.size();
  buf.resize(orig_size + m_condi_bitstream.size());
  std::copy(m_condi_bitstream.cbegin(), m_condi_bitstream.cend(), buf.begin() + orig_size);

  if (!m_conditioner.is_constant(m_condi_bitstream[0]))
    std::visit([&buf](auto&& enc) { enc->append_encoded_bitstream(buf); }, m_encoder);
}

auto sperr::SPECK_FLT::release_decoded_data() -> vecd_type&&
{
  return std::move(m_vals_d);
}

void sperr::SPECK_FLT::set_q(double q)
{
  m_q = q;
  m_tol.reset();
}

void sperr::SPECK_FLT::set_tolerance(double tol)
{
  m_tol = tol;
  m_q = 1.5 * tol;
}

void sperr::SPECK_FLT::set_dims(dims_type dims)
{
  m_dims = dims;
}

auto sperr::SPECK_FLT::integer_len() const -> size_t
{
  switch (m_uint_flag) {
    case UINTType::UINT64:
      assert(m_vals_ui.index() == 0);
      assert(m_encoder.index() == 0 ||
             std::visit([](auto&& coder) { return coder == nullptr; }, m_encoder));
      assert(m_decoder.index() == 0 ||
             std::visit([](auto&& coder) { return coder == nullptr; }, m_decoder));
      return sizeof(uint64_t);
    case UINTType::UINT32:
      assert(m_vals_ui.index() == 1);
      assert(m_encoder.index() == 1 ||
             std::visit([](auto&& coder) { return coder == nullptr; }, m_encoder));
      assert(m_decoder.index() == 1 ||
             std::visit([](auto&& coder) { return coder == nullptr; }, m_decoder));
      return sizeof(uint32_t);
    case UINTType::UINT16:
      assert(m_vals_ui.index() == 2);
      assert(m_encoder.index() == 2 ||
             std::visit([](auto&& coder) { return coder == nullptr; }, m_encoder));
      assert(m_decoder.index() == 2 ||
             std::visit([](auto&& coder) { return coder == nullptr; }, m_decoder));
      return sizeof(uint16_t);
    default:
      assert(m_vals_ui.index() == 3);
      assert(m_encoder.index() == 3 ||
             std::visit([](auto&& coder) { return coder == nullptr; }, m_encoder));
      assert(m_decoder.index() == 3 ||
             std::visit([](auto&& coder) { return coder == nullptr; }, m_decoder));
      return sizeof(uint8_t);
  }
}

void sperr::SPECK_FLT::m_instantiate_int_vec()
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

auto sperr::SPECK_FLT::m_midtread_f2i() -> RTNType
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
    case UINTType::UINT64:
      for (size_t i = 0; i < total_vals; i++) {
        auto ll = std::llrint(m_vals_d[i] / m_q);
        m_sign_array[i] = (ll >= 0);
        std::get<0>(m_vals_ui)[i] = static_cast<uint64_t>(std::abs(ll));
      }
      break;
    case UINTType::UINT32:
      for (size_t i = 0; i < total_vals; i++) {
        auto ll = std::llrint(m_vals_d[i] / m_q);
        m_sign_array[i] = (ll >= 0);
        std::get<1>(m_vals_ui)[i] = static_cast<uint32_t>(std::abs(ll));
      }
      break;
    case UINTType::UINT16:
      for (size_t i = 0; i < total_vals; i++) {
        auto ll = std::llrint(m_vals_d[i] / m_q);
        m_sign_array[i] = (ll >= 0);
        std::get<2>(m_vals_ui)[i] = static_cast<uint16_t>(std::abs(ll));
      }
      break;
    default:
      for (size_t i = 0; i < total_vals; i++) {
        auto ll = std::llrint(m_vals_d[i] / m_q);
        m_sign_array[i] = (ll >= 0);
        std::get<3>(m_vals_ui)[i] = static_cast<uint8_t>(std::abs(ll));
      }
  }

  return RTNType::Good;
}

void sperr::SPECK_FLT::m_midtread_i2f()
{
  assert(m_sign_array.size() == std::visit([](auto&& vec) { return vec.size(); }, m_vals_ui));

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
  if (m_vals_d.empty() || m_q <= 0.0 || m_vals_d.size() != total_vals)
    return RTNType::Error;

  m_condi_bitstream.clear();

  // Step 1: data goes through the conditioner
  //    Believe it or not, there are constant fields passed in for compression!
  //    Let's detect that case and skip the rest of the compression routine if it occurs.
  m_condi_bitstream = m_conditioner.condition(m_vals_d, m_dims);
  if (m_conditioner.is_constant(m_condi_bitstream[0]))
    return RTNType::Good;

  // Step 2: wavelet transform
  auto rtn = m_cdf.take_data(std::move(m_vals_d), m_dims);
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
  std::visit([&dims = m_dims](auto&& encoder) { encoder->set_dims(dims); }, m_encoder);
  switch (m_uint_flag) {
    case UINTType::UINT64:
      assert(m_vals_ui.index() == 0);
      assert(m_encoder.index() == 0);
      rtn = std::get<0>(m_encoder)->use_coeffs(std::move(std::get<0>(m_vals_ui)),
                                               std::move(m_sign_array));
      break;
    case UINTType::UINT32:
      assert(m_vals_ui.index() == 1);
      assert(m_encoder.index() == 1);
      rtn = std::get<1>(m_encoder)->use_coeffs(std::move(std::get<1>(m_vals_ui)),
                                               std::move(m_sign_array));
      break;
    case UINTType::UINT16:
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
  if (m_q <= 0.0 || m_condi_bitstream.empty())
    return RTNType::Error;

  m_vals_d.clear();
  std::visit([](auto&& vec) { vec.clear(); }, m_vals_ui);
  m_sign_array.clear();

  // `m_condi_bitstream` might be indicating a constant field, so let's see if that's
  //    the case, and if it is, we don't need to go through wavelet and speck stuff anymore.
  if (m_conditioner.is_constant(m_condi_bitstream[0])) {
    auto rtn = m_conditioner.inverse_condition(m_vals_d, m_dims, m_condi_bitstream);
    return rtn;
  }

  // Step 1: Integer SPECK decode.
  //    Note: the decoder has already parsed the bitstream in function `use_bitstream()`.
  std::visit([&dims = m_dims](auto&& decoder) { decoder->set_dims(dims); }, m_decoder);
  std::visit([](auto&& decoder) { decoder->decode(); }, m_decoder);
  std::visit([&vec = m_vals_ui](auto&& dec) { vec = dec->release_coeffs(); }, m_decoder);
  m_sign_array = std::visit([](auto&& dec) { return dec->release_signs(); }, m_decoder);

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
  rtn = m_conditioner.inverse_condition(m_vals_d, m_dims, m_condi_bitstream);
  if (rtn != RTNType::Good)
    return rtn;

  return RTNType::Good;
}
