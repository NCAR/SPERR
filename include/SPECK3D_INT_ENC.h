#ifndef SPECK3D_INT_ENC_H
#define SPECK3D_INT_ENC_H

#include "SPECK3D_INT.h"

#include <optional>

namespace sperr {

//
// Main SPECK3D_INT_ENC class
//
template <typename T>
class SPECK3D_INT_ENC : public SPECK3D_INT<T> {
 private:
  //
  // Bring members from parent classes to this derived class.
  //
  using SPECK_INT<T>::m_LIP_mask;
  using SPECK_INT<T>::m_dims;
  using SPECK_INT<T>::m_LSP_new;
  using SPECK_INT<T>::m_threshold;
  using SPECK_INT<T>::m_coeff_buf;
  using SPECK_INT<T>::m_bit_buffer;
  using SPECK_INT<T>::m_sign_array;
  using SPECK3D_INT<T>::m_LIS;
  using SPECK3D_INT<T>::m_partition_S_XYZ;

  void m_sorting_pass() override;

  void m_process_S(size_t idx1, size_t idx2, SigType, size_t& counter, bool output);
  void m_process_P(size_t idx, SigType, size_t& counter, bool output);
  void m_code_S(size_t idx1, size_t idx2, std::array<SigType, 8>);

  // Decide if a set is significant or not.
  // If it is significant, also identify the point that makes it significant.
  auto m_decide_significance(const Set3D&) const -> std::optional<std::array<uint32_t, 3>>;
};

};  // namespace sperr

#endif
