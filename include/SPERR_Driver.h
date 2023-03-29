#ifndef SPERR_DRIVER_H
#define SPERR_DRIVER_H

//
// This class is supposed to be the base class of 1D, 2D, and 3D SPERR.
//

#include "CDF97.h"
#include "Conditioner.h"
#include "SPECK_INT.h"

namespace sperr {

class SPERR_Driver {
 public:
  //
  // Virtual Destructor
  //
  virtual ~SPERR_Driver() = default;

  //
  // Input
  //
  // Accept incoming data: copy from a raw memory block
  template <typename T>
  void copy_data(const T* p,   // Input: pointer to the memory block
                 size_t len);  // Input: number of values

  // Accept incoming data: take ownership of a memory block
  void take_data(std::vector<double>&&);

  // Use an encoded bitstream
  virtual auto use_bitstream(const void* p, size_t len) -> RTNType;

  //
  // Output
  //
  auto get_encoded_bitstream() -> vec8_type;
  auto release_decoded_data() -> vecd_type&&;

  //
  // General configurations
  //
  void set_q(double q);
  void set_dims(dims_type);

  //
  // Actions
  //
  void toggle_conditioning(Conditioner::settings_type);
  virtual auto compress() -> RTNType;
  virtual auto decompress() -> RTNType;

 protected:
  dims_type m_dims = {0, 0, 0};
  double m_q = 1.0;  // 1.0 is a better initial value than 0.0
  vecd_type m_vals_d;
  vecui_t m_vals_ui;               // Unsigned integers to be passed to the encoder
  vecb_type m_sign_array;          // Signs to be passed to the encoder
  Conditioner::settings_type m_conditioning_settings = {true, false, false, false};

  Conditioner::meta_type m_condi_bitstream;
  vec8_type m_speck_bitstream;

  CDF97 m_cdf;
  Conditioner m_conditioner;
  std::unique_ptr<SPECK_INT> m_encoder = nullptr;
  std::unique_ptr<SPECK_INT> m_decoder = nullptr;

  auto m_midtread_f2i() -> RTNType;
  void m_midtread_i2f();

  // Both wavelet transforms operate on `m_vals_d`.
  virtual void m_wavelet_xform() = 0;
  virtual void m_inverse_wavelet_xform() = 0;

  // Quantization reads from `m_vals_d`, and writes to `m_vals_ui` and `m_sign_array`.
  // Inverse quantization reads from `m_vals_ui` and `m_sign_array`, and writes to `m_vals_d`.
  virtual auto m_quantize() -> RTNType = 0;
  virtual auto m_inverse_quantize() -> RTNType = 0;

  // Optional procedures for flexibility
  virtual auto m_proc_1() -> RTNType;
  virtual auto m_proc_2() -> RTNType;
};

};  // namespace sperr

#endif
