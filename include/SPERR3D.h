#ifndef SPERR3D_INT_DRIVER_H
#define SPERR3D_INT_DRIVER_H

#include "CDF97.h"
#include "Conditioner.h"
#include "SPECK3D_INT_ENC.h"
#include "SPECK3D_INT_DEC.h"

namespace sperr {

class SPERR3D_INT_Driver{
 public:
  //
  // Input
  //
  // Accept incoming data: copy from a raw memory block
  template <typename T>
  void copy_data(const T* p,      // Input: pointer to the memory block
                 size_t len);     // Input: number of values

  // Accept incoming data: take ownership of a memory block
  void take_data(std::vector<double>&&);

  // Use an encoded bitstream
  auto use_bitstream(const void* p, size_t len) -> RTNType; 

  //
  // Output
  //
  auto release_encoded_bitstream() -> vec8_type&&;
  auto release_decoded_data() -> vecd_type&&;

  //
  // Generic configurations
  //
  void toggle_conditioning(Conditioner::settings_type);
  void set_pwe(double pwe);
  void set_dims(dims_type);

  //
  // Actions
  //
  auto compress() -> RTNType;
  auto decompress() -> RTNType;

 private:
  dims_type            m_dims = {0, 0, 0};
  vecb_type            m_sign_array;
  vecd_type            m_vals_d;
  veci_t               m_vals_ui;
  std::vector<int64_t> m_vals_ll;

  Conditioner     m_conditioner;
  CDF97           m_cdf;
  SPECK3D_INT_ENC m_encoder;
  SPECK3D_INT_DEC m_decoder;

  Conditioner::settings_type m_conditioning_settings = {true, false, false, false};

  // Storage for various bitstreams
  Conditioner::meta_type m_condi_stream;
  vec8_type              m_speck_bitstream;
  vec8_type              m_encoded_bitstream;

  double m_target_psnr = sperr::max_d;
  double m_target_pwe = 0.0;
};

};  // namespace sperr

#endif
