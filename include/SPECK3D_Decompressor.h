//
// This is a the class object that is supposed to be used by most users, because
// it provides easy-to-use APIs.
// Functionality wise, it does not bring anything new though.
//

#ifndef SPECK3D_DECOMPRESSOR_H
#define SPECK3D_DECOMPRESSOR_H

#include "CDF97.h"
#include "Conditioner.h"
#include "SPECK3D.h"
#include "SPERR.h"

#ifdef USE_ZSTD
#include "zstd.h"
#endif

using sperr::RTNType;

class SPECK3D_Decompressor {
 public:
  // Accept incoming data.
  auto use_bitstream(const void* p, size_t len) -> RTNType;

#ifndef QZ_TERM
  auto set_bpp(double) -> RTNType;
#endif

  auto decompress() -> RTNType;

  // Get the decompressed volume in a float or double buffer.
  template <typename T>
  auto get_data() const -> std::vector<T>;
  auto view_data() const -> const std::vector<double>&;
  auto release_data() -> std::vector<double>&&;
  auto get_dims() const -> std::array<size_t, 3>;

 private:
  sperr::dims_type m_dims = {0, 0, 0};

  sperr::Conditioner::meta_type m_condi_stream;
  sperr::vec8_type m_speck_stream;
  sperr::vecd_type m_val_buf;

  sperr::Conditioner m_conditioner;
  sperr::CDF97 m_cdf;
  sperr::SPECK3D m_decoder;

#ifdef QZ_TERM
  sperr::SPERR m_sperr;
  sperr::vec8_type m_sperr_stream;
#else
  double m_bpp = 0.0;
#endif

#ifdef USE_ZSTD
  std::unique_ptr<uint8_t[]> m_zstd_buf = nullptr;
  size_t m_zstd_buf_len = 0;
  std::unique_ptr<ZSTD_DCtx, decltype(&ZSTD_freeDCtx)> m_dctx = {nullptr, &ZSTD_freeDCtx};
#endif
};

#endif
