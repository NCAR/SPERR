#ifndef SPECK3D_INT_DRIVER_H
#define SPECK3D_INT_DRIVER_H

#include "SPECK3D_INT_ENC.h"
#include "SPECK3D_INT_DEC.h"

namespace sperr {

class SPECK3D_INT_Driver{
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
  virtual auto use_bitstream(const void* p, size_t len) -> RTNType; 

  //
  // Output
  //
  virtual auto release_encoded_bitstream() -> vec8_type&&;
  virtual auto release_decoded_data() -> vecd_type&&;

  //
  // Generic configurations
  //
  void set_q(double q);
  void set_dims(dims_type);

  //
  // Actions
  //
  virtual auto encode() -> RTNType;
  virtual auto decode() -> RTNType;

 protected:
  dims_type            m_dims = {0, 0, 0};
  double               m_q = 1.0;  // 1.0 is a better initial value than 0.0
  vecb_type            m_sign_array;
  vecd_type            m_vals_d;
  veci_t               m_vals_ui;
  vec8_type            m_speck_bitstream;
  std::vector<int64_t> m_vals_ll;

  SPECK3D_INT_ENC m_encoder;
  SPECK3D_INT_DEC m_decoder;

  auto m_translate_f2i(const vecd_type&) -> RTNType; // Results put in `m_vals_ui` and `m_sign_array`.
  void m_translate_i2f(const veci_t&, const vecb_type&); // Results put in `m_vals_d`.


};

};  // namespace sperr

#endif
