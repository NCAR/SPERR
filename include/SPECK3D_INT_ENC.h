#ifndef SPECK3D_INT_ENC_H
#define SPECK3D_INT_ENC_H

#include "sperr_helper.h"

using int_t = uint64_t;
using veci_t = std::vector<int_t>;

namespace sperr {

//
// Main SPECK3D_INT class
//
class SPECK3D_INT_ENC {
 public:
  // Input and Output
  auto take_input(veci_t&& coeffs, vecb_type&& signs, dims_type) -> RTNType;

  // core operations
  void encode();

 private:
  // auto m_ready_to_encode() const -> bool;
  void m_clean_LIS();
  void m_initialize_sets_lists();
  void m_sorting_pass();
  void m_refinement_pass();

  // For the following 6 methods, indices are used to locate which set to
  // process from m_LIS, Note that when process_S or process_P is called from a
  // code_S routine, code_S will pass in a counter to record how many subsets
  // are discovered significant already. That counter, however, doesn't do
  // anything if process_S or process_P is called from the sorting pass.
  void m_process_S(size_t idx1, size_t idx2, SigType, size_t& counter, bool);
  void m_process_P(size_t idx, SigType, size_t& counter, bool);
  void m_code_S(size_t idx1, size_t idx2, std::array<SigType, 8>);

  // Decide if a set is significant or not.
  // If it is significant, also identify the point that makes it significant.
  auto m_decide_significance(const Set3D&) const -> std::pair<SigType, std::array<uint32_t, 3>>;

  //
  // Private data members
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
