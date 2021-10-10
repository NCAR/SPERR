//
// This is a the class object that is supposed to be used by most users, because
// it provides easy-to-use APIs.
//

#ifndef SPECK2D_COMPRESSOR_H
#define SPECK2D_COMPRESSOR_H

#include "CDF97.h"
#include "Conditioner.h"
#include "SPECK2D.h"

using sperr::RTNType;

class SPECK2D_Compressor {
 public:
  // Accept incoming data: copy from a raw memory block
  template <typename T>
  auto copy_data(const T* p, size_t len, sperr::dims_type dims) -> RTNType;

  // Accept incoming data by taking ownership of the memory block
  auto take_data(std::vector<double>&& buf, sperr::dims_type dims) -> RTNType;

  void toggle_conditioning(sperr::Conditioner::settings_type);

  auto set_bpp(double) -> RTNType;

  auto compress() -> RTNType;

  auto view_encoded_bitstream() const -> const std::vector<uint8_t>&;
  auto release_encoded_bitstream() -> std::vector<uint8_t>&&;

 private:
  sperr::dims_type m_dims = {0, 0, 0};
  sperr::vecd_type m_val_buf;

  const size_t m_meta_size = 2;
  double m_bpp = 0.0;

  sperr::Conditioner m_conditioner;
  sperr::CDF97 m_cdf;
  sperr::SPECK2D m_encoder;

  sperr::Conditioner::settings_type m_conditioning_settings = {true, false, false, false};
  sperr::Conditioner::meta_type m_condi_stream;

  // Store bitstreams from the conditioner and SPECK encoding, and the overall bitstream.
  sperr::vec8_type m_speck_stream;
  sperr::vec8_type m_encoded_stream;

  auto m_assemble_encoded_bitstream() -> RTNType;
};

#endif
