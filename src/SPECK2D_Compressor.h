//
// This is a the class object that is supposed to be used by most users, because
// it provides easy-to-use APIs.
// Functionality wise, it does not bring anything new though.
//

#ifndef SPECK2D_COMPRESSOR_H
#define SPECK2D_COMPRESSOR_H

#include "CDF97.h"
#include "Conditioner.h"
#include "SPECK2D.h"

using speck::RTNType;

class SPECK2D_Compressor {
 public:
  // Accept incoming data: copy from a raw memory block
  template <typename T>
  auto copy_data(const T* p, size_t len, speck::dims_type dims) -> RTNType;

  // Accept incoming data by taking ownership of the memory block
  auto take_data(std::vector<double>&& buf, speck::dims_type dims) -> RTNType;

  auto set_bpp(float) -> RTNType;

  auto compress() -> RTNType;

  auto view_encoded_bitstream() const -> const std::vector<uint8_t>&;
  auto release_encoded_bitstream() -> std::vector<uint8_t>&&;

 private:
  speck::dims_type m_dims = {0, 0, 0};
  speck::vecd_type m_val_buf;

  const size_t m_meta_size = 2;
  float m_bpp = 0.0;

  speck::Conditioner m_conditioner;
  speck::CDF97 m_cdf;
  speck::SPECK2D m_encoder;

  // Store bitstreams from the conditioner and SPECK encoding, and the overall
  // bitstream.
  speck::vec8_type m_condi_stream;
  speck::vec8_type m_speck_stream;
  speck::vec8_type m_encoded_stream;

  auto m_assemble_encoded_bitstream() -> RTNType;
};

#endif
