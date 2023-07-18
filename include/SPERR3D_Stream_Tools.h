#ifndef SPERR3D_STREAM_TOOLS_H
#define SPERR3D_STREAM_TOOLS_H

#include "sperr_helper.h"

namespace sperr {

//
// The 3D SPERR header definition is in SPERR3D_OMP_C.cpp::m_generate_header().
//

class SPERR3D_Stream_Tools {
 public:
  // Info directly stored in the header
  uint8_t major_version = 0;
  bool is_portion = false;
  bool is_3D = false;
  bool is_float = false;
  bool multi_chunk = false;
  dims_type vol_dims = {0, 0, 0}; 
  dims_type chunk_dims = {0, 0, 0};

  // Info calculated from above
  size_t header_len = 0;
  size_t stream_len = 0;
  std::vector<size_t> chunk_offsets;

  // Read the first 20 bytes of a bitstream, and determine the total length of the header.
  // Need 20 bytes because it's the larger of the header magic number (in multi-chunk case).
  auto get_header_len(std::array<uint8_t, 20>) const -> size_t;

  // Read a bitstream that's at least as long as what's determined by `get_header_len()`, and
  // populate the information listed above.
  void populate_stream_info(const void*);

  // Function that reads in portions of a bitstream to facilitate progressive access.
  //
  auto progressive_read(std::string filename, double bpp) -> vec8_type;

 private:
  const size_t m_header_magic_nchunks = 20; 
  const size_t m_header_magic_1chunk = 14; 
};

} // End of namespace sperr

#endif
