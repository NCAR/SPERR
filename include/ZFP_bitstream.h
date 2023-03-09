#ifndef ZFP_BITSTREAM_H
#define ZFP_BITSTREAM_H

#include "bitstream.inl"

#include <cassert>
#include <memory>
#include <vector>

namespace sperr {

class ZFP_bitstream {
 public:
  // Constructor
  // How many bits does it hold initially?
  ZFP_bitstream(size_t nbits = 1024);

  // Functions for both read and write
  void rewind();
  auto capacity() const -> size_t;

  // Functions for read
  auto rtell() const -> size_t;
  void rseek(size_t offset);
  auto stream_read_n_bits(size_t n) -> uint64_t;
  auto stream_read_bit() -> bool { return zfp::stream_read_bit(m_handle.get()); }
  auto random_read(size_t pos) const -> bool { return zfp::random_read(m_handle.get(), pos); }
  auto test_range(size_t start_pos, size_t range_len) -> bool;

  // Functions for write
  // All write functions won't flush, except for `random_write()`.
  auto wtell() const -> size_t;
  void wseek(size_t offset);
  auto flush() -> size_t;  // See ZFP API for return value meaning
  auto stream_write_bit(bool bit) -> bool
  {
    if (zfp::stream_wtell(m_handle.get()) == m_capacity)
      m_wgrow_buf();
    return zfp::stream_write_bit(m_handle.get(), bit);
  }
  auto stream_write_n_bits(uint64_t value, size_t n) -> uint64_t
  {
    assert(n <= 64);
    if (zfp::stream_wtell(m_handle.get()) + n > m_capacity)
      m_wgrow_buf();
    return zfp::stream_write_bits(m_handle.get(), value, n);
  }
  auto random_write(bool bit, size_t pos) -> bool // will effectively flush upon every call.
  {
    return zfp::random_write(m_handle.get(), bit, pos);
  }

 private:
  std::unique_ptr<zfp::bitstream, decltype(&zfp::stream_close)> m_handle = {nullptr,
                                                                            &zfp::stream_close};
  std::vector<uint64_t> m_buf;

  size_t m_capacity = 0;

  // Grows the buffer in write mode
  void m_wgrow_buf();
};

};  // namespace sperr

#endif
