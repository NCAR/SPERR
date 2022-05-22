#ifndef CONDITIONER_H
#define CONDITIONER_H

//
// Applies conditioning operations to an array of data.
//

#include "sperr_helper.h"

#include <tuple>

namespace sperr {

class Conditioner {
 public:
  // Define a few data types
  using meta_type = std::array<uint8_t, 17>;
  using settings_type = std::array<bool, 4>;

  // Booleans recording what operations are applied:
  // bool[0] : subtract mean
  // bool[1] : divide by rms
  // bool[2-3] : unused
  void toggle_all_settings(settings_type);

  // The 17 bytes returned by `condition()`: 1 byte (8 booleans) followed by two
  // doubles. The byte of booleans records what operations are applied:
  // Accordingly, `inverse_condition()` takes a buffer of 17 bytes.
  auto condition(vecd_type&) -> std::pair<RTNType, meta_type>;
  auto inverse_condition(vecd_type&, const meta_type&) -> RTNType;

  // Given an array, test if it's a constant field. The test result (true/false)
  // is returned as boolean and a properly packed meta data if true.
  // Similarly, `parse_constant` takes in a meta data block and returns if it
  // represents a constant field. If true, it also returns the constant value.
  auto test_constant(const vecd_type&) const -> std::pair<bool, meta_type>;
  auto parse_constant(const meta_type&) const -> std::tuple<bool, double, uint64_t>;

 private:
  //
  // what settings are applied?
  //
  std::array<bool, 8> m_settings = {true,    // subtract mean
                                    false,   // divide by rms
                                    false,   // unused
                                    false,   // unused
                                    false,   // [4]: is this a constant field?
                                    false,   // unused
                                    false,   // unused
                                    false};  // unused

  // Calculations are carried out by strides, which
  // should be a divisor of the input data size.
  size_t m_num_strides = 2048;  // should be good enough for most applications.
  std::vector<double> m_stride_buf;

  // Action items
  // Buffers passed in here are guaranteed to have correct lengths and conditions.
  auto m_calc_mean(const vecd_type& buf) -> double;
  auto m_calc_rms(const vecd_type& buf) -> double;
  // Adjust the value of `m_num_strides` so it'll be a divisor of `len`.
  void m_adjust_strides(size_t len);
};

};  // namespace sperr

#endif
