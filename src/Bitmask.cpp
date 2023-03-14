#include "Bitmask.h"

#include <algorithm>

sperr::Bitmask::Bitmask(size_t nbits)
{
  if (nbits > 0) {
    auto num_longs = nbits / 64;
    if (nbits % 64 != 0)
      num_longs++;
    m_buf.assign(num_longs, 0);
    m_num_bits = num_longs * 64;
  }
}

auto sperr::Bitmask::size() const -> size_t
{
  return m_num_bits;
}

void sperr::Bitmask::resize(size_t nbits) 
{
  auto num_longs = nbits / 64;
  if (nbits % 64 != 0)
    num_longs++;
  m_buf.resize(num_longs);
  m_num_bits = num_longs * 64;
}

void sperr::Bitmask::reset()
{
  std::fill(m_buf.begin(), m_buf.end(), 0);
}

auto sperr::Bitmask::read_long(size_t idx) const -> uint64_t
{
  return m_buf[idx / 64];
}

auto sperr::Bitmask::read_bit(size_t idx) const -> bool
{
  auto word = m_buf[idx / 64];
  word &= uint64_t(1ul) << (idx % 64);
  return (word != 0);
}

void sperr::Bitmask::write_long(size_t idx, uint64_t value)
{
  m_buf[idx / 64] = value;
}

void sperr::Bitmask::write_bit(size_t idx, bool bit)
{
  const auto wstart = idx / 64;

  auto word = m_buf[wstart];
  const auto mask = uint64_t(1ul) << (idx % 64);
  if (bit)
    word |= mask;
  else
    word &= ~mask;
  m_buf[wstart] = word;
}
