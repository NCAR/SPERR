#ifndef SPECK_STORAGE_H
#define SPECK_STORAGE_H

//
// This class implements mechanisms and interfaces to help 2D and 3D SPECK
// classes manage storage of coefficients and bitstreams.
//

#include <limits>
#include <string>
#include <utility>  // std::pair<>
#include <vector>

#include "sperr_helper.h"

namespace sperr {

class SPECK_Storage {
 public:
  //
  // Memory management: input and output
  //
  template <typename T>
  auto copy_data(const T*, size_t len, dims_type dims) -> RTNType;
  auto take_data(vecd_type&&, dims_type dims) -> RTNType;
  auto release_data() -> vecd_type&&;
  auto view_data() const -> const vecd_type&;
  auto view_encoded_bitstream() const -> const vec8_type&;
  auto release_encoded_bitstream() -> vec8_type&&;
  auto release_quantized_coeff() -> vecd_type&&;

  // Prepare internal states for a decompression operation from an encoded bitstream
  // Note: it takes a raw pointer because it accesses memory provided by others,
  //       and others most likely provide a raw pointer.
  auto parse_encoded_bitstream(const void*, size_t) -> RTNType;

  // Given a SPECK stream, tell how long the speck stream (including header) is.
  // Note: don't need to provide the buffer size because this function
  //       goes to a fixed location to retrieve the stream size.
  auto get_speck_stream_size(const void*) const -> uint64_t;

  void set_dimensions(dims_type);
  void set_data_range(double);

  // Set all compression parameters together. Notice their order!
  auto set_comp_params(size_t bit_budget, int32_t qlev, double psnr, double pwe) -> RTNType;

 protected:
  //
  // Member variables
  //
  const size_t m_header_size = 10;  // See header definition in SPECK_Storage.cpp.
  size_t m_encode_budget = 0;
  int32_t m_qz_lev = sperr::lowest_int32;
  double m_target_psnr = sperr::max_d;
  double m_target_pwe = 0.0;
  double m_data_range = sperr::max_d;  // range of data before DWT.

  // Use a cache variable to store the current compression mode.
  // The compression mode, however, is NOT set or determined by `m_mode_cache`. It is determined
  // by the combination of `m_encode_budget`, `m_qz_lev`, `m_target_psnr`, and `m_target_pwe`,
  // and can be retrieved by calling function sperr::compression_mode().
  CompMode m_mode_cache = CompMode::Unknown;

  //
  // A few data structures shared by both 2D and 3D SPECK algorithm
  //
  std::vector<double> m_coeff_buf;  // Wavelet coefficients
  std::vector<bool> m_bit_buffer;   // Bitstream produced by the algorithm
  std::vector<size_t> m_LIP;        // List of insignificant pixels
  std::vector<size_t> m_LSP_new;    // List of newly found significant pixels
  std::vector<bool> m_LSP_mask;     // Significant pixels previously found
  std::vector<bool> m_sign_array;   // Keep the signs of every coefficient
  vec8_type m_encoded_stream;

  size_t m_LSP_mask_sum = 0;     // Number of TRUE values in `m_LSP_mask`.
  size_t m_bit_idx = 0;          // Used for decode. Which bit we're at?
  double m_threshold = 0.0;      // Threshold that's used for an iteration.
  int32_t m_max_coeff_bit = 0;   // Maximum bitplane.
  dims_type m_dims = {0, 0, 0};  // Dimension of the 2D/3D volume
  const size_t m_u64_garbage_val = std::numeric_limits<size_t>::max();

  //
  // A few data members for error estimation
  //
  std::vector<double> m_orig_coeff;
  std::vector<double> m_qz_coeff;
  std::vector<double> m_calc_mse_buf; // temporary buffer facilitating MSE calculation
  size_t m_num_qz_coeff = 0;          // number of quantized (non-zero) coefficients

  //
  // Member methods
  //
  auto m_prepare_encoded_bitstream() -> RTNType;
  auto m_refinement_pass_encode() -> RTNType;
  auto m_refinement_pass_decode() -> RTNType;

  // Is there anything demanding the termination of iterations?
  auto m_termination_check(size_t bitplane) -> RTNType;
};

};  // namespace sperr

#endif
