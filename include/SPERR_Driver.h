#ifndef SPERR_DRIVER_H
#define SPERR_DRIVER_H

//
// This class is supposed to be the base class of 1D, 2D, and 3D SPERR.
//

#include "CDF97.h"
#include "Conditioner.h"
#include "SPECK_INT.h"

#include <variant>

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
  // Accept incoming data: copy from a raw memory block.
  // `len` is the number of values.
  template <typename T>
  void copy_data(const T* p, size_t len);

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
  // Default to use 64-bit integers, but can also use smaller sizes.
  //
  UINTType m_uint_flag = UINTType::UINT64;
  std::variant<std::vector<uint64_t>,
               std::vector<uint32_t>,
               std::vector<uint16_t>,
               std::vector<uint8_t>>
      m_vals_ui;
  std::variant<std::unique_ptr<SPECK_INT<uint64_t>>,
               std::unique_ptr<SPECK_INT<uint32_t>>,
               std::unique_ptr<SPECK_INT<uint16_t>>,
               std::unique_ptr<SPECK_INT<uint8_t>>>
      m_encoder, m_decoder;

  dims_type m_dims = {0, 0, 0};
  double m_q = 1.0;  // 1.0 is a better initial value than 0.0
  vecd_type m_vals_d;
  vecb_type m_sign_array;  // Signs to be passed to the encoder
  Conditioner::settings_type m_conditioning_settings = {true, false, false, false};

  Conditioner::meta_type m_condi_bitstream;
  vec8_type m_speck_bitstream;

  CDF97 m_cdf;
  Conditioner m_conditioner;

  // Derived classes instantiate the correct `m_encoder` and `m_decoder` depending on
  //  3D/2D/1D classes, and the integer length in use.
  virtual void m_instantiate_encoder() = 0;
  virtual void m_instantiate_decoder() = 0;

  // Instantiate `m_vals_ui` based on the chosen integer length.
  virtual void m_instantiate_int_vec();

  // Both wavelet transforms operate on `m_vals_d`.
  virtual void m_wavelet_xform() = 0;
  virtual void m_inverse_wavelet_xform() = 0;

  // Quantization reads from `m_vals_d`, and writes to `m_vals_ui` and `m_sign_array`.
  // Inverse quantization reads from `m_vals_ui` and `m_sign_array`, and writes to `m_vals_d`.
  virtual auto m_quantize() -> RTNType = 0;
  virtual auto m_inverse_quantize() -> RTNType = 0;

  // This base class provides two midtread quantization implementations, but derived classes
  //  can have other quantization methods.
  auto m_midtread_f2i() -> RTNType;
  void m_midtread_i2f();
};

};  // namespace sperr

#endif