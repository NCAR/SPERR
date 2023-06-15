#ifndef BITMASK_H
#define BITMASK_H

/*
 * Bitmask is intended and optimized for 1) random reads and writes, and 2) bulk reads
 *   and writes, e.g., reading and writing 64 bits at a tiem.
 *   For heavy use of streaming reads and writes, please use the Bitstream class instead.
 *
 * Methods read_long(idx) and write_long(idx, value) are used for bulk reads and writes of
 *   64 bits. The idx parameter is the position of bits, specifying where the long integer is
 *   read or written. As a result, read_long(0) and read_long(63) will return the same integer,
 *   and write_long(64, val) and write_long(127, val) will write val to the same location.
 *
 * Bitmask does not automatically adjust its size. The size of a Bitmask is initialized
 *   at construction time, and is only changed by users calling the resize() method.
 *   The current size of a Bitmask can be queried by the size() method.
 */

#include <cstddef>
#include <cstdint>
#include <vector>

namespace sperr {

class Bitmask {
 public:
  // Constructor
  //
  Bitmask(size_t nbits = 0);  // How many bits does it hold initially?

  // Functions for both read and write
  //
  auto size() const -> size_t;  // Num. of useful bits in this mask.
  void resize(size_t nbits);    // Resize to hold n bits.
  void reset();                 // Set the current bitmask to be all 0's.
  void reset_true();            // Set the current bitmask to be all 1's.

  // Functions for read
  //
  auto read_long(size_t idx) const -> uint64_t;
  auto read_bit(size_t idx) const -> bool;

  // Functions for write
  //
  void write_long(size_t idx, uint64_t value);
  void write_bit(size_t idx, bool bit);
  void write_true(size_t idx);   // This is faster than `write_bit(idx, true)`.
  void write_false(size_t idx);  // This is faster than `write_bit(idx, false)`.

  // Functions for direct access of the underlying data buffer
  // Note: `use_bitstream()` reads the number of values (uint64_t type) that provide
  //       enough bits for the specified size of this mask.
  //
  auto view_buffer() const -> const std::vector<uint64_t>&;
  void use_bitstream(const void* p);

  // Compare if two Bitmasks are identical.
  //
#if __cplusplus >= 202002L
  auto operator==(const Bitmask& rhs) const -> bool = default;
#endif

 private:
  std::vector<uint64_t> m_buf;
  size_t m_num_bits = 0;
};

};  // namespace sperr

#endif
