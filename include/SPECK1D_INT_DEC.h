#ifndef SPECK1D_INT_DEC_H
#define SPECK1D_INT_DEC_H

#include "SPECK1D_INT.h"

namespace sperr {

//
// Main SPECK1D_INT_DEC class
//
class SPECK1D_INT_DEC : public SPECK1D_INT {
 public:
  // core operations
  virtual void decode() override;

 private:
  virtual void m_sorting_pass() override;

  void m_process_S(size_t idx1, size_t idx2, size_t& counter, bool read);
  void m_process_P(size_t idx, size_t& counter, bool);
  void m_code_S(size_t idx1, size_t idx2);
};

};  // namespace sperr

#endif
