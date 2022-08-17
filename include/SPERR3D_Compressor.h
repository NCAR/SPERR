#ifndef SPERR3D_COMPRESSOR_H
#define SPERR3D_COMPRESSOR_H

#include "CDF97.h"
#include "Conditioner.h"
#include "SPECK3D.h"
#include "SPERR.h"

#ifdef USE_ZSTD
#include "zstd.h"
#endif

namespace sperr {

class SPERR3D_Compressor {
 public:
  // Accept incoming data: copy from a raw memory block
  template <typename T>
  auto copy_data(const T* p,      // Input: pointer to the memory block
                 size_t len,      // Input: number of values
                 dims_type dims)  // Input: dimension of the 3D volume
      -> RTNType;

  // Accept incoming data: take ownership of a memory block
  auto take_data(std::vector<double>&& buf, dims_type dims) -> RTNType;

  void toggle_conditioning(Conditioner::settings_type);

  auto set_comp_params(size_t bit_budget, double psnr, double pwe) -> RTNType;

  // Return 1) the number of outliers, and 2) the number of bytes to encode them.
  auto get_outlier_stats() const -> std::pair<size_t, size_t>;

  auto compress() -> RTNType;

  auto view_encoded_bitstream() const -> const std::vector<uint8_t>&;
  auto release_encoded_bitstream() -> std::vector<uint8_t>&&;

 private:
  dims_type m_dims = {0, 0, 0};
  vecd_type m_val_buf;

  Conditioner m_conditioner;
  CDF97 m_cdf;
  SPECK3D m_encoder;

  Conditioner::settings_type m_conditioning_settings = {true, false, false, false};

  // Store bitstreams from the conditioner and SPECK encoding, and the overall bitstream.
  Conditioner::meta_type m_condi_stream;
  vec8_type m_encoded_stream;

  size_t m_bit_budget = 0;  // Total bit budget, including headers etc.
  double m_target_psnr = sperr::max_d;
  double m_target_pwe = 0.0;

  SPERR m_sperr;
  vec8_type m_sperr_stream;
  vecd_type m_val_buf2;        // Copy of `m_val_buf` that's used for outlier coding.
  std::vector<Outlier> m_LOS;  // List of OutlierS

#ifdef USE_ZSTD
  vec8_type m_zstd_buf;
  std::unique_ptr<ZSTD_CCtx, decltype(&ZSTD_freeCCtx)> m_cctx = {nullptr, &ZSTD_freeCCtx};
#endif

  auto m_assemble_encoded_bitstream() -> RTNType;
};

};  // namespace sperr

#endif
