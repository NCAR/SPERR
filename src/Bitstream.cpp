#include "Bitstream.h"

#include <algorithm>  // std::max()
#include <cassert>
#include <cstring>

// Constructor
sperr::Bitstream::Bitstream(size_t nbits)
{
  assert(m_wsize == zfp::stream_alignment());
  // Actually, leaving the pointer `m_handle` NULL will require so many null pointer
  //   tests down the road, so we make sure to initialize it in the constructor.
  this->reserve(std::max(m_wsize, nbits));
}

// Destructor
sperr::Bitstream::~Bitstream()
{
  if (m_handle) {
    zfp::stream_close(m_handle);
    m_handle = nullptr;
  }
}

// Functions for both read and write
void sperr::Bitstream::rewind()
{
  return zfp::stream_rewind(m_handle);
}

auto sperr::Bitstream::capacity() const -> size_t
{
  return m_capacity;
}
auto sperr::Bitstream::wsize() const -> size_t
{
  return m_wsize;
}

void sperr::Bitstream::reserve(size_t nbits)
{
  if (nbits > m_capacity) {
    // Number of longs that's absolutely needed.
    auto num_longs = nbits / m_wsize;
    if (nbits % m_wsize != 0)
      num_longs++;

    m_buf.reserve(num_longs);
    m_buf.resize(m_buf.capacity());
    m_capacity = m_buf.size() * m_wsize;

    if (m_handle)
      zfp::stream_close(m_handle);
    m_handle = zfp::stream_open(m_buf.data(), m_buf.size() * sizeof(uint64_t));
    zfp::stream_rewind(m_handle);
  }
}

// Functions for read
auto sperr::Bitstream::rtell() const -> size_t
{
  return zfp::stream_rtell(m_handle);
}

void sperr::Bitstream::rseek(size_t offset)
{
  zfp::stream_rseek(m_handle, offset);
}

auto sperr::Bitstream::test_range(size_t start_pos, size_t range_len) -> bool
{
  return zfp::test_range(m_handle, start_pos, range_len);
}

auto sperr::Bitstream::stream_read_n_bits(size_t n) -> uint64_t
{
  assert(n <= 64);
  return zfp::stream_read_bits(m_handle, n);
}

auto sperr::Bitstream::stream_read_bit() -> bool
{
  return zfp::stream_read_bit(m_handle);
}

auto sperr::Bitstream::random_read_bit(size_t pos) const -> bool
{
  return zfp::random_read_bit(m_handle, pos);
}

// Functions for write
auto sperr::Bitstream::wtell() const -> size_t
{
  return zfp::stream_wtell(m_handle);
}

void sperr::Bitstream::wseek(size_t offset)
{
  return zfp::stream_wseek(m_handle, offset);
}

auto sperr::Bitstream::flush() -> size_t
{
  return zfp::stream_flush(m_handle);
}

auto sperr::Bitstream::stream_write_bit(bool bit) -> bool
{
  if (zfp::stream_wtell(m_handle) == m_capacity)
    m_wgrow_buf();
  return zfp::stream_write_bit(m_handle, bit);
}

auto sperr::Bitstream::stream_write_n_bits(uint64_t value, size_t n) -> uint64_t
{
  assert(n <= 64);
  if (zfp::stream_wtell(m_handle) + n > m_capacity)
    m_wgrow_buf();
  return zfp::stream_write_bits(m_handle, value, n);
}

auto sperr::Bitstream::random_write_bit(bool bit, size_t pos) -> bool
{
  return zfp::random_write_bit(m_handle, bit, pos);
}

void sperr::Bitstream::m_wgrow_buf()
{
  const auto curr_pos = zfp::stream_wtell(m_handle);
  zfp::stream_flush(m_handle);

  m_buf.push_back(0ul);
  m_buf.resize(m_buf.capacity());
  m_capacity = m_buf.size() * m_wsize;

  zfp::stream_close(m_handle);
  m_handle = zfp::stream_open(m_buf.data(), m_buf.size() * sizeof(uint64_t));
  zfp::stream_wseek(m_handle, curr_pos);
}

// Functions to provide or parse a compact bitstream
auto sperr::Bitstream::get_bitstream(size_t num_bits) -> std::vector<std::byte>
{
  assert(num_bits <= m_capacity);
  const auto num_longs = num_bits / m_wsize;
  const auto rem_bits = num_bits % m_wsize;
  auto rem_bytes = rem_bits / 8;
  if (rem_bits % 8 != 0)
    rem_bytes++;

  auto tmp = std::vector<std::byte>(num_longs * sizeof(uint64_t) + rem_bytes);
  std::memcpy(tmp.data(), m_buf.data(), num_longs * sizeof(uint64_t));

  if (rem_bits > 0) {
    zfp::stream_rseek(m_handle, num_longs * m_wsize);
    uint64_t value = zfp::stream_read_bits(m_handle, rem_bits);
    std::memcpy(tmp.data() + num_longs * sizeof(uint64_t), &value, rem_bytes);
  }

  return tmp;
}

void sperr::Bitstream::parse_bitstream(const void* p, size_t num_bits)
{
  this->reserve(num_bits);

  const auto num_longs = num_bits / m_wsize;
  const auto rem_bits = num_bits % m_wsize;
  auto rem_bytes = rem_bits / 8;
  if (rem_bits % 8 != 0)
    rem_bytes++;

  const auto* p_byte = static_cast<const std::byte*>(p);
  std::memcpy(m_buf.data(), p_byte, num_longs * sizeof(uint64_t));

  if (rem_bits > 0)
    std::memcpy(m_buf.data() + num_longs, p_byte + num_longs * sizeof(uint64_t), rem_bytes);

  zfp::stream_rewind(m_handle);
}
