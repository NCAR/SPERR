#ifndef SPECK_INT_H
#define SPECK_INT_H

// This is the base class of 1D, 2D, and 3D integer SPECK implementations.

#include "sperr_helper.h"

#include "Bitmask.h"
#include "Bitstream.h"

namespace sperr {

class SPECK_INT {
 public:
  // Virtual destructor
  virtual ~SPECK_INT() = default;

  void set_dims(dims_type);

  // Retrieve the full length of a SPECK bitstream from its header
  auto get_speck_full_len(const void*) const -> uint64_t;

  // Actions
  virtual void encode();
  virtual void decode();

  // Input
  virtual void use_coeffs(vecui_t coeffs, vecb_type signs);
  virtual void use_bitstream(const vec8_type&);

  // Output
  virtual void write_encoded_bitstream(vec8_type& buf, size_t offset = 0) const;
  virtual auto release_coeffs() -> vecui_t&&;
  virtual auto release_signs() -> vecb_type&&;
  virtual auto view_coeffs() const -> const vecui_t&;
  virtual auto view_signs() const -> const vecb_type&;

 protected:
  // Core SPECK procedures
  virtual void m_clean_LIS() = 0;
  virtual void m_initialize_lists() = 0;
  virtual void m_sorting_pass() = 0;
  virtual void m_refinement_pass_encode();
  virtual void m_refinement_pass_decode();

  // Data members
  dims_type m_dims = {0, 0, 0};
  uint_t m_threshold = 0;
  vecui_t m_coeff_buf;
  vecb_type m_sign_array;
  Bitstream m_bit_buffer;
  Bitmask m_LSP_mask;
  std::vector<uint64_t> m_LIP, m_LSP_new;

  const size_t m_u64_garbage_val = std::numeric_limits<size_t>::max();
  const size_t m_header_size = 9;  // 9 bytes

  uint64_t m_bit_idx = 0;     // current bit idx when decoding
  uint64_t m_total_bits = 0;  // keeps track of useful bits in `m_bit_buffer`
  uint8_t m_num_bitplanes = 0;
};

};  // namespace sperr

#endif
