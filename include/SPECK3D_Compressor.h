//
// This is a the class object that is supposed to be used by most users, because
// it provides easy-to-use APIs.
//

#ifndef SPECK3D_COMPRESSOR_H
#define SPECK3D_COMPRESSOR_H

#include "CDF97.h"
#include "Conditioner.h"
#include "SPECK3D.h"

#ifdef QZ_TERM
#include "SPERR.h"
#endif

#ifdef USE_ZSTD
#include "zstd.h"
#endif

using sperr::RTNType;

class SPECK3D_Compressor {
 public:
  // Accept incoming data: copy from a raw memory block
  template <typename T>
  auto copy_data(const T* p,             // Input: pointer to the memory block
                 size_t len,             // Input: number of values
                 sperr::dims_type dims)  // Input: dimension of the 3D volume
      -> RTNType;

  // Accept incoming data: take ownership of a memory block
  auto take_data(std::vector<double>&& buf, sperr::dims_type dims) -> RTNType;

  void toggle_conditioning(sperr::Conditioner::settings_type);

#ifdef QZ_TERM
  void set_qz_level(int32_t);
  void set_tolerance(double);
  // Return 1) the number of outliers, and 2) the number of bytes to encode them.
  auto get_outlier_stats() const -> std::pair<size_t, size_t>;
#else
  auto set_bpp(double) -> RTNType;
#endif

  auto compress() -> RTNType;

  auto view_encoded_bitstream() const -> const std::vector<uint8_t>&;
  auto release_encoded_bitstream() -> std::vector<uint8_t>&&;

 private:
  sperr::dims_type m_dims = {0, 0, 0};
  sperr::vecd_type m_val_buf;

  sperr::Conditioner m_conditioner;
  sperr::CDF97 m_cdf;
  sperr::SPECK3D m_encoder;

  sperr::Conditioner::settings_type m_conditioning_settings = {true, false, false, false};

  // Store bitstreams from the conditioner and SPECK encoding, and the overall
  // bitstream.
  sperr::Conditioner::meta_type m_condi_stream;
  sperr::vec8_type m_speck_stream;
  sperr::vec8_type m_encoded_stream;

#ifdef QZ_TERM
  sperr::vec8_type m_sperr_stream;
  sperr::SPERR m_sperr;
  int32_t m_qz_lev = 0;
  double m_tol = 0.0;                 // tolerance used in error correction
  std::vector<sperr::Outlier> m_LOS;  // List of OutlierS
  sperr::vecd_type m_val_buf2;        // Copy of `m_val_buf` that goes through encoding.
  sperr::vecd_type m_diffv;           // Store differences to locate outliers.
#else
  double m_bpp = 0.0;
#endif

#ifdef USE_ZSTD
  sperr::vec8_type m_zstd_buf;
  std::unique_ptr<ZSTD_CCtx, decltype(&ZSTD_freeCCtx)> m_cctx = {nullptr, &ZSTD_freeCCtx};
#endif

  auto m_assemble_encoded_bitstream() -> sperr::RTNType;
};

#endif
