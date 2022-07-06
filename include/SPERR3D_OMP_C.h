//
// This is a class that performs SPERR3D compression, and also utilizes OpenMP
// to achieve parallelization: the input volume is divided into smaller chunks
// and then they're processed individually.
//

#ifndef SPERR3D_OMP_C_H
#define SPERR3D_OMP_C_H

#include "SPERR3D_Compressor.h"

using sperr::RTNType;

class SPERR3D_OMP_C {
 public:
  void set_num_threads(size_t);

  // Upon receiving incoming data, a chunking scheme is decided, and the volume
  // is divided and kept in separate chunks.
  template <typename T>
  auto copy_data(const T*, size_t len, sperr::dims_type vol_dims, sperr::dims_type chunk_dims)
      -> RTNType;

  void toggle_conditioning(sperr::Conditioner::settings_type);

  // Return 1) the number of outliers, and 2) the num of bytes to encode them.
  //auto get_outlier_stats() const -> std::pair<size_t, size_t>;

  auto set_target_bpp(double) -> RTNType;
  void set_target_qz_level(int32_t);
  void set_target_psnr(double);
  void set_target_pwe(double);

  auto compress() -> RTNType;

  // Provide a copy of the encoded bitstream to the caller.
  auto get_encoded_bitstream() const -> std::vector<uint8_t>;

 private:
  sperr::dims_type m_dims = {0, 0, 0};        // Dimension of the entire volume
  sperr::dims_type m_chunk_dims = {0, 0, 0};  // Preferred dimensions for a chunk
  size_t m_num_threads = 1;
  sperr::Conditioner::settings_type m_conditioning_settings = {true, false, false, false};

  std::vector<sperr::vecd_type> m_chunk_buffers;
  std::vector<sperr::vec8_type> m_encoded_streams;
  sperr::vec8_type m_total_stream;

  const size_t m_header_magic = 26;  // header size would be this number + num_chunks * 4

  size_t m_bit_budget = 0; // Total bit budget, including headers etc.
  int32_t m_qz_lev = sperr::lowest_int32;
  double m_target_psnr = sperr::max_d;
  double m_target_pwe = 0.0;

//  double m_tol = 0.0;
  // Outlier stats include 1) the number of outliers, and 2) the num of bytes used to encode them.
//  std::vector<std::pair<size_t, size_t>> m_outlier_stats;


  //
  // Private methods
  //
  auto m_generate_header() const -> sperr::vec8_type;
};

#endif
