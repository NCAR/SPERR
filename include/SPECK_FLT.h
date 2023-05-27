#ifndef SPECK_FLT_H
#define SPECK_FLT_H

//
// This class serves as the base class of 1D, 2D, and 3D SPECK algorithm on floats.
//

#include "CDF97.h"
#include "Conditioner.h"
#include "SPECK_INT.h"

#include <variant>

namespace sperr {

class SPECK_FLT {
 public:
  //
  // Virtual Destructor
  //
  virtual ~SPECK_FLT() = default;

  //
  // Input
  //
  // Accept incoming data: copy from a raw memory block.
  // Note: `len` is the number of values.
  template <typename T>
  void copy_data(const T* p, size_t len);

  // Accept incoming data: take ownership of a memory block
  void take_data(std::vector<double>&&);

  // Use an encoded bitstream
  // Note: `len` is the number of bytes.
  virtual auto use_bitstream(const void* p, size_t len) -> RTNType;

  //
  // Output
  //
  void append_encoded_bitstream(vec8_type& buf) const;
  auto release_decoded_data() -> vecd_type&&;

  //
  // General configuration and info.
  //
  void set_q(double q);
  void set_dims(dims_type);
  auto integer_len() const -> size_t;

  //
  // Actions
  //
  virtual auto compress() -> RTNType;
  virtual auto decompress() -> RTNType;

 protected:
  double m_q = 1.0;  // 1.0 is a better initial value than 0.0
  UINTType m_uint_flag = UINTType::UINT64;  // Default to use 64-bit integers.
  dims_type m_dims = {0, 0, 0};
  vecd_type m_vals_d;
  vecb_type m_sign_array;  // Signs to be passed to the integer encoder
  vec8_type m_condi_bitstream;

  Conditioner m_conditioner;
  CDF97 m_cdf;

  std::variant<std::unique_ptr<SPECK_INT<uint64_t>>,
               std::unique_ptr<SPECK_INT<uint32_t>>,
               std::unique_ptr<SPECK_INT<uint16_t>>,
               std::unique_ptr<SPECK_INT<uint8_t>>>
      m_encoder, m_decoder;

  std::variant<std::vector<uint64_t>,
               std::vector<uint32_t>,
               std::vector<uint16_t>,
               std::vector<uint8_t>>
      m_vals_ui;

  // Instantiate `m_vals_ui` based on the chosen integer length.
  void m_instantiate_int_vec();

  // Derived classes instantiate the correct `m_encoder` and `m_decoder` depending on
  //   3D/2D/1D classes, and on the integer length in use.
  virtual void m_instantiate_encoder() = 0;
  virtual void m_instantiate_decoder() = 0;

  // Both wavelet transforms operate on `m_vals_d`.
  virtual void m_wavelet_xform() = 0;
  virtual void m_inverse_wavelet_xform() = 0;

  // Quantization reads from `m_vals_d`, and writes to `m_vals_ui` and `m_sign_array`.
  // Inverse quantization reads from `m_vals_ui` and `m_sign_array`, and writes to `m_vals_d`.
  virtual auto m_quantize() -> RTNType = 0;
  virtual auto m_inverse_quantize() -> RTNType = 0;

  // This base class provides two midtread quantization implementations, but derived classes
  //   can have other quantization methods.
  auto m_midtread_f2i() -> RTNType;
  void m_midtread_i2f();
};

};  // namespace sperr

#endif