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

void sperr::SPECK_INT::use_coeffs(veci_t coeffs, vecb_type signs)
{
  m_coeff_buf = std::move(coeffs);
  m_sign_array = std::move(signs);
}

void sperr::SPECK_INT::use_bitstream(const vec8_type& stream)
{
  // Header definition: 9 bytes in total:
  // num_bitplanes (uint8_t), num_useful_bits (uint64_t)

  // Step 1: extract num_bitplanes and num_useful_bits
  uint64_t useful_bits = 0;
  std::memcpy(&m_num_bitplanes, stream.data(), sizeof(m_num_bitplanes));
  std::memcpy(&useful_bits, stream.data() + sizeof(m_num_bitplanes), sizeof(useful_bits));

  // Step 2: unpack bits
  const auto num_of_bools = (stream.size() - m_header_size) * 8;
  m_bit_buffer.resize(num_of_bools);
  sperr::unpack_booleans(m_bit_buffer, stream.data(), stream.size(), m_header_size);

  // Step 3: remove padding bits
  m_bit_buffer.resize(useful_bits);
}

auto sperr::SPECK_INT::view_encoded_bitstream() const -> const vec8_type&
{
  return m_encoded_bitstream;
}

auto sperr::SPECK_INT::release_coeffs() -> veci_t&&
{
  return std::move(m_coeff_buf);
}

auto sperr::SPECK_INT::release_signs() -> vecb_type&&
{
  return std::move(m_sign_array);
}

auto sperr::SPECK_INT::view_coeffs() const -> const veci_t&
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
  const uint64_t useful_bits = m_bit_buffer.size();
  while (m_bit_buffer.size() % 8 != 0)
    m_bit_buffer.push_back(false);

  // Step 2: allocate space for the encoded bitstream
  const uint64_t bit_in_byte = m_bit_buffer.size() / 8;
  const size_t total_size = m_header_size + bit_in_byte;
  m_encoded_bitstream.resize(total_size);
  auto* const ptr = m_encoded_bitstream.data();

  // Step 3: fill header
  size_t pos = 0;
  std::memcpy(ptr + pos, &m_num_bitplanes, sizeof(m_num_bitplanes));
  pos += sizeof(m_num_bitplanes);
  std::memcpy(ptr + pos, &useful_bits, sizeof(useful_bits));
  pos += sizeof(useful_bits);

  // Step 4: assemble `m_bit_buffer` into bytes
  sperr::pack_booleans(m_encoded_bitstream, m_bit_buffer, pos);

  // Step 5: restore `m_bit_buffer` to its original size
  m_bit_buffer.resize(useful_bits);
}


