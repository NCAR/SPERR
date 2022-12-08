#ifndef SPECK_INT_DRIVER_H
#define SPECK_INT_DRIVER_H

//
// This class is supposed to be the base class of 1D, 2D, and 3D drivers.
//

namespace sperr {

class SPECK_INT_Driver{
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
  auto release_encoded_bitstream() -> vec8_type&&;
  auto release_decoded_data() -> vecd_type&&;

  //
  // Generic configurations
  //
  void set_q(double q);
  void set_dims(dims_type);

  //
  // Queries
  //

  // Has to be called after an `encode` action.
  // Returns the number of values coded in the resulting bitstream.
  auto num_coded_vals() const -> size_t;

  //
  // Actions
  //
  virtual auto encode() -> RTNType = 0;
  virtual auto decode() -> RTNType = 0;

 protected:
  dims_type            m_dims = {0, 0, 0};
  double               m_q = 1.0;  // 1.0 is a better initial value than 0.0
  vecd_type            m_vals_d;
  std::vector<int64_t> m_vals_ll; // Signed integers produced by std::llrint()
  veci_t               m_vals_ui; // Unsigned integers to be passed to the encoder
  vecb_type            m_sign_array;  // Signs to be passed to the encoder
  vec8_type            m_speck_bitstream;

  SPECK3D_INT_ENC m_encoder;
  SPECK3D_INT_DEC m_decoder;

  virtual auto m_translate_f2i(const vecd_type&) -> RTNType; 
  virtual void m_translate_i2f(const veci_t&, const vecb_type&);


};

};  // namespace sperr

#endif
