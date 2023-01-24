#ifndef CONDITIONER_H
#define CONDITIONER_H

//
// Applies conditioning operations to an array of data.
// Any data is subject to an "subtract mean" operation, but can also undergo
// operations provided by a custom filter.
// Finally, the Conditioner also detects constant fields.
//

#include "sperr_helper.h"
#include "Base_Filter.h"
#include "Matthias_Filter.h"

namespace sperr {

class Conditioner {
 public:

  auto condition(vecd_type& buf, dims_type) -> vec8_type;
  auto inverse_condition(vecd_type& buf, dims_type, const vec8_type& header) -> RTNType;

  auto is_constant(uint8_t) const -> bool;
  auto header_size(const void*) const -> size_t;

 private:
  //
  // what operations are applied?
  //
  std::array<bool, 8> m_meta = {true,    // subtract mean
                                false,   // custom filter used?
                                false,   // unused
                                false,   // unused
                                false,   // unused
                                false,   // unused
                                false,   // unused
                                false};  // [7]: is this a constant field?

  const size_t m_constant_field_header_size = 17;
  const size_t m_default_num_strides = 2048;
  const size_t m_constant_field_idx = 7;
  const size_t m_custom_filter_idx = 1;
  const size_t m_min_header_size = 5; // when there's only a mean value saved.

  Base_Filter m_filter;
  //Matthias_Filter m_filter;

  // Buffers passed in here are guaranteed to have correct lengths and conditions.
  auto m_calc_mean(const vecd_type& buf) -> double;

  // Calculations are carried out by strides, which
  // should be a divisor of the input data size.
  size_t m_num_strides = m_default_num_strides;
  std::vector<double> m_stride_buf;
  // Adjust the value of `m_num_strides` so it'll be a divisor of `len`.
  void m_adjust_strides(size_t len);
  // Reset meta fields
  void m_reset_meta();
};

};  // namespace sperr

#endif
