#ifndef CONDITIONER_H
#define CONDITIONER_H

//
// Applies conditioning operations to an array of data.
// It also detects constant fields.
//

#include "sperr_helper.h"

namespace sperr {

using condi_type = std::array<uint8_t, 17>;

class Conditioner {
 public:
  auto condition(vecd_type& buf, dims_type) -> condi_type;
  auto inverse_condition(vecd_type& buf, dims_type, condi_type header) -> RTNType;

  auto is_constant(uint8_t) const -> bool;

  // Save a double to the last 8 bytes of a condi_type.
  void save_q(condi_type& header, double q) const;
  auto retrieve_q(condi_type header) const -> double;

 private:
  const size_t m_q_pos = 9;
  const size_t m_constant_field_idx = 7;
  const size_t m_default_num_strides = 2048;

  vecd_type m_stride_buf;

  // Buffers passed in here are guaranteed to have correct lengths and conditions.
  auto m_calc_mean(const vecd_type& buf) -> double;

  // Calculation is carried out by strides, which should be a divisor of the input data size.
  size_t m_num_strides = m_default_num_strides;

  // Adjust the value of `m_num_strides` so it'll be a divisor of `len`.
  void m_adjust_strides(size_t len);

  // Reset meta data field; see the cpp file for detail.
  void m_reset_meta(std::array<bool, 8>&) const;
};

};  // namespace sperr

#endif
