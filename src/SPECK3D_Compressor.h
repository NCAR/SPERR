//
// This is a the class object that is supposed to be used by most users, because
// it provides easy-to-use APIs.
//

#ifndef SPECK3D_COMPRESSOR_H
#define SPECK3D_COMPRESSOR_H

#include "CDF97.h"
#include "Conditioner.h"
#include "SPECK3D.h"
#include "SPERR.h"

#ifdef USE_ZSTD
#include "zstd.h"
#endif

using speck::RTNType;

class SPECK3D_Compressor {
 public:
  // Accept incoming data: copy from a raw memory block
  template <typename T>
  auto copy_data(const T* p, size_t len, speck::dims_type dims) -> RTNType;

  // Accept incoming data: take ownership of a memory block
  auto take_data(std::vector<double>&& buf, speck::dims_type dims) -> RTNType;

  void toggle_conditioning(std::array<bool, 8>);

#ifdef QZ_TERM
  void set_qz_level(int32_t);
  auto set_tolerance(double) -> RTNType;

  // Return 1) the number of outliers, and 2) the num of bytes to encode them.
  auto get_outlier_stats() const -> std::pair<size_t, size_t>;
#else
  auto set_bpp(float) -> RTNType;
#endif

  auto compress() -> RTNType;

  auto view_encoded_bitstream() const -> const std::vector<uint8_t>&;
  auto release_encoded_bitstream() -> std::vector<uint8_t>&&;

 private:
  speck::dims_type m_dims = {0, 0, 0};
  speck::vecd_type m_val_buf;

  speck::Conditioner m_conditioner;
  speck::CDF97 m_cdf;
  speck::SPECK3D m_encoder;

  std::array<bool, 8> m_conditioning_settings = {true,  false, false, false,
                                                 false, false, false, false};

  // Store bitstreams from the conditioner and SPECK encoding, and the overall
  // bitstream.
  speck::Conditioner::meta_type m_condi_stream;
  speck::vec8_type m_speck_stream;
  speck::vec8_type m_encoded_stream;

#ifdef QZ_TERM
  speck::vec8_type m_sperr_stream;
  speck::SPERR m_sperr;
  int32_t m_qz_lev = 0;
  double m_tol = 0.0;  // tolerance used in error correction
  size_t m_num_outlier = 0;
  std::vector<speck::Outlier> m_LOS;  // List of OutlierS
  speck::vecd_type
      m_val_buf2;            // Copy of `m_val_buf` that goes through encoding.
  speck::vecd_type m_diffv;  // Store differences in double in a vector.
#else
  float m_bpp = 0.0;
#endif

#ifdef USE_ZSTD
  std::unique_ptr<uint8_t[]> m_zstd_buf = nullptr;
  size_t m_zstd_buf_len = 0;
  std::unique_ptr<ZSTD_CCtx, decltype(&ZSTD_freeCCtx)> m_cctx = {
      nullptr, &ZSTD_freeCCtx};
#endif

  auto m_assemble_encoded_bitstream() -> speck::RTNType;
};

#endif
