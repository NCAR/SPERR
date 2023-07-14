//
// This is a class that performs SPERR3D decompression, and also utilizes OpenMP
// to achieve parallelization: input to this class is supposed to be smaller
// chunks of a bigger volume, and each chunk is decompressed individually before
// returning back the big volume.
//

#ifndef SPERR3D_OMP_D_H
#define SPERR3D_OMP_D_H

#include "SPECK3D_FLT.h"

namespace sperr {

class SPERR3D_OMP_D {
 public:
  // If 0 is passed in here, the maximum number of threads will be used.
  void set_num_threads(size_t);

  // Parse the header of this stream, and stores the pointer.
  auto setup_decomp(const void*, size_t) -> RTNType;

  // The pointer passed in here MUST be the same as the one passed to `use_bitstream()`.
  auto decompress(const void*) -> RTNType;

  auto release_decoded_data() -> sperr::vecd_type&&;
  auto view_decoded_data() const -> const sperr::vecd_type&;

  auto get_dims() const -> sperr::dims_type;

  // Read the first 20 bytes of a bitstream, and determine the total length of the header.
  //    Note: Need 20 bytes because it's the larger of the header magic number (`read_sections`).
  //
  auto get_header_len(std::array<uint8_t, 20>) const -> size_t;

  // A special use case facilitating progressive access:
  //    reading in the header of a complete bitstream (produced by `get_encoded_bitstream()` here
  //    and provide a new header as well as a list of sections of the bitstream to read in so
  //    a portion of the complete bitstream can be used for decoding.
  // Note 1: This list of sections will be input to `sperr::read_sections()`.
  // Note 2: This function also produces a header that should be input to `sperr::read_sections()`.
  // Note 3: The length of the stream should be at least what's determined by `get_header_len()`.
  //
  auto progressive_sections(const void* stream, double bpp) const
      -> std::pair<vec8_type, std::vector<size_t>>;

 private:
  sperr::dims_type m_dims = {0, 0, 0};        // Dimension of the entire volume
  sperr::dims_type m_chunk_dims = {0, 0, 0};  // Preferred dimensions for a chunk

#ifdef USE_OMP
  size_t m_num_threads = 1;

  // It turns out that the object of `SPECK3D_FLT` is not copy-constructible, so it's
  //    a little difficult to work with a container (std::vector<>), so we ask the
  //    container to store pointers (which are trivially constructible) instead.
  //
  std::vector<std::unique_ptr<SPECK3D_FLT>> m_decompressors;
#else
  // This single instance of decompressor doesn't need to be allocated on the heap;
  //    rather, it's just to keep consistency with the USE_OMP case.
  //
  std::unique_ptr<SPECK3D_FLT> m_decompressor;
#endif

  sperr::vecd_type m_vol_buf;
  std::vector<size_t> m_offsets;  // Address offset to locate each bitstream chunk.
  const uint8_t* m_bitstream_ptr = nullptr;

  // Header size would be the magic number + num_chunks * 4
  const size_t m_header_magic_nchunks = 20;
  const size_t m_header_magic_1chunk = 14;

  // Put this chunk to a bigger volume
  // Memory errors will occur if the big and small volumes are not the same size as described.
  void m_scatter_chunk(vecd_type& big_vol,
                       dims_type vol_dim,
                       const vecd_type& small_vol,
                       std::array<size_t, 6> chunk_info);
};

}  // End of namespace sperr

#endif
