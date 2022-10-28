#ifndef SPECK3D_INT_H
#define SPECK3D_INT_H

#include "sperr_helper.h"

using int_t = uint64_t;
using veci_t = std::vector<int_t>;

namespace sperr {

//
// Main SPECK3D_INT class; intended to be the base class of both encoder and decoder.
//
class SPECK3D_INT {
 public:

 protected:
  // auto m_ready_to_encode() const -> bool;
  void m_clean_LIS();
  void m_initialize_sets_lists();

  //
  // Data members
  //
  std::vector<std::vector<Set3D>> m_LIS;
  std::vector<uint64_t> m_LIP, m_LSP_new;
  vecb_type m_bit_buffer, m_LSP_mask, m_sign_array;
  dims_type m_dims = {0, 0, 0};  // Dimension of the 2D/3D volume
  veci_t m_coeff_buf;            // Quantized wavelet coefficients
  int_t m_threshold = 0;

  const size_t m_u64_garbage_val = std::numeric_limits<size_t>::max();
};

};  // namespace sperr

#endif
