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
  virtual void decode() override;

 private:
  virtual void m_sorting_pass() override;
  virtual void m_refinement_pass() override;

  void m_process_S(size_t idx1, size_t idx2, size_t& counter, bool);
  void m_process_P(size_t idx, size_t& counter, bool);
  void m_code_S(size_t idx1, size_t idx2);

  // Data members
  std::vector<bool>::const_iterator  m_bit_itr;
};

};  // namespace sperr

#endif
