#ifndef BITSTREAM_H
#define BITSTREAM_H

#include "zfp_bitstream.h"

#include <cassert>
#include <cstdint>
#include <vector>

namespace sperr {

class Bitstream {
 public:
  // Constructor and destructor
  //
  // How many bits does it hold initially?
  Bitstream(size_t nbits = 64);
  ~Bitstream();
  Bitstream(const Bitstream& other) = delete;
  Bitstream(Bitstream&& other) = delete;
  Bitstream& operator=(const Bitstream& other) = delete;
  Bitstream& operator=(Bitstream&& other) = delete;

  // Functions for both read and write
  void rewind();
  auto capacity() const -> size_t;
  void reserve(size_t nbits);

  // Functions for read
  auto rtell() const -> size_t;
  void rseek(size_t offset);
  auto stream_read_n_bits(size_t n) -> uint64_t;
  auto stream_read_bit() -> bool;
  auto random_read_bit(size_t pos) const -> bool;
  auto test_range(size_t start_pos, size_t range_len) -> bool;

  // Functions for write
  //   All write functions won't flush, except for `random_write_bit()`.
  auto wtell() const -> size_t;
  void wseek(size_t offset);
  auto flush() -> size_t;
  auto stream_write_bit(bool bit) -> bool;
  auto stream_write_n_bits(uint64_t value, size_t n) -> uint64_t;
  auto random_write_bit(bool bit, size_t pos) -> bool;  // will effectively flush upon every call.

  // Functions that provide or parse a compact bitstream
  auto get_bitstream(size_t num_bits) -> std::vector<std::byte>;
  void parse_bitstream(const void* p, size_t num_bits);

 private:
  struct bitstream* m_handle = nullptr;

  std::vector<uint64_t> m_buf;
  const size_t m_wsize = 64;
  size_t m_capacity = 0;

  // Grows the buffer in write mode
  void m_wgrow_buf();
};

};  // namespace sperr

#endif
