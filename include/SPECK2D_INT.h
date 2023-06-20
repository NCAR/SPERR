#ifndef SPECK2D_INT_H
#define SPECK2D_INT_H

#include "SPECK_INT.h"

namespace sperr {

class Set2D {
 public:
  uint32_t start_x = 0;
  uint32_t start_y = 0;
  uint32_t length_x = 0;
  uint32_t length_y = 0;
  uint16_t part_level = 0;
  SetType type = SetType::TypeS;

 public:
  auto is_pixel() const -> bool;
  auto is_empty() const -> bool;
};

//
// Main SPECK2D_INT class; intended to be the base class of both encoder and decoder.
//
template <typename T>
class SPECK2D_INT : public SPECK_INT<T> {
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
  auto m_partition_S(Set2D) const -> std::array<Set2D, 4>;
  //auto m_partition_I() const -> std::array<Set2D, 3>;
  //void m_initialize_lists() override;

  //
  // SPECK2D_INT specific data members
  //
  std::vector<std::vector<Set2D>> m_LIS;
  Set2D m_I = {0, 0, 0, 0, 0, SetType::TypeI};
};

};  // namespace sperr

#endif
