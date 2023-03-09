#include "ZFP_bitstream.h"

// Constructor
sperr::ZFP_bitstream::ZFP_bitstream(size_t nbits)
{
  // Number of longs that's absolutely needed.
  auto num_of_longs = nbits / 64ul;
  if (nbits % 64ul != 0)
    num_of_longs++;

  m_buf.resize(num_of_longs);
  m_capacity = num_of_longs * 64ul;

  m_handle.reset(zfp::stream_open(m_buf.data(), m_buf.size() * sizeof(uint64_t)));
}

// Functions for both read and write
void sperr::ZFP_bitstream::rewind()
{
  return zfp::stream_rewind(m_handle.get());
}

auto sperr::ZFP_bitstream::capacity() const -> size_t
{
  return m_capacity;
}

// Functions for read
auto sperr::ZFP_bitstream::rtell() const -> size_t
{
  return zfp::stream_rtell(m_handle.get());
}

void sperr::ZFP_bitstream::rseek(size_t offset)
{
  zfp::stream_rseek(m_handle.get(), offset);
}

auto sperr::ZFP_bitstream::test_range(size_t start_pos, size_t range_len) -> bool
{
  return zfp::test_range(m_handle.get(), start_pos, range_len);
}

// Functions for write
auto sperr::ZFP_bitstream::wtell() const -> size_t
{
  return zfp::stream_wtell(m_handle.get());
}

void sperr::ZFP_bitstream::wseek(size_t offset)
{
  return zfp::stream_wseek(m_handle.get(), offset);
}

auto sperr::ZFP_bitstream::flush() -> size_t
{
  return zfp::stream_flush(m_handle.get());
}

void sperr::ZFP_bitstream::m_wgrow_buf()
{
  const auto curr_pos = zfp::stream_wtell(m_handle.get());
  zfp::stream_flush(m_handle.get());

  m_buf.push_back(0ul);
  m_buf.resize(m_buf.capacity());
  m_capacity = m_buf.size() * zfp::stream_get_wsize();
  m_handle.reset(zfp::stream_open(m_buf.data(), m_buf.size() * sizeof(uint64_t)));

  zfp::stream_wseek(m_handle.get(), curr_pos);
}
