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

  // The 2D case is different from 3D and 1D cases in that it doesn't implement the logic that
  //    infers the significance of subsets by where the significant point is. I.e., the decide
  //    set significance functions return only true/false, but not the index of the first 
  //    significant point. With this simplification, the m_process_S()/m_process_P()/m_process_I()
  //    functions have the same signature, so they can be virtual, and m_sorting_pass()/m_code_S/
  //    m_code_I() are really the same when encoding and decoding, so they can be implemented
  //    here instead of in SPECK2D_INT_ENC/SPECK2D_INT_DEC.
  //
  void m_sorting_pass() override;
  void m_code_S(size_t idx1, size_t idx2);
  void m_code_I();

  virtual void m_process_S(size_t idx1, size_t idx2, size_t& counter, bool need_decide) = 0;
  virtual void m_process_P(size_t idx, size_t& counter, bool need_decide) = 0;
  virtual void m_process_I(bool need_decide) = 0;

  void m_clean_LIS() override;
  auto m_partition_S(Set2D) const -> std::array<Set2D, 4>;
  auto m_partition_I() -> std::array<Set2D, 3>;
  void m_initialize_lists() override;

  //
  // SPECK2D_INT specific data members
  //
  std::vector<std::vector<Set2D>> m_LIS;
  Set2D m_I = {0, 0, 0, 0, 0, SetType::TypeI};
};

};  // namespace sperr

#endif
