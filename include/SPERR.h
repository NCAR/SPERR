//
// This class implements the SPECK encoder for errors, dubbed as SPERR.
//

#ifndef SPERR_H
#define SPERR_H

#include "sperr_helper.h"

namespace sperr {

//
// Auxiliary class to hold a 1D SPECK Set
//
class SPECKSet1D {
 public:
  size_t start = 0;
  size_t length = 0;
  uint32_t part_level = 0;
  SetType type = SetType::TypeS;  // only to indicate if it's garbage
};

//
// Auxiliary class to represent an outlier
//
class Outlier {
 public:
  size_t location = 0;
  double error = 0.0;

  Outlier() = default;
  Outlier(size_t, double);
};

class SPERR {
 public:
  // Input
  //
  // Important note on the outliers: each one must live at a unique location,
  // and each error value must be greater than or equal to the tolerance.
  void add_outlier(size_t, double);                     // add a single outlier.
                                                        // Does not affect existing outliers.
  void copy_outlier_list(const std::vector<Outlier>&);  // Copy a given list of outliers.
                                                        // Existing outliers are erased.
  void take_outlier_list(std::vector<Outlier>&&);  // Take ownership of a given list of outliers.
                                                   // Existing outliers are erased.
  void set_length(uint64_t);                       // set 1D array length
  void set_tolerance(double);                      // set error tolerance (Must be positive)

  // Output
  auto release_outliers() -> std::vector<Outlier>&&;  // Release ownership of decoded outliers
  auto view_outliers() -> const std::vector<Outlier>&;
  auto get_encoded_bitstream() -> std::vector<uint8_t>;
  auto parse_encoded_bitstream(const void*, size_t) -> RTNType;

  // Given a SPERR stream, tell how long the sperr stream is.
  auto get_sperr_stream_size(const void*) const -> uint64_t;

  // Action methods
  auto encode() -> RTNType;
  auto decode() -> RTNType;

 private:
  //
  // Private methods
  //
  auto m_part_set(const SPECKSet1D&) const -> std::array<SPECKSet1D, 2>;
  void m_initialize_LIS();
  void m_clean_LIS();
  auto m_ready_to_encode() const -> bool;
  auto m_ready_to_decode() const -> bool;

  // If the set to be decided is significant, return a pair containing true and
  // the index in the outlier list that makes it significant.
  // If not, return a pair containing false and zero.
  auto m_decide_significance(const SPECKSet1D&) const -> std::pair<bool, size_t>;

  // Encoding methods
  void m_process_S_encoding(size_t idx1, size_t idx2, size_t& counter, bool output);
  void m_refinement_pass_encoding();
  void m_sorting_pass();          // Used in both encoding and decoding
  void m_code_S(size_t, size_t);  // Used in both encoding and decoding

  // Decoding methods
  void m_process_S_decoding(size_t idx1, size_t idx2, size_t& counter, bool input);
  void m_refinement_pass_decoding();

  //
  // Private data members
  //
  uint64_t m_total_len = 0;  // 1D array length
  double m_tolerance = 0.0;  // Error tolerance.
  float m_max_threshold_f = 0.0;
  const size_t m_header_size = 20;

  double m_threshold = 0.0;   // Threshold that's used for quantization
  bool m_encode_mode = true;  // Encode (true) or Decode (false) mode?
  size_t m_bit_idx = 0;       // decoding only. Which bit we're at?
  size_t m_LOS_size = 0;      // decoding only. Size of `m_LOS` at the end of an iteration.
  size_t m_num_itrs = 0;      // encoding only. Number of iterations.

  std::vector<bool> m_bit_buffer;
  std::vector<double> m_q;     // encoding only. This list is refined in the refinement pass.
  std::vector<Outlier> m_LOS;  // List of OutlierS. This list is not altered when encoding,
                               // but constantly updated when decoding.

  std::vector<size_t> m_LSP_new;        // encoding only
  std::vector<size_t> m_LSP_old;        // encoding only
  std::vector<bool> m_sig_map;          // encoding only
  std::vector<bool> m_recovered_signs;  // decoding only

  std::vector<std::vector<SPECKSet1D>> m_LIS;
};

};  // namespace sperr

#endif
