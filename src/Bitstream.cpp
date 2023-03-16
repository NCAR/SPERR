#include "Bitstream.h"

#include <algorithm>  // std::max()
#include <iterator>  // std::distance()
#include <cassert>
#include <cstring>

// Constructor
sperr::Bitstream::Bitstream(size_t nbits)
{
  m_itr = m_buf.begin();  // give it an initial value!
  this->reserve(std::max(64ul, nbits));
}

// Functions for both read and write
void sperr::Bitstream::rewind()
{
  m_itr = m_buf.begin();
  m_buffer = 0;
  m_bits = 0;
}

auto sperr::Bitstream::capacity() const -> size_t
{
  return m_buf.size() * 64;
}

void sperr::Bitstream::reserve(size_t nbits)
{
  if (nbits > m_buf.size() * 64) {
    // Number of longs that's absolutely needed.
    auto num_longs = nbits / 64;
    if (nbits % 64 != 0)
      num_longs++;

    const auto dist = std::distance(m_buf.begin(), m_itr);
    m_buf.resize(num_longs, 0);
    m_itr = m_buf.begin() + dist;
  }
}

// Functions for read
auto sperr::Bitstream::rtell() const -> size_t
{
  // Stupid C++ insists that `m_buf.begin()` gives me a const iterator...
  std::vector<uint64_t>::const_iterator itr2 = m_itr;
  return std::distance(m_buf.begin(), itr2) * 64 - m_bits;
}

void sperr::Bitstream::rseek(size_t offset)
{
  m_itr = m_buf.begin() + offset / 64;
  const auto rem = offset % 64;
  if (rem) {
    m_buffer = *m_itr >> rem;
    ++m_itr;
    m_bits = 64 - rem;
  }
  else {
    m_buffer = 0;
    m_bits = 0;
  }
}

auto sperr::Bitstream::read_bit() -> bool
{
  if (m_bits == 0) {
    m_buffer = *m_itr;
    ++m_itr;
    m_bits = 64;
  }
  --m_bits;
  bool bit = m_buffer & uint64_t{1};
  m_buffer >>= 1;
  return bit;
}

// Functions for write
auto sperr::Bitstream::wtell() const -> size_t
{
  // Stupid C++ insists that `m_buf.begin()` gives me a const iterator...
  std::vector<uint64_t>::const_iterator itr2 = m_itr;
  return std::distance(m_buf.begin(), itr2) * 64 + m_bits;
}

void sperr::Bitstream::wseek(size_t offset)
{
  m_itr = m_buf.begin() + offset / 64;
  const auto rem = offset % 64;
  if (rem) {
    m_buffer = *m_itr;
    m_buffer &= (uint64_t{1} << rem) - 1;
    m_bits = rem;
  }
  else {
    m_buffer = 0;
    m_bits = 0;
  }
}

void sperr::Bitstream::write_bit(bool bit)
{
  m_buffer += uint64_t{bit} << m_bits;
  if (++m_bits == 64) {
    if (m_itr == m_buf.end()) {  // allocate memory if necessary
      const auto dist = m_buf.size();
      m_buf.push_back(0);
      m_buf.resize(m_buf.capacity());
      m_itr = m_buf.begin() + dist;
    }
    *m_itr = m_buffer;
    ++m_itr;
    m_buffer = 0;
    m_bits = 0;
  }
}

void sperr::Bitstream::flush()
{
  if (m_bits) { // only flush when there are remaining bits
    if (m_itr == m_buf.end()) {
      const auto dist = m_buf.size();
      m_buf.push_back(0);
      m_buf.resize(m_buf.capacity());
      m_itr = m_buf.begin() + dist;
    }
    *m_itr = m_buffer;
    ++m_itr;
    m_buffer = 0;
    m_bits = 0;
  }
}

// Functions to provide or parse a compact bitstream
auto sperr::Bitstream::get_bitstream(size_t num_bits) const -> std::vector<std::byte>
{
  assert(num_bits <= m_buf.size() * 64);
  const auto num_longs = num_bits / 64;
  const auto rem_bits = num_bits % 64;
  auto rem_bytes = rem_bits / 8;
  if (rem_bits % 8 != 0)
    rem_bytes++;

  auto tmp = std::vector<std::byte>(num_longs * sizeof(uint64_t) + rem_bytes);
  std::memcpy(tmp.data(), m_buf.data(), num_longs * sizeof(uint64_t));

  if (rem_bytes > 0) {
    uint64_t value = m_buf[num_longs];
    std::memcpy(tmp.data() + num_longs * sizeof(uint64_t), &value, rem_bytes);
  }

  return tmp;
}

void sperr::Bitstream::parse_bitstream(const void* p, size_t num_bits)
{
  this->reserve(num_bits);

  const auto num_longs = num_bits / 64;
  const auto rem_bits = num_bits % 64;
  auto rem_bytes = rem_bits / 8;
  if (rem_bits % 8 != 0)
    rem_bytes++;

  const auto* p_byte = static_cast<const std::byte*>(p);
  std::memcpy(m_buf.data(), p_byte, num_longs * sizeof(uint64_t));

  if (rem_bytes > 0)
    std::memcpy(m_buf.data() + num_longs, p_byte + num_longs * sizeof(uint64_t), rem_bytes);

  this->rewind();
}
