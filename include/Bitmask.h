#ifndef BITMASK_H
#define BITMASK_H

#include <cstddef>
#include <cstdint>
#include <vector>

namespace sperr {

class Bitmask {
 public:
  // Constructor
  Bitmask(size_t nbits = 0);  // How many bits does it hold initially?

  // Functions for both read and write
  //
  auto size() const -> size_t;  // Report the current size (in num of bits).
  void resize(size_t nbits);    // Resize to hold n bits.
  void reset();                 // Set the current bitmask to be all 0s.

  // Functions for read
  //
  // Return the long (uint64_t) that contains the bit at the requested idx.
  auto read_long(size_t idx) const -> uint64_t;
  auto read_bit(size_t idx) const -> bool;

  // Functions for write
  //
  // Over-writes the long (uint64_t) that contains the bit at `idx` with `value`.
  void write_long(size_t idx, uint64_t value);
  void write_bit(size_t idx, bool bit);

 private:
  std::vector<uint64_t> m_buf;
  size_t m_num_bits = 0;  // The actual number of bits in use; not necessarily 64 * m_buf.size()
};

};  // namespace sperr

#endif
