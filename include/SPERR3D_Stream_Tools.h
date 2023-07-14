#ifndef SPERR3D_STREAM_TOOLS_H
#define SPERR3D_STREAM_TOOLS_H

#include "sperr_helper.h"

namespace sperr {

//
// The header definition is in SPERR3D_OMP_D.cpp::m_generate_header().
//

class SPERR3D_Stream_Tools {
 public:
  // Info directly stored in the header
  uint8_t major_version = 0;
  bool is_portion = false;
  bool is_float = false;
  bool multi_chunk = false;
  dims_type vol_dims = {0, 0, 0}; 
  dims_type chunk_dims = {0, 0, 0};

  // Info calculated from above
  size_t header_len = 0;
  size_t stream_len = 0;
  std::vector<size_t> chunk_offsets;

  // Read the first 20 bytes of a bitstream, and determine the total length of the header.
  // Need 20 bytes because it's the larger of the header magic number.
  auto get_header_len(std::array<uint8_t, 20>) const -> size_t;

  // Read a bitstream that's at least as long as what's determined by `get_header_len()`, and
  // populate the information listed above.
  void populate_stream_info(const void*);

  // A special use case facilitating progressive access:
  //    reading in the header of a complete bitstream (produced by `get_encoded_bitstream()` here
  //    and provide a new header as well as a list of sections of the bitstream to read in so
  //    a portion of the complete bitstream can be used for decoding.
  // Note 1: This list of sections will be input to `sperr::read_sections()`.
  // Note 2: This function also produces a header that should be input to `sperr::read_sections()`.
  // Note 3: `stream` should be at least as long as what's determined by `get_header_len()`.
  //
  //auto progressive_sections(const void* stream, double bpp) const
  //    -> std::pair<vec8_type, std::vector<size_t>>;

 private:
  const size_t m_header_magic_nchunks = 20; 
  const size_t m_header_magic_1chunk = 14; 
};

} // End of namespace sperr

#endif
