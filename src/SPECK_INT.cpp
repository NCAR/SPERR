#include "SPECK_INT.h"

#include <algorithm>
#include <cassert>
#include <cstring>
#include <numeric>

template <typename T>
sperr::SPECK_INT<T>::SPECK_INT()
{
  static_assert(std::is_integral_v<T>);
  static_assert(std::is_unsigned_v<T>);
}

template <typename T>
auto sperr::SPECK_INT<T>::integer_len() const -> UINTType
{
  if constexpr (std::is_same_v<uint64_t, T>)
    return UINTType::UINT64;
  else if constexpr (std::is_same_v<uint32_t, T>)
    return UINTType::UINT32;
  else if constexpr (std::is_same_v<uint16_t, T>)
    return UINTType::UINT16;
  else
    return UINTType::UINT8;
}

template <typename T>
void sperr::SPECK_INT<T>::set_dims(dims_type dims)
{
  m_dims = dims;
}

template <typename T>
auto sperr::SPECK_INT<T>::get_num_bitplanes(const void* buf) const -> uint8_t
{
  // Given the header definition, directly retrieve the value stored in the first byte.
  const uint8_t* const ptr = static_cast<const uint8_t*>(buf);
  uint8_t bitplanes = 0;
  std::memcpy(&bitplanes, ptr, sizeof(bitplanes));
  return bitplanes;

}

template <typename T>
auto sperr::SPECK_INT<T>::get_speck_bits(const void* buf) const -> uint64_t
{
  // Given the header definition, directly retrieve the value stored in bytes 1--9.
  const uint8_t* const ptr = static_cast<const uint8_t*>(buf);
  uint64_t num_bits = 0;
  std::memcpy(&num_bits, ptr + 1, sizeof(num_bits));
  return num_bits;
}

template <typename T>
auto sperr::SPECK_INT<T>::get_stream_full_len(const void* buf) const -> uint64_t
{
  auto num_bits = get_speck_bits(buf);
  while (num_bits % 8 != 0)
    ++num_bits;
  return (m_header_size + num_bits / 8);
}

template <typename T>
void sperr::SPECK_INT<T>::encode()
{
  m_bit_buffer.rewind();
  m_total_bits = 0;
  m_initialize_lists();

  // Mark every coefficient as insignificant
  m_LSP_mask.resize(m_coeff_buf.size());
  m_LSP_mask.reset();

  // Decide the starting threshold.
  const auto max_coeff = *std::max_element(m_coeff_buf.cbegin(), m_coeff_buf.cend());
  m_num_bitplanes = 1;
  m_threshold = 1;
  while (m_threshold * uint_type{2} <= max_coeff) {
    m_threshold *= uint_type{2};
    m_num_bitplanes++;
  }

  for (uint8_t bitplane = 0; bitplane < m_num_bitplanes; bitplane++) {
    m_sorting_pass();
    m_refinement_pass_encode();

    m_threshold /= uint_type{2};
    m_clean_LIS();
  }

  // Flush the bitstream, and record the total number of bits
  m_total_bits = m_bit_buffer.wtell();
  m_bit_buffer.flush();
}

template <typename T>
void sperr::SPECK_INT<T>::decode()
{
  m_initialize_lists();

  // initialize coefficients to be zero, and sign array to be all positive
  const auto coeff_len = m_dims[0] * m_dims[1] * m_dims[2];
  m_coeff_buf.assign(coeff_len, uint_type{0});
  m_sign_array.assign(coeff_len, true);

  // Mark every coefficient as insignificant
  m_LSP_mask.resize(m_coeff_buf.size());
  m_LSP_mask.reset();
  m_bit_buffer.rewind();
  m_bit_idx = 0;

  // Restore the biggest `m_threshold`
  m_threshold = 1;
  for (uint8_t i = 1; i < m_num_bitplanes; i++)
    m_threshold *= uint_type{2};

  for (uint8_t bitplane = 0; bitplane < m_num_bitplanes; bitplane++) {
    m_sorting_pass();
    m_refinement_pass_decode();

    m_threshold /= uint_type{2};
    m_clean_LIS();
  }

  assert(m_bit_idx == m_total_bits);
}

template <typename T>
void sperr::SPECK_INT<T>::use_coeffs(vecui_type coeffs, vecb_type signs)
{
  m_coeff_buf = std::move(coeffs);
  m_sign_array = std::move(signs);
}

