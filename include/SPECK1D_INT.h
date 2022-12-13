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
class SPECK1D_INT : public SPECK_INT {
 public:
  // Virtual destructor
  virtual ~SPECK1D_INT() = default;

 protected:
  virtual void m_clean_LIS() override;
  virtual void m_initialize_lists() override;
  auto m_partition_set(const Set1D&) const -> std::array<Set1D, 2>;

  //
  // SPECK1D_INT specific data members
  //
  std::vector<std::vector<Set1D>> m_LIS;
};

};  // namespace sperr

#endif
