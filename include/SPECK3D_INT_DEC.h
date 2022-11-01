#ifndef SPECK3D_INT_DEC_H
#define SPECK3D_INT_DEC_H

#include "SPECK3D_INT.h"

namespace sperr {

//
// Main SPECK3D_INT_DEC class
//
class SPECK3D_INT_DEC : public SPECK3D_INT {
 public:
  // core operations
  void decode();

  // Input and output
  void use_bitstream(const vec8_type&);
  auto release_coeffs() -> veci_t&&;
  auto release_signs() -> vecb_type&&;

 private:
  //auto m_ready_to_decode() const -> bool;
  void m_sorting_pass();
  void m_refinement_pass();

  void m_process_S(size_t idx1, size_t idx2, size_t& counter, bool);
  void m_process_P(size_t idx, size_t& counter, bool);
  void m_code_S(size_t idx1, size_t idx2);

  // Data members
  std::vector<bool>::const_iterator  m_bit_itr;
};

};  // namespace sperr

#endif
