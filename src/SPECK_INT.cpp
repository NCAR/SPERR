#include "SPECK_INT.h"

#include <algorithm>
#include <cassert>
#include <cstring>
#include <numeric>

void sperr::SPECK_INT::set_dims(dims_type dims)
{
  m_dims = dims;
}

auto sperr::SPECK_INT::get_speck_full_len(const void* buf) const -> uint64_t
{
  // Given the header definition, directly go retrieve the value stored in the bytes 1--9.
  const uint8_t* const ptr = static_cast<const uint8_t*>(buf);
  uint64_t num_bits = 0;
  std::memcpy(&num_bits, ptr + 1, sizeof(num_bits));
  while (num_bits % 8 != 0)
    ++num_bits;

  return (m_header_size + num_bits / 8);
}

void sperr::SPECK_INT::encode()
{
  std::printf("This func should not be run!\n");
  // Return an error code
}

void sperr::SPECK_INT::decode()
{
  std::printf("This func should not be run!\n");
  // Return an error code
}

void sperr::SPECK_INT::use_coeffs(vecui_t coeffs, vecb_type signs)
{
  m_coeff_buf = std::move(coeffs);
  m_sign_array = std::move(signs);
}

void sperr::SPECK_INT::use_bitstream(const vec8_type& stream)
{
  // Header definition: 9 bytes in total:
  // num_bitplanes (uint8_t), num_useful_bits (uint64_t)

  // Step 1: extract num_bitplanes and num_useful_bits
  std::memcpy(&m_num_bitplanes, stream.data(), sizeof(m_num_bitplanes));
  std::memcpy(&m_total_bits, stream.data() + sizeof(m_num_bitplanes), sizeof(m_total_bits));

  // Step 2: unpack bits
  //const auto num_of_bools = (stream.size() - m_header_size) * 8;
  //m_bit_buffer.resize(num_of_bools);
  //sperr::unpack_booleans(m_bit_buffer, stream.data(), stream.size(), m_header_size);
  m_bit_buffer.parse_bitstream(stream.data() + m_header_size, m_total_bits);
}

auto sperr::SPECK_INT::view_encoded_bitstream() const -> const vec8_type&
{
  return m_encoded_bitstream;
}

auto sperr::SPECK_INT::release_encoded_bitstream() -> vec8_type&&
{
  return std::move(m_encoded_bitstream);
}

auto sperr::SPECK_INT::release_coeffs() -> vecui_t&&
{
  return std::move(m_coeff_buf);
}

auto sperr::SPECK_INT::release_signs() -> vecb_type&&
{
  return std::move(m_sign_array);
}

auto sperr::SPECK_INT::view_coeffs() const -> const vecui_t&
{
  return m_coeff_buf;
}

auto sperr::SPECK_INT::view_signs() const -> const vecb_type&
{
  return m_sign_array;
}

void sperr::SPECK_INT::m_assemble_bitstream()
{
  // Header definition: 9 bytes in total:
  // num_bitplanes (uint8_t), num_useful_bits (uint64_t)

  // Step 1: keep the number of useful bits, and then pad `m_bit_buffer`.
  //const uint64_t useful_bits = m_bit_buffer.size();
  //while (m_bit_buffer.size() % 8 != 0)
  //  m_bit_buffer.push_back(false);

  // Step 2: allocate space for the encoded bitstream
  uint64_t bit_in_byte = m_total_bits / 8;
  if (m_total_bits % 8 != 0)
    ++bit_in_byte;
  const size_t total_size = m_header_size + bit_in_byte;
  m_encoded_bitstream.resize(total_size);
  auto* const ptr = m_encoded_bitstream.data();

  // Step 3: fill header
  size_t pos = 0;
  std::memcpy(ptr + pos, &m_num_bitplanes, sizeof(m_num_bitplanes));
  pos += sizeof(m_num_bitplanes);
  std::memcpy(ptr + pos, &m_total_bits, sizeof(m_total_bits));
  pos += sizeof(m_total_bits);

  // Step 4: assemble `m_bit_buffer` into bytes
  //sperr::pack_booleans(m_encoded_bitstream, m_bit_buffer, pos);
  m_bit_buffer.write_bitstream(ptr + m_header_size, m_total_bits);

  // Step 5: restore `m_bit_buffer` to its original size
  //m_bit_buffer.resize(useful_bits);
}

void sperr::SPECK_INT::m_refinement_pass_encode()
{
  // First, process significant pixels previously found.
  //
  const auto tmp1 = std::array<uint_t, 2>{uint_t{0}, m_threshold};

  for (size_t i = 0; i < m_LSP_mask.size(); i += 64) {
    const auto value = m_LSP_mask.read_long(i);
    if (value != 0) {
      for (size_t j = 0; j < 64; j++) {
        if ((value >> j) & uint64_t{1}) {
          const bool o1 = m_coeff_buf[i + j] >= m_threshold;
          m_coeff_buf[i + j] -= tmp1[o1];
          //m_bit_buffer.push_back(o1);
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

void sperr::SPECK_INT::m_refinement_pass_decode()
{
  // First, process significant pixels previously found.
  //
  const auto tmp = std::array<uint_t, 2>{uint_t{0}, m_threshold};

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
