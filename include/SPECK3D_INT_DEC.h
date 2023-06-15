#ifndef SPECK3D_INT_DEC_H
#define SPECK3D_INT_DEC_H

#include "SPECK3D_INT.h"

namespace sperr {

//
// Main SPECK3D_INT_DEC class
//
template <typename T>
class SPECK3D_INT_DEC : public SPECK3D_INT<T> {
 private:
  //
  // Bring members from parent classes to this derived class.
  //
  using SPECK_INT<T>::m_LIP_mask;
  using SPECK_INT<T>::m_dims;
  using SPECK_INT<T>::m_LSP_new;
  using SPECK_INT<T>::m_bit_idx;
  using SPECK_INT<T>::m_threshold;
  using SPECK_INT<T>::m_coeff_buf;
  using SPECK_INT<T>::m_bit_buffer;
  using SPECK_INT<T>::m_sign_array;
  using SPECK3D_INT<T>::m_LIS;
  using SPECK3D_INT<T>::m_partition_S_XYZ;

  void m_sorting_pass() override;

  void m_process_S(size_t idx1, size_t idx2, size_t& counter, bool);
  void m_process_P(size_t idx, size_t& counter, bool);
  void m_code_S(size_t idx1, size_t idx2);
};

};  // namespace sperr

#endif
