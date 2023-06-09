//
// This is a class that performs SPERR3D compression, and also utilizes OpenMP
// to achieve parallelization: the input volume is divided into smaller chunks
// and then they're processed individually.
//

#ifndef SPERR3D_OMP_C_H
#define SPERR3D_OMP_C_H

#include "SPECK3D_FLT.h"

namespace sperr {

class SPERR3D_OMP_C {
 public:
  // If 0 is passed in, the maximal number of threads will be used.
  void set_num_threads(size_t);

  // Upon receiving incoming data, a chunking scheme is decided, and the volume
  //    is divided and kept in separate chunks.
  template <typename T>
  auto copy_data(const T*, size_t len, dims_type vol_dims, dims_type chunk_dims) -> RTNType;

  void set_psnr(double);
  void set_tolerance(double);

  auto compress() -> RTNType;

  void append_encoded_bitstream(vec8_type& buf) const;

 private:
  dims_type m_dims = {0, 0, 0};        // Dimension of the entire volume
  dims_type m_chunk_dims = {0, 0, 0};  // Preferred dimensions for a chunk
  bool m_orig_is_float = true;         // The original input precision is saved in header.
  double m_quality = 0.0;
  CompMode m_comp_mode = CompMode::Unknown;

  std::vector<vecd_type> m_chunk_buffers;
  std::vector<vec8_type> m_encoded_streams;

#ifdef USE_OMP
  size_t m_num_threads = 1;
  std::vector<std::unique_ptr<SPECK3D_FLT>> m_compressors;
#else
  std::unique_ptr<SPECK3D_FLT> m_compressor;
#endif

  // Header size would be the magic number + num_chunks * 4
  const size_t m_header_magic_nchunks = 26;
  const size_t m_header_magic_1chunk = 14;

  //
  // Private methods
  //
  auto m_generate_header() const -> vec8_type;
};

}  // End of namespace sperr

#endif
