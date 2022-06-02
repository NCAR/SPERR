//
// This is a the class object that is supposed to be used by most users, because
// it provides easy-to-use APIs.
// Functionality wise, it does not bring anything new though.
//

#ifndef SPERR3D_DECOMPRESSOR_H
#define SPERR3D_DECOMPRESSOR_H

#include "CDF97.h"
#include "Conditioner.h"
#include "SPECK3D.h"

#ifdef QZ_TERM
#include "SPERR.h"
#endif

#ifdef USE_ZSTD
#include "zstd.h"
#endif

namespace sperr {

class SPERR3D_Decompressor {
 public:
  // Accept incoming data.
  auto use_bitstream(const void* p, size_t len) -> RTNType;

  void set_dims(dims_type);

#ifndef QZ_TERM
  auto set_bpp(double) -> RTNType;
#endif

  auto decompress() -> RTNType;

  // Get the decompressed volume in a float or double buffer.
  template <typename T>
  auto get_data() const -> std::vector<T>;
  auto view_data() const -> const std::vector<double>&;
  auto release_data() -> std::vector<double>&&;

 private:
  dims_type m_dims = {0, 0, 0};
  Conditioner::meta_type m_condi_stream;
  vec8_type m_speck_stream;
  vecd_type m_val_buf;

  Conditioner m_conditioner;
  CDF97 m_cdf;
  SPECK3D m_decoder;

#ifdef QZ_TERM
  SPERR m_sperr;
  vec8_type m_sperr_stream;
#else
  double m_bpp = 0.0;
#endif

#ifdef USE_ZSTD
  std::unique_ptr<uint8_t[]> m_zstd_buf = nullptr;
  size_t m_zstd_buf_len = 0;
  std::unique_ptr<ZSTD_DCtx, decltype(&ZSTD_freeDCtx)> m_dctx = {nullptr, &ZSTD_freeDCtx};
#endif
};

};  // namespace sperr

#endif
