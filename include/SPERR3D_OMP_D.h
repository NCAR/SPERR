//
// This is a class that performs SPERR3D decompression, and also utilizes OpenMP
// to achieve parallelization: input to this class is supposed to be smaller
// chunks of a bigger volume, and each chunk is decompressed individually before
// returning back the big volume.
//

#ifndef SPERR3D_OMP_D_H
#define SPERR3D_OMP_D_H

#include "SPERR3D_Decompressor.h"

using sperr::RTNType;

class SPERR3D_OMP_D {
 public:
  // Parse the header of this stream, and stores the pointer.
  auto use_bitstream(const void*, size_t) -> RTNType;

  void set_num_threads(size_t);

  // The pointer passed in here MUST be the same as the one passed to `use_bitstream()`.
  auto decompress(const void*) -> RTNType;

  auto release_data() -> std::vector<double>&&;
  auto view_data() const -> const std::vector<double>&;
  template <typename T>
  auto get_data() const -> std::vector<T>;
  auto get_dims() const -> sperr::dims_type;

 private:
  sperr::dims_type m_dims = {0, 0, 0};        // Dimension of the entire volume
  sperr::dims_type m_chunk_dims = {0, 0, 0};  // Preferred dimensions for a chunk
  size_t m_num_threads = 1;                   // number of theads to use in OpenMP sections

  const size_t m_header_magic = 26;  // header size would be this number + num_chunks * 4

  std::vector<double> m_vol_buf;
  std::vector<size_t> m_offsets;
  const uint8_t* m_bitstream_ptr = nullptr;
};

#endif
