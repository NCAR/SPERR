#ifndef SPECK1D_INT_DEC_H
#define SPECK1D_INT_DEC_H

#include "SPECK1D_INT.h"

namespace sperr {

//
// Main SPECK1D_INT_DEC class
//
template <typename T>
class SPECK1D_INT_DEC : public SPECK1D_INT<T> {
 private:
  //
  // Bring members from parent classes to this derived class.
  //
  using SPECK_INT<T>::m_LIP;
  using SPECK_INT<T>::m_LSP_new;
  using SPECK_INT<T>::m_bit_idx;
  using SPECK_INT<T>::m_threshold;
  using SPECK_INT<T>::m_coeff_buf;
  using SPECK_INT<T>::m_bit_buffer;
  using SPECK_INT<T>::m_sign_array;
  using SPECK_INT<T>::m_u64_garbage_val;
  using SPECK1D_INT<T>::m_LIS;
  using SPECK1D_INT<T>::m_partition_set;

  virtual void m_sorting_pass() override;

  void m_process_S(size_t idx1, size_t idx2, size_t& counter, bool read);
  void m_process_P(size_t idx, size_t& counter, bool);
  void m_code_S(size_t idx1, size_t idx2);
};

};  // namespace sperr

#endif