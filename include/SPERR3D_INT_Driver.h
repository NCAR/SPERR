#ifndef SPERR3D_INT_DRIVER_H
#define SPERR3D_INT_DRIVER_H

#include "CDF97.h"
#include "Conditioner.h"
#include "SPECK3D_INT_ENC.h"
#include "SPECK3D_INT_DEC.h"

#ifdef USE_ZSTD
#include "zstd.h"
#endif

namespace sperr {

class SPERR3D_INT_Driver{
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

  void set_pwe(double pwe);

  auto compress() -> RTNType;

  auto release_speck_bitstream() -> std::vector<uint8_t>&&;

 private:
  dims_type m_dims = {0, 0, 0};
  vecb_type m_sign_array;
  vecd_type m_vals_d;
  veci_t m_vals_i;
  std::vector<int64_t> m_vals_ll;

  Conditioner m_conditioner;
  CDF97 m_cdf;
  SPECK3D_INT_ENC m_encoder;
  SPECK3D_INT_DEC m_decoder;

  Conditioner::settings_type m_conditioning_settings = {true, false, false, false};

  // Store bitstreams from the conditioner and SPECK encoding, and the overall bitstream.
  Conditioner::meta_type m_condi_stream;
  vec8_type m_speck_stream, m_encoded_stream;

  double m_target_psnr = sperr::max_d;
  double m_target_pwe = 0.0;

#ifdef USE_ZSTD
  vec8_type m_zstd_buf;
  std::unique_ptr<ZSTD_CCtx, decltype(&ZSTD_freeCCtx)> m_cctx = {nullptr, &ZSTD_freeCCtx};
#endif

  auto m_assemble_encoded_bitstream() -> RTNType;
};

};  // namespace sperr

#endif
