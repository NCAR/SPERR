#ifndef SPECK3D_H
#define SPECK3D_H

#include "SPECK_Storage.h"

namespace sperr {

//
// Auxiliary class to hold a 3D SPECK Set
//
class SPECKSet3D {
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
  // which partition level is this set at (starting from zero, in all 3
  // directions). This data member is the sum of all 3 partition levels.
  uint16_t part_level = 0;
  SigType signif = SigType::Insig;
  SetType type = SetType::TypeS;  // Only used to indicate garbage status

 public:
  //
  // Member functions
  //
  auto is_pixel() const -> bool;
  auto is_empty() const -> bool;
};

//
// Main SPECK3D class
//
class SPECK3D : public SPECK_Storage {
 public:
  // core operations
  auto encode() -> RTNType;
  auto decode() -> RTNType;

 private:
  auto m_ready_to_encode() const -> bool;
  auto m_ready_to_decode() const -> bool;
  void m_clean_LIS();
  void m_initialize_sets_lists();
  auto m_sorting_pass_encode() -> RTNType;
  auto m_sorting_pass_decode() -> RTNType;

  // For the following 6 methods, indices are used to locate which set to
  // process from m_LIS, Note that when process_S or process_P is called from a
  // code_S routine, code_S will pass in a counter to record how many subsets
  // are discovered significant already. That counter, however, doesn't do
  // anything if process_S or process_P is called from the sorting pass.
  auto m_process_S_encode(size_t idx1, size_t idx2, SigType, size_t& counter, bool) -> RTNType;
  auto m_code_S_encode(size_t idx1, size_t idx2, std::array<SigType, 8>) -> RTNType;
  auto m_process_P_encode(size_t idx, SigType, size_t& counter, bool) -> RTNType;

  auto m_process_S_decode(size_t idx1, size_t idx2, size_t& counter, bool) -> RTNType;
  auto m_code_S_decode(size_t idx1, size_t idx2) -> RTNType;
  auto m_process_P_decode(size_t idx, size_t& counter, bool) -> RTNType;

  // Divide a SPECKSet3D into 8, 4, or 2 smaller subsets.
  auto m_partition_S_XYZ(const SPECKSet3D&) const -> std::array<SPECKSet3D, 8>;
  auto m_partition_S_XY(const SPECKSet3D&) const -> std::array<SPECKSet3D, 4>;
  auto m_partition_S_Z(const SPECKSet3D&) const -> std::array<SPECKSet3D, 2>;

  // Decide if a set is significant or not.
  // If it is significant, also identify the pixel that makes it significant.
  auto m_decide_significance(const SPECKSet3D&) const
      -> std::pair<SigType, std::array<uint32_t, 3>>;

  //
  // Private data members
  //
  std::vector<std::vector<SPECKSet3D>> m_LIS;
};

};  // namespace sperr

#endif
