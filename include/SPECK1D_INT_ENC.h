#ifndef SPECK1D_INT_ENC_H
#define SPECK1D_INT_ENC_H

#include "SPECK1D_INT.h"

namespace sperr {

//
// Main SPECK1D_INT_ENC class
//
class SPECK1D_INT_ENC : public SPECK1D_INT {
 public:
  // core operations
  virtual void encode() override;

 private:
  virtual void m_sorting_pass() override;
  virtual void m_refinement_pass() override;

  void m_process_S(size_t idx1, size_t idx2, SigType, size_t& counter, bool);
  void m_process_P(size_t idx, SigType, size_t& counter, bool);
  void m_code_S(size_t idx1, size_t idx2, std::array<SigType, 2>);

  // Decide if a set is significant or not.
  // If it is significant, also identify the point that makes it significant.
  auto m_decide_significance(const Set1D&) const -> std::pair<SigType, int64_t>;
};

};  // namespace sperr

#endif
