#ifndef SPECK3D_INT_H
#define SPECK3D_INT_H

#include "SPECK_INT.h"

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

  // What's the offset of this set in a morton organized storage?
  uint64_t morton_offset = 0;

  // Which partition level is this set at (starting from zero, in all 3 directions).
  // This data member is the sum of all 3 partition levels.
  uint16_t part_level = 0;
  SetType type = SetType::TypeS;  // Only used to indicate garbage status

 public:
  //
  // Member functions
  //
  auto is_pixel() const -> bool;
  auto is_empty() const -> bool;
  auto num_elem() const -> size_t;
};

//
// Main SPECK3D_INT class; intended to be the base class of both encoder and decoder.
//
template <typename T>
class SPECK3D_INT : public SPECK_INT<T> {
 protected:
  //
  // Bring members from the base class to this derived class.
  //
  using SPECK_INT<T>::m_LIP_mask;
  using SPECK_INT<T>::m_dims;
  using SPECK_INT<T>::m_LSP_new;
  using SPECK_INT<T>::m_coeff_buf;
  using SPECK_INT<T>::m_bit_buffer;

  void m_clean_LIS() override;
  void m_initialize_lists() override;

  // Divide a Set3D into 8, 4, or 2 smaller subsets.
  auto m_partition_S_XYZ(const Set3D&) const -> std::array<Set3D, 8>;
  auto m_partition_S_XY(const Set3D&) const -> std::array<Set3D, 4>;
  auto m_partition_S_Z(const Set3D&) const -> std::array<Set3D, 2>;

  //
  // SPECK3D_INT specific data members
  //
  std::vector<std::vector<Set3D>> m_LIS;

  // Experiment with morton curves.
  bool m_morton_valid = false;
};

};  // namespace sperr

#endif
