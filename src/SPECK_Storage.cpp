#include "SPECK_Storage.h"

#include <cassert>
#include <cstring>
#include <numeric>

template <typename T>
auto sperr::SPECK_Storage::copy_data(const T* p, size_t len, dims_type dims) -> RTNType
{
  static_assert(std::is_floating_point<T>::value, "!! Only floating point values are supported !!");
  if (len != dims[0] * dims[1] * dims[2])
    return RTNType::WrongDims;

  // If `m_coeff_buf` is empty, or having a different length, we need to
  // allocate memory!
  m_coeff_buf.resize(len);
  std::copy(p, p + len, m_coeff_buf.begin());

  m_dims = dims;

  return RTNType::Good;
}
template auto sperr::SPECK_Storage::copy_data(const double*, size_t, dims_type) -> RTNType;
template auto sperr::SPECK_Storage::copy_data(const float*, size_t, dims_type) -> RTNType;

auto sperr::SPECK_Storage::take_data(vecd_type&& coeffs, dims_type dims) -> RTNType
{
  if (coeffs.size() != dims[0] * dims[1] * dims[2])
    return RTNType::WrongDims;

  m_coeff_buf = std::move(coeffs);
  m_dims = dims;

  return RTNType::Good;
}

auto sperr::SPECK_Storage::release_data() -> vecd_type&&
{
  m_dims = {0, 0, 0};
  return std::move(m_coeff_buf);
}

auto sperr::SPECK_Storage::view_data() const -> const vecd_type&
{
  return m_coeff_buf;
}

auto sperr::SPECK_Storage::m_prepare_encoded_bitstream() -> RTNType
{
  // Header definition: 24 bytes in QZ_TERM mode
  // dim_x,     dim_y,     dim_z,     max_coeff_bits, qz_lev,   stream_len (in byte)
  // uint32_t,  uint32_t,  uint32_t,  int16_t,        int16_t,  uint64_t
  //
  // Header definition: 10 bytes in size-bounded mode
  // max_coeff_bits, stream_len (in byte)
  // int16_t,        uint64_t

  assert(m_bit_buffer.size() % 8 == 0);
  const uint64_t bit_in_byte = m_bit_buffer.size() / 8;
  const size_t total_size = m_header_size + bit_in_byte;
  m_encoded_stream.resize(total_size);
  auto* const ptr = m_encoded_stream.data();

  // Fill header
  size_t pos = 0;
  int16_t max_bit = static_cast<int16_t>(m_max_coeff_bit);  // int16_t is big enough
  std::memcpy(ptr + pos, &max_bit, sizeof(max_bit));
  pos += sizeof(max_bit);

#ifdef QZ_TERM
  int16_t qz_lev = static_cast<int16_t>(m_qz_lev);  // int16_t is big enough
  std::memcpy(ptr + pos, &qz_lev, sizeof(qz_lev));
  pos += sizeof(qz_lev);
#endif

  std::memcpy(ptr + pos, &bit_in_byte, sizeof(bit_in_byte));
  pos += sizeof(bit_in_byte);
  assert(pos == m_header_size);

  // Assemble the bitstream into bytes
  auto rtn = sperr::pack_booleans(m_encoded_stream, m_bit_buffer, pos);
  return rtn;
}

auto sperr::SPECK_Storage::parse_encoded_bitstream(const void* comp_buf, size_t comp_size)
    -> RTNType
{
  // The buffer passed in is supposed to consist a header and then a compacted
  // bitstream, just like what was returned by `prepare_encoded_bitstream()`. Note:
  // header definition is documented in prepare_encoded_bitstream().

  const uint8_t* const ptr = static_cast<const uint8_t*>(comp_buf);

  // Parse the header
  size_t pos = 0;
  int16_t max_bit;
  std::memcpy(&max_bit, ptr + pos, sizeof(max_bit));
  pos += sizeof(max_bit);
  m_max_coeff_bit = max_bit;

#ifdef QZ_TERM
  int16_t qz_lev;
  std::memcpy(&qz_lev, ptr + pos, sizeof(qz_lev));
  pos += sizeof(qz_lev);
  m_qz_lev = qz_lev;
#endif

  uint64_t bit_in_byte;
  std::memcpy(&bit_in_byte, ptr + pos, sizeof(bit_in_byte));
  pos += sizeof(bit_in_byte);

  // Unpack bits
  const auto num_of_bools = (comp_size - pos) * 8;
  m_bit_buffer.resize(num_of_bools);  // allocate enough space before passing it around
  auto rtn = sperr::unpack_booleans(m_bit_buffer, comp_buf, comp_size, pos);
  if (rtn != RTNType::Good)
    return rtn;

  m_coeff_buf.resize(m_dims[0] * m_dims[1] * m_dims[2]);

  m_encoded_stream.clear();

  return RTNType::Good;
}

auto sperr::SPECK_Storage::view_encoded_bitstream() const -> const vec8_type&
{
  return m_encoded_stream;
}

auto sperr::SPECK_Storage::release_encoded_bitstream() -> vec8_type&&
{
  return std::move(m_encoded_stream);
}

auto sperr::SPECK_Storage::get_speck_stream_size(const void* buf) const -> uint64_t
{
  // Given the header definition in `prepare_encoded_bitstream()`, directly
  // go retrieve the value stored in the last 8 bytes of the header
  const uint8_t* const ptr = static_cast<const uint8_t*>(buf);
  uint64_t bit_in_byte;
  std::memcpy(&bit_in_byte, ptr + m_header_size - 8, sizeof(bit_in_byte));

  return (m_header_size + size_t(bit_in_byte));
}

