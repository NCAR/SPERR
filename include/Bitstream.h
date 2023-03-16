#ifndef BITSTREAM_H
#define BITSTREAM_H

/*
 * Bitstream is intended and optimized for streaming reads and writes.
 *   It is heavily modeled after the bitstream structure in ZFP:
 *   (https://github.com/LLNL/zfp/blob/develop/include/zfp/bitstream.inl)
 *   For heavy use of random reads and writes, please use the Bitmask class instead.
 *
 * Bitstream uses words of 64 bits in its storage. A buffered word of 64 bits is also
 *   used to speed up stream reads and writes.
 *
 * A few caveats:
 *   1. Random reads CAN be achieved via repeated rseek() and read_bit() calls.
 *      However, it will be much less efficient than the random reads in Bitmask.
 *   2. A function call of wseek() will erase the remaining bits in a word, i.e., 
 *      from the wseek() position to the next word boundary, though the bits up to the wseek()
 *      position will be preserved. This design is for better efficiency of write_bit().
 *   3. Because of 2, true random writes is not possible; it's only possible at the end of
 *      each word, e.g., positions of 63, 127, 191.
 *   4. a function call of flush() will align the writing position to the beginning of the
 *      next word.
 */

#include <cstdint>
#include <vector>

namespace sperr {

class Bitstream {
 public:
  // Constructor
  //
  // How many bits does it hold initially?
  Bitstream(size_t nbits = 64);

  // Functions for both read and write
  void rewind();
  auto capacity() const -> size_t;
  void reserve(size_t nbits);

  // Functions for read
  auto rtell() const -> size_t;
  void rseek(size_t offset);
  auto read_bit() -> bool;

  // Functions for write
  auto wtell() const -> size_t;
  void wseek(size_t offset);
  void write_bit(bool bit);
  void flush();

  // Functions that provide or parse a compact bitstream
  auto get_bitstream(size_t num_bits) -> std::vector<std::byte>;
  void parse_bitstream(const void* p, size_t num_bits);

 private:
  std::vector<uint64_t> m_buf;
  std::vector<uint64_t>::iterator m_itr;  // Pointer to the next word to be read/written.

  uint64_t m_buffer = 0;  // incoming/outgoing bits
  uint32_t m_bits = 0;    // number of buffered bits
};

};  // namespace sperr

#endif
