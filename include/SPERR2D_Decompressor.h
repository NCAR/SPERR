//
// This is a the class object that is supposed to be used by most users, because
// it provides easy-to-use APIs.
// Functionality wise, it does not bring anything new though.
//

#ifndef SPERR2D_DECOMPRESSOR_H
#define SPERR2D_DECOMPRESSOR_H

#include "CDF97.h"
#include "Conditioner.h"
#include "SPECK2D.h"

#ifdef QZ_TERM
#include "SPERR.h"
#endif

using sperr::RTNType;

class SPERR2D_Decompressor {
 public:
  // Accept incoming data.
  auto use_bitstream(const void* p, size_t len) -> RTNType;

#ifndef QZ_TERM
  auto set_bpp(double) -> RTNType;
#endif

  auto decompress() -> RTNType;

  // Get the decompressed data in a float or double buffer.
  template <typename T>
  auto get_data() const -> std::vector<T>;
  auto release_data() -> std::vector<double>&&;
  auto view_data() const -> const std::vector<double>&;
  auto get_dims() const -> sperr::dims_type;

 private:
  sperr::dims_type m_dims = {0, 0, 0};
  sperr::Conditioner::meta_type m_condi_stream;
  sperr::vec8_type m_speck_stream;
  sperr::vecd_type m_val_buf;
  const size_t m_meta_size = 10;  // Need to be the same as in SPERR2D_Compressor.h

#ifdef QZ_TERM
  sperr::SPERR m_sperr;
  sperr::vec8_type m_sperr_stream;
#else
  double m_bpp = 0.0;
#endif

  sperr::Conditioner m_conditioner;
  sperr::CDF97 m_cdf;
  sperr::SPECK2D m_decoder;
};

#endif
