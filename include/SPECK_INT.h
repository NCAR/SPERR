#ifndef SPECK_INT_H
#define SPECK_INT_H

// This is the base class of 1D, 2D, and 3D integer SPECK implementations.

#include "sperr_helper.h"

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
  virtual void use_coeffs(veci_t coeffs, vecb_type signs);
  virtual void use_bitstream(const vec8_type&);

  // Output
  virtual auto view_encoded_bitstream() const -> const vec8_type&;
  virtual auto release_encoded_bitstream() -> vec8_type&&;
  virtual auto release_coeffs() -> veci_t&&;
  virtual auto release_signs() -> vecb_type&&;
  virtual auto view_coeffs() const -> const veci_t&;
  virtual auto view_signs() const -> const vecb_type&;

 protected:
  // Core SPECK procedures
  virtual void m_clean_LIS() = 0;
  virtual void m_initialize_lists() = 0;
  virtual void m_sorting_pass() = 0;
  virtual void m_refinement_pass() = 0;

  // Misc. procedures
  virtual void m_assemble_bitstream();
  //virtual auto m_ready_to_encode() const -> RTNType;
  //virtual auto m_ready_to_decode() const -> RTNType;

  // Data members
  dims_type m_dims = {0, 0, 0};
  int_t m_threshold = 0;
  veci_t m_coeff_buf;
  vec8_type m_encoded_bitstream;
  vecb_type m_bit_buffer, m_LSP_mask, m_sign_array;
  std::vector<uint64_t> m_LIP, m_LSP_new;
  uint8_t m_num_bitplanes = 0;

  const size_t m_u64_garbage_val = std::numeric_limits<size_t>::max();
  const size_t m_header_size = 9; // 9 bytes

};  // class SPECK_INT

};  // namespace sperr

#endif
