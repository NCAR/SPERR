#ifndef SPECK_INT_H
#define SPECK_INT_H

//
// This is the base class of 1D, 2D, and 3D integer SPECK implementations.
//

#include "sperr_helper.h"

#include "Bitmask.h"
#include "Bitstream.h"

namespace sperr {

//
// Given a bitstream, one needs to know the integer length needed *before* instantiate a
//    SPECK_INT class to decode the bitstream. Thus, provide this free-standing helper.
//
auto speck_int_get_num_bitplanes(const void* bitstream) -> uint8_t;

//
// Class SPECK_INT
//
template <typename T>
class SPECK_INT {
  using uint_type = T;
  using vecui_type = std::vector<uint_type>;

 public:
  // Constructor and destructor
  SPECK_INT();
  virtual ~SPECK_INT() = default;

  // The length (1, 2, 4, 8) of the integer type in use
  auto integer_len() const -> size_t;

  void set_dims(dims_type);

  // Retrieve info of a SPECK bitstream from its header
  // Note that `speck_int_get_num_bitplanes()` is provided as a free-standing helper function.
  //
  auto get_speck_bits(const void*) const -> uint64_t;
  auto get_stream_full_len(const void*) const -> uint64_t;

  // Actions
  void encode();
  void decode();

  // Clear bit buffer and other data structures.
  void reset();

  // Input
  auto use_coeffs(vecui_type coeffs, vecb_type signs) -> RTNType;
  void use_bitstream(const void* p, size_t len);

  // Output
  void append_encoded_bitstream(vec8_type& buf) const;
  auto release_coeffs() -> vecui_type&&;
  auto release_signs() -> vecb_type&&;
  auto view_coeffs() const -> const vecui_type&;
  auto view_signs() const -> const vecb_type&;

 protected:
  // Core SPECK procedures
  virtual void m_clean_LIS() = 0;
  virtual void m_sorting_pass() = 0;
  virtual void m_initialize_lists() = 0;
  void m_refinement_pass_encode();
  void m_refinement_pass_decode();

  // Data members
  dims_type m_dims = {0, 0, 0};
  uint_type m_threshold = 0;
  Bitmask m_LSP_mask;
  vecui_type m_coeff_buf;
  vecb_type m_sign_array;
  Bitstream m_bit_buffer;
  std::vector<uint64_t> m_LIP, m_LSP_new;

  const uint64_t m_u64_garbage_val = std::numeric_limits<uint64_t>::max();
  const size_t m_header_size = 9;  // 9 bytes

  uint64_t m_bit_idx = 0;     // current bit idx when decoding
  uint64_t m_total_bits = 0;  // keeps track of useful bits in `m_bit_buffer`
  uint8_t m_num_bitplanes = 0;
};

};  // namespace sperr

#endif
