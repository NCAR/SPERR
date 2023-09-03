#ifndef SPECK1D_INT_H
#define SPECK1D_INT_H

#include "SPECK_INT.h"

namespace sperr {

class Set1D {
 public:
  uint64_t start = 0;
  uint64_t length = 0;
  uint16_t part_level = 0;
  SetType type = SetType::TypeS;  // Only used to indicate garbage status
};

//
// Main SPECK1D_INT class; intended to be the base class of both encoder and decoder.
//
template <typename T>
class SPECK1D_INT : public SPECK_INT<T> {
 protected:
  //
  // Bring members from the base class to this derived class.
  //
  using SPECK_INT<T>::m_LIP_mask;
  using SPECK_INT<T>::m_LSP_new;
  using SPECK_INT<T>::m_dims;
  using SPECK_INT<T>::m_coeff_buf;

  // The 1D case is different from 3D and 2D cases in that it implements additional logic that
  //    infers the significance of subsets by where the significant point is. With this
  //    consideration, functions such as m_process_S() and m_process_P() have different signatures
  //    during decoding/encoding, so they're implemented in their respective subclasses.
  //
  void m_clean_LIS() override;
  void m_initialize_lists() override;
  auto m_partition_set(const Set1D&) const -> std::array<Set1D, 2>;

  //
  // SPECK1D_INT specific data members
  //
  std::vector<std::vector<Set1D>> m_LIS;
};

};  // namespace sperr

#endif
