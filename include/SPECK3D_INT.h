#ifndef SPECK3D_INT_H
#define SPECK3D_INT_H

#include "sperr_helper.h"

namespace sperr {

class Set3D {
 public:
  //
  // Member data
  //
  uint32_t start_x = 0;
  uint32_t start_y = 0;
  uint32_t start_z = 0;
  uint32_t length_x = 0;
  uint32_t length_y = 0;
  uint32_t length_z = 0;
  // which partition level is this set at (starting from zero, in all 3
  // directions). This data member is the sum of all 3 partition levels.
  uint16_t part_level = 0;
  SigType signif = SigType::Insig;
  SetType type = SetType::TypeS;  // Only used to indicate garbage status

 public:
  //
  // Member functions
  //
  auto is_pixel() const -> bool;
  auto is_empty() const -> bool;
};

//
// Main SPECK3D_INT class; intended to be the base class of both encoder and decoder.
//
class SPECK3D_INT {
 public:

  void set_dims(dims_type);
  auto get_speck_full_len(const void*) const -> uint64_t;

 protected:
  // auto m_ready_to_encode() const -> bool;
  void m_clean_LIS();
  void m_initialize_sets_lists();

  // Divide a Set3D into 8, 4, or 2 smaller subsets.
  auto m_partition_S_XYZ(const Set3D&) -> std::array<Set3D, 8>;
  auto m_partition_S_XY(const Set3D&) -> std::array<Set3D, 4>;
  auto m_partition_S_Z(const Set3D&) -> std::array<Set3D, 2>;
  //
  // Data members
  //
  std::vector<std::vector<Set3D>> m_LIS;
  std::vector<uint64_t> m_LIP, m_LSP_new;
  vecb_type m_bit_buffer, m_LSP_mask, m_sign_array;
  vec8_type m_encoded_bitstream;
  dims_type m_dims = {0, 0, 0};  // Dimension of the 2D/3D volume
  veci_t m_coeff_buf;            // Quantized wavelet coefficients
  int_t m_threshold = 0;

  const size_t m_u64_garbage_val = std::numeric_limits<size_t>::max();
  const size_t m_header_size = 9; // 9 bytes
};

};  // namespace sperr

#endif
