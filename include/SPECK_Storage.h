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
  const size_t m_header_size = 12;  // See header definition in SPECK_Storage.cpp.
  const size_t m_u64_garbage_val = std::numeric_limits<size_t>::max();
  size_t m_encode_budget = 0;
  size_t m_LSP_mask_sum = 0;    // Number of TRUE values in `m_LSP_mask`
  size_t m_bit_idx = 0;         // Used for decode. Which bit we're at?
  float m_max_threshold_f = 0.0; // float representation of max threshold
  size_t m_num_bitplanes = 0;    // In PWE mode, the number of bitplanes to use in quantization
  int32_t m_qz_lev = sperr::lowest_int32;
  double m_data_range = sperr::max_d;  // range of data before DWT
  double m_target_psnr = sperr::max_d;
  double m_target_pwe = 0.0;  // used in fixed-PWE mode
  double m_threshold = 0.0;   // Threshold that's used for an iteration

  // Use a cache variable to store the current compression mode.
  // The compression mode, however, is NOT set or determined by `m_mode_cache`. It is determined
  // by the combination of `m_encode_budget`, `m_qz_lev`, `m_target_psnr`, and `m_target_pwe`,
  // and can be retrieved by calling function sperr::compression_mode().
  CompMode m_mode_cache = CompMode::Unknown;

  // Data storage shared by both 2D and 3D SPECK.
  //
  std::vector<double> m_coeff_buf;  // Wavelet coefficients
  std::vector<bool> m_bit_buffer;   // Bitstream produced by the algorithm
  std::vector<size_t> m_LIP;        // List of insignificant pixels
  std::vector<size_t> m_LSP_new;    // List of newly found significant pixels
  std::vector<bool> m_LSP_mask;     // Significant pixels previously found
  std::vector<bool> m_sign_array;   // Keep the signs of every coefficient
  vec8_type m_encoded_stream;       // Stores the SPECK bitstream
  dims_type m_dims = {0, 0, 0};     // Dimension of the 2D/3D volume

  // In fixed-pwe mode, keep track of would-be quantized coefficient values.
  //
  std::vector<double> m_qz_coeff;

  //
  // Member methods
  //
  auto m_prepare_encoded_bitstream() -> RTNType;
  auto m_refinement_pass_encode() -> RTNType;
  auto m_refinement_pass_decode() -> RTNType;

  auto m_termination_check(size_t bitplane_idx) const -> RTNType;
  auto m_estimate_mse() const -> double;
};

};  // namespace sperr

#endif
