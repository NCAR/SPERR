#ifndef BITMASK_H
#define BITMASK_H

/*
 * Bitmask is intended and optimized for 1) random reads and writes, and 2) bulk reads
 *   and writes, e.g., reading and writing 64 bits at a tiem.
 *   For heavy use of streaming reads and writes, please use the Bitstream class instead.
 *
 * Methods rint(idx) and wint(idx, value) are used for bulk reads and writes of
 *   64 bits. The idx parameter is the position of bits, specifying where the integer is
 *   read or written. As a result, rint(0) and rint(63) will return the same integer,
 *   and wint(64, val) and wint(127, val) will write val to the same location, assuming
 *   that 64-bit unsigned long is used as the underlying integer type.
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

  // Size related functions
  //
  auto size() const -> size_t;  // Num. of useful bits in this mask.
  void resize(size_t nbits);    // Resize to hold n bits.

  // Functions for read
  //
  auto rint(size_t idx) const -> uint64_t;  // `idx` of the bit, not the int.
  auto rbit(size_t idx) const -> bool;

  // Functions to perform bulk tests.
  auto has_true(size_t start, size_t len) const -> bool;  // Is there any true in this range?
  auto count_true() const -> size_t;                      // How many 1's in this mask?

  // Functions for write
  //
  void wbit(size_t idx, bool bit);
  void wint(size_t idx, uint64_t value);  // `idx` of the bit, not the int.
  void wtrue(size_t idx);                  // This is faster than `wbit(idx, true)`.
  void wfalse(size_t idx);                 // This is faster than `wbit(idx, false)`.
  void reset();                            // Set the current bitmask to be all 0's.
  void reset_true();                       // Set the current bitmask to be all 1's.

  // Functions for direct access of the underlying data buffer
  // Note: `use_bitstream()` reads the number of values (uint64_t type) that provide
  //       enough bits for the specified size of this mask.
  //
  auto view_buffer() const -> const std::vector<uint64_t>&;
  void use_bitstream(const void* p);

#if defined __cpp_lib_three_way_comparison && defined __cpp_impl_three_way_comparison
  auto operator<=>(const Bitmask& rhs) const noexcept;
  auto operator==(const Bitmask& rhs) const noexcept -> bool;
#endif

 private:
  size_t m_width = 64;
  size_t m_num_bits = 0;
  std::vector<uint64_t> m_buf;
};

};  // namespace sperr

#endif
