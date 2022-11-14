#ifndef SPERR3D_H
#define SPERR3D_H

#include "CDF97.h"
#include "Conditioner.h"
#include "SPECK3D_INT_Driver.h"

namespace sperr {

class SPERR3D : public SPECK3D_INT_Driver {
 public:

  //
  // Input
  //
  virtual auto use_bitstream(const void* p, size_t len) -> RTNType; 

  //
  // Output
  //
  virtual auto release_encoded_bitstream() -> vec8_type&&;

  //
  // Configurations
  //
  void toggle_conditioning(Conditioner::settings_type);
  void set_target_psnr(double);

  //
  // Actions
  //
  virtual auto encode() -> RTNType;
  virtual auto decode() -> RTNType;

 private:
  Conditioner     m_conditioner;
  CDF97           m_cdf;
  SPECK3D_INT_ENC m_encoder;
  SPECK3D_INT_DEC m_decoder;

  Conditioner::settings_type m_conditioning_settings = {true, false, false, false};

  // Storage for various bitstreams
  Conditioner::meta_type m_condi_bitstream;
  vec8_type              m_encoded_bitstream;

  double m_target_psnr = sperr::max_d;
};

};  // namespace sperr

#endif
