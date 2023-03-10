#include "ZFP_bitstream.h"

#include <cstring>

// Constructor
sperr::ZFP_bitstream::ZFP_bitstream(size_t nbits)
{
  // Number of longs that's absolutely needed.
  auto num_of_longs = nbits / 64ul;
  if (nbits % 64ul != 0)
    num_of_longs++;

  m_buf.resize(num_of_longs);
  m_capacity = num_of_longs * 64ul;

  m_handle.begin = m_buf.data();
  m_handle.end = m_handle.begin + m_buf.size();
#ifdef BIT_STREAM_STRIDED
  zfp::stream_set_stride(m_handle, 0, 0);
#endif
  zfp::stream_rewind(&m_handle);
}

// Functions for both read and write
void sperr::ZFP_bitstream::rewind()
{
  return zfp::stream_rewind(&m_handle);
}

auto sperr::ZFP_bitstream::capacity() const -> size_t
{
  return m_capacity;
}

// Functions for read
auto sperr::ZFP_bitstream::rtell() const -> size_t
{
  return zfp::stream_rtell(&m_handle);
}

void sperr::ZFP_bitstream::rseek(size_t offset)
{
  zfp::stream_rseek(&m_handle, offset);
}

auto sperr::ZFP_bitstream::test_range(size_t start_pos, size_t range_len) -> bool
{
  return zfp::test_range(&m_handle, start_pos, range_len);
}

auto sperr::ZFP_bitstream::stream_read_n_bits(size_t n) -> uint64_t
{
  assert(n <= 64);
  return zfp::stream_read_bits(&m_handle, n);
}

// Functions for write
auto sperr::ZFP_bitstream::wtell() const -> size_t
{
  return zfp::stream_wtell(&m_handle);
}

void sperr::ZFP_bitstream::wseek(size_t offset)
{
  return zfp::stream_wseek(&m_handle, offset);
}

auto sperr::ZFP_bitstream::flush() -> size_t
{
  return zfp::stream_flush(&m_handle);
}

auto sperr::ZFP_bitstream::random_write_bit(bool bit, size_t pos) -> bool
{
  return zfp::random_write_bit(&m_handle, bit, pos);
}

void sperr::ZFP_bitstream::m_wgrow_buf()
{
  const auto curr_pos = zfp::stream_wtell(&m_handle);
  zfp::stream_flush(&m_handle);

  m_buf.push_back(0ul);
  m_buf.resize(m_buf.capacity());
  m_capacity = m_buf.size() * 64;

  m_handle.begin = m_buf.data();
  m_handle.end = m_handle.begin + m_buf.size();
#ifdef BIT_STREAM_STRIDED
  zfp::stream_set_stride(m_handle, 0, 0);
#endif

  zfp::stream_wseek(&m_handle, curr_pos);
}

// Functions to provide or parse a compact bitstream
auto sperr::ZFP_bitstream::get_bitstream(size_t num_bits) -> std::vector<std::byte>
{
  assert(num_bits <= m_capacity);
  const auto num_longs = num_bits / 64;
  const auto rem_bits = num_bits - num_longs * 64;
  auto rem_bytes = rem_bits / 8;
  if (rem_bits % 8 != 0)
    rem_bytes++;

  auto tmp = std::vector<std::byte>(num_longs * sizeof(uint64_t) + rem_bytes);
  std::memcpy(tmp.data(), m_buf.data(), num_longs * sizeof(uint64_t));

  if (rem_bits > 0) {
    zfp::stream_rseek(&m_handle, num_longs * 64);
    uint64_t value = zfp::stream_read_bits(&m_handle, rem_bits);
    std::memcpy(tmp.data() + num_longs * sizeof(uint64_t), &value, rem_bytes);
  }

  return tmp;
}

void sperr::ZFP_bitstream::parse_bitstream(const void* p, size_t num_bits)
{
  if (num_bits > m_capacity)
    m_wgrow_buf();

  const auto num_longs = num_bits / 64;
  const auto rem_bits = num_bits - num_longs * 64;
  auto rem_bytes = rem_bits / 8;
  if (rem_bits % 8 != 0)
    rem_bytes++;

  const auto* p_byte = static_cast<const std::byte*>(p);
  std::memcpy(m_buf.data(), p_byte, num_longs * sizeof(uint64_t));

  if (rem_bits > 0)
    std::memcpy(m_buf.data() + num_longs, p_byte + num_longs * sizeof(uint64_t), rem_bytes);

  zfp::stream_rewind(&m_handle);
}
