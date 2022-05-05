#ifndef SPECK2D_H
#define SPECK2D_H

#include "SPECK_Storage.h"

#include <limits>

namespace sperr {

//
// Auxiliary class to hold a SPECK Set
//
class SPECKSet2D {
 public:
  //
  // Member data
  //
  uint32_t start_x = 0;
  uint32_t start_y = 0;
  uint32_t length_x = 0;    // For typeI set, this value is always m_dims[0].
  uint32_t length_y = 0;    // For typeI set, this value is always m_dims[1].
  uint16_t part_level = 0;  // which partition level is this set at (starting from zero).
  SigType signif = SigType::Insig;
  SetType type = SetType::TypeS;

 public:
  //
  // Member functions
  //
  auto is_pixel() const -> bool;
  auto is_empty() const -> bool;
};

//
// Main SPECK2D class
//
class SPECK2D : public SPECK_Storage {
 public:
  // Constructor
  SPECK2D();

  // trivial input
#ifdef QZ_TERM
  void set_quantization_level(int32_t lev);
#else
  void set_bit_budget(size_t);
#endif

  // core operations
  auto encode() -> RTNType;
  auto decode() -> RTNType;

 private:
  auto m_sorting_pass_encode() -> RTNType;
  auto m_sorting_pass_decode() -> RTNType;
  auto m_process_S_encode(size_t, size_t, size_t&, bool) -> RTNType;
  auto m_process_S_decode(size_t, size_t, size_t&, bool) -> RTNType;
  auto m_process_P_encode(size_t, size_t&, bool) -> RTNType;
  auto m_process_P_decode(size_t, size_t&, bool) -> RTNType;
  auto m_code_S_encode(size_t, size_t) -> RTNType;
  auto m_code_S_decode(size_t, size_t) -> RTNType;
  auto m_process_I(bool) -> RTNType;
  auto m_code_I() -> RTNType;
  void m_initialize_sets_lists();
  auto m_partition_I() -> std::array<SPECKSet2D, 3>;
  auto m_decide_set_S_significance(const SPECKSet2D& set) -> SigType;
  auto m_decide_set_I_significance(const SPECKSet2D& set) -> SigType;
  void m_clean_LIS();
  auto m_produce_root() const -> SPECKSet2D;
  auto m_partition_S(const SPECKSet2D&) const -> std::array<SPECKSet2D, 4>;
  auto m_ready_to_encode() const -> bool;
  auto m_ready_to_decode() const -> bool;

#ifdef QZ_TERM
  void m_quantize_P_encode(size_t idx);
  void m_quantize_P_decode(size_t idx);
#else
  auto m_refinement_pass_encode() -> RTNType;
  auto m_refinement_pass_decode() -> RTNType;
#endif

  //
  // Private data members
  //
  size_t m_bit_idx = 0;       // Used for decode. Which bit we're at?
  bool m_encode_mode = true;  // Encode (true) or Decode (false) mode?

  std::vector<std::vector<SPECKSet2D>> m_LIS;  // List of insignificant sets
  std::vector<size_t> m_LIP;                   // List of insignificant pixels
  SPECKSet2D m_I;

  const size_t m_u64_garbage_val = std::numeric_limits<size_t>::max();
  std::vector<bool> m_sign_array;

#ifdef QZ_TERM
  std::array<double, 64> m_threshold_arr;
  size_t m_threshold_idx = 0;
#else
  double m_threshold = 0.0;       // Threshold that's used for an iteration.
  std::vector<size_t> m_LSP_new;  // List of significant pixels, just identified
  std::vector<size_t> m_LSP_old;  // List of significant pixels, previously identified
  size_t m_budget = 0;            // What's the budget for num of bits in fixed-rate mode?
#endif

};

};  // namespace sperr

#endif
