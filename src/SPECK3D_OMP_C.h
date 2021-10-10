//
// This is a class that performs SPECK3D compression, and also utilizes OpenMP
// to achieve parallelization: the input volume is divided into smaller chunks
// and then they're processed individually.
//

#ifndef SPECK3D_OMP_C_H
#define SPECK3D_OMP_C_H

#include "SPECK3D_Compressor.h"

using sperr::RTNType;

class SPECK3D_OMP_C {
 public:
  void set_dims(sperr::dims_type);
  void prefer_chunk_dims(sperr::dims_type);
  void set_num_threads(size_t);

  // Upon receiving incoming data, a chunking scheme is decided, and the volume
  // is divided and kept in separate chunks.
  template <typename T>
  auto use_volume(const T*, size_t) -> RTNType;

  void toggle_conditioning(sperr::Conditioner::settings_type);

#ifdef QZ_TERM
  void set_qz_level(int32_t);
  auto set_tolerance(double) -> RTNType;
  // Return 1) the number of outliers, and 2) the num of bytes to encode them.
  auto get_outlier_stats() const -> std::pair<size_t, size_t>;
#else
  auto set_bpp(double) -> RTNType;
#endif

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

#ifdef QZ_TERM
  int32_t m_qz_lev = 0;
  double m_tol = 0.0;
  // Outlier stats include 1) the number of outliers, and 2) the num of bytes to
  // encode them.
  std::vector<std::pair<size_t, size_t>> m_outlier_stats;
#else
  double m_bpp = 0.0;
#endif

  //
  // Private methods
  //
  auto m_generate_header() const -> sperr::vec8_type;
};

#endif
