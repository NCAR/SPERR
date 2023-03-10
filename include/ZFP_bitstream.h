#ifndef ZFP_BITSTREAM_H
#define ZFP_BITSTREAM_H

#include "bitstream.inl"

#include <cassert>
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
  auto stream_read_bit() -> bool { return zfp::stream_read_bit(&m_handle); }
  auto random_read_bit(size_t pos) const -> bool { return zfp::random_read_bit(&m_handle, pos); }
  auto test_range(size_t start_pos, size_t range_len) -> bool;

  // Functions for write
  //   All write functions won't flush, except for `random_write_bit()`.
  auto wtell() const -> size_t;
  void wseek(size_t offset);
  auto flush() -> size_t;
  auto stream_write_bit(bool bit) -> bool
  {
    if (zfp::stream_wtell(&m_handle) == m_capacity)
      m_wgrow_buf();
    return zfp::stream_write_bit(&m_handle, bit);
  }
  auto stream_write_n_bits(uint64_t value, size_t n) -> uint64_t
  {
    assert(n <= 64);
    if (zfp::stream_wtell(&m_handle) + n > m_capacity)
      m_wgrow_buf();
    return zfp::stream_write_bits(&m_handle, value, n);
  }
  auto random_write_bit(bool bit, size_t pos) -> bool;  // will effectively flush upon every call.

  // Functions that provide or parse a compact bitstream
  auto get_bitstream(size_t num_bits) -> std::vector<std::byte>;
  void parse_bitstream(const void* p, size_t num_bits);

 private:
  zfp::bitstream m_handle;
  std::vector<uint64_t> m_buf;

  size_t m_capacity = 0;

  // Grows the buffer in write mode
  void m_wgrow_buf();
};

};  // namespace sperr

#endif
