#ifndef SPECK_STORAGE_H
#define SPECK_STORAGE_H

//
// This class implements mechanisms and interfaces to help 2D and 3D SPECK
// classes manage storage of coefficients and bitstreams.
//

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
  auto release_data() -> vecd_type&&;          // Release ownership
  auto view_data() const -> const vecd_type&;  // Keep ownership

  // Get the encoded bitstream.
  auto view_encoded_bitstream() const -> const vec8_type&;
  auto release_encoded_bitstream() -> vec8_type&&;

  // Prepare internal states for a decompression operation from an encoded bitstream
  // Note: it takes a raw pointer because it accesses memory provided by others,
  //       and others most likely provide a raw pointer.
  auto parse_encoded_bitstream(const void*, size_t) -> RTNType;

  // Given a SPECK stream, tell how long the speck stream (including header) is.
  // Note: don't need to provide the buffer size because this function
  //       goes to a fixed location to retrieve the stream size.
  auto get_speck_stream_size(const void*) const -> uint64_t;

  void set_dimensions(dims_type);

 protected:
  //
  // Member variables
  //
#ifdef QZ_TERM
  const size_t m_header_size = 12;  // See header definition in SPECK_Storage.cpp.
  int32_t m_qz_lev = 0;             // At which quantization level does encoding terminate?
                                    // Necessary in preparing bitstream headers.
#else
  const size_t m_header_size = 10;  // See header definition in SPECK_Storage.cpp.
#endif

  int32_t m_max_coeff_bit = 0;  // Necessary in preparing bitstream headers.
  dims_type m_dims = {0, 0, 0};

  std::vector<double> m_coeff_buf;
  std::vector<bool> m_bit_buffer;
  vec8_type m_encoded_stream;

  auto m_prepare_encoded_bitstream() -> RTNType;
};

};  // namespace sperr

#endif