template <typename T>
void sperr::SPECK_INT<T>::use_bitstream(const vec8_type& stream)
{
  // Header definition: 9 bytes in total:
  // num_bitplanes (uint8_t), num_useful_bits (uint64_t)

  // Step 1: extract num_bitplanes and num_useful_bits
  std::memcpy(&m_num_bitplanes, stream.data(), sizeof(m_num_bitplanes));
  std::memcpy(&m_total_bits, stream.data() + sizeof(m_num_bitplanes), sizeof(m_total_bits));

  // Step 2: unpack bits
  m_bit_buffer.parse_bitstream(stream.data() + m_header_size, m_total_bits);
}

template <typename T>
auto sperr::SPECK_INT<T>::release_coeffs() -> vecui_type&&
{
  return std::move(m_coeff_buf);
}

template <typename T>
auto sperr::SPECK_INT<T>::release_signs() -> vecb_type&&
{
  return std::move(m_sign_array);
}

template <typename T>
auto sperr::SPECK_INT<T>::view_coeffs() const -> const vecui_type&
{
  return m_coeff_buf;
}

template <typename T>
auto sperr::SPECK_INT<T>::view_signs() const -> const vecb_type&
{
  return m_sign_array;
}

template <typename T>
void sperr::SPECK_INT<T>::write_encoded_bitstream(vec8_type& buffer, size_t offset) const
{
  // Header definition: 9 bytes in total:
  // num_bitplanes (uint8_t), num_useful_bits (uint64_t)

  // Step 1: calculate size and allocate space for the encoded bitstream
  uint64_t bit_in_byte = m_total_bits / 8;
  if (m_total_bits % 8 != 0)
    ++bit_in_byte;
  const size_t total_size = m_header_size + bit_in_byte;

  buffer.resize(total_size);
  buffer.resize(total_size);
  auto* const ptr = buffer.data() + offset;

  // Step 2: fill header
  size_t pos = 0;
  std::memcpy(ptr + pos, &m_num_bitplanes, sizeof(m_num_bitplanes));
  pos += sizeof(m_num_bitplanes);
  std::memcpy(ptr + pos, &m_total_bits, sizeof(m_total_bits));
  pos += sizeof(m_total_bits);

  // Step 3: assemble `m_bit_buffer` into bytes
  m_bit_buffer.write_bitstream(ptr + m_header_size, m_total_bits);
}

template <typename T>
void sperr::SPECK_INT<T>::m_refinement_pass_encode()
{
  // First, process significant pixels previously found.
  //
  const auto tmp1 = std::array<uint_type, 2>{uint_type{0}, m_threshold};

  for (size_t i = 0; i < m_LSP_mask.size(); i += 64) {
    const auto value = m_LSP_mask.read_long(i);
    if (value != 0) {
      for (size_t j = 0; j < 64; j++) {
        if ((value >> j) & uint64_t{1}) {
          const bool o1 = m_coeff_buf[i + j] >= m_threshold;
          m_coeff_buf[i + j] -= tmp1[o1];
          m_bit_buffer.wbit(o1);
        }
      }
    }
  }

  // Second, mark newly found significant pixels in `m_LSP_mask`.
  //
  for (auto idx : m_LSP_new)
    m_LSP_mask.write_bit(idx, true);
  m_LSP_new.clear();
}

template <typename T>
void sperr::SPECK_INT<T>::m_refinement_pass_decode()
{
  // First, process significant pixels previously found.
  //
  const auto tmp = std::array<uint_type, 2>{uint_type{0}, m_threshold};

  for (size_t i = 0; i < m_LSP_mask.size(); i += 64) {
    const auto value = m_LSP_mask.read_long(i);
    if (value != 0) {
      for (size_t j = 0; j < 64; j++) {
        if ((value >> j) & uint64_t{1}) {
          m_coeff_buf[i + j] += tmp[m_bit_buffer.rbit()];
          ++m_bit_idx;
        }
      }
    }
  }
  assert(m_bit_idx <= m_total_bits);

  // Second, mark newly found significant pixels in `m_LSP_mask`
  //
  for (auto idx : m_LSP_new)
    m_LSP_mask.write_bit(idx, true);
  m_LSP_new.clear();
}
