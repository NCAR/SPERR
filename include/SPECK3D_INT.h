#ifndef SPECK3D_INT_H
#define SPECK3D_INT_H

#include "SPECK_INT.h"

namespace sperr {

class Set3D {
 public:
  //
  // Member data
  //
  uint32_t start_x = 0;
  uint32_t start_y = 0;
  uint32_t start_z = 0;
  uint32_t length_x = 0;
  uint32_t length_y = 0;
  uint32_t length_z = 0;
  // which partition level is this set at (starting from zero, in all 3 directions). 
  // This data member is the sum of all 3 partition levels.
  uint16_t part_level = 0;
  SetType type = SetType::TypeS;  // Only used to indicate garbage status

 public:
  //
  // Member functions
  //
  auto is_pixel() const -> bool;
  auto is_empty() const -> bool;
};

//
// Main SPECK3D_INT class; intended to be the base class of both encoder and decoder.
//
class SPECK3D_INT : public SPECK_INT {
 public:
  // Virtual destructor
  virtual ~SPECK3D_INT() = default;

 protected:

  virtual void m_clean_LIS() override;
  virtual void m_initialize_lists() override;

  // Divide a Set3D into 8, 4, or 2 smaller subsets.
  auto m_partition_S_XYZ(const Set3D&) -> std::array<Set3D, 8>;
  auto m_partition_S_XY(const Set3D&) -> std::array<Set3D, 4>;
  auto m_partition_S_Z(const Set3D&) -> std::array<Set3D, 2>;

  //
  // SPECK3D_INT specific data members
  //
  std::vector<std::vector<Set3D>> m_LIS;
};

};  // namespace sperr

#endif