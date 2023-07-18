#ifndef CONDITIONER_H
#define CONDITIONER_H

//
// Applies conditioning operations to an array of data.
// It also detects constant fields.
//

#include "sperr_helper.h"

namespace sperr {

class Conditioner {
 public:
  auto condition(vecd_type& buf, dims_type) -> vec8_type;
  auto inverse_condition(vecd_type& buf, dims_type, const vec8_type& header) -> RTNType;

  auto is_constant(uint8_t) const -> bool;
  auto header_size(const void*) const -> size_t;

 private:
  using meta_type = std::array<bool, 8>;
  const uint8_t m_constant_field_idx = 7;
  const uint8_t m_min_header_size = 9;              // when there's only a mean value saved.
  const uint8_t m_constant_field_header_size = 17;  // when recording a constant field.

  const size_t m_default_num_strides = 2048;

  vecd_type m_stride_buf;

  // Buffers passed in here are guaranteed to have correct lengths and conditions.
  auto m_calc_mean(const vecd_type& buf) -> double;

  // Calculation is carried out by strides, which should be a divisor of the input data size.
  size_t m_num_strides = m_default_num_strides;

  // Adjust the value of `m_num_strides` so it'll be a divisor of `len`.
  void m_adjust_strides(size_t len);

  // Reset meta data field; see the cpp file for detail.
  void m_reset_meta(meta_type&) const;
};

};  // namespace sperr

#endif
