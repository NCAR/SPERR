#ifndef ANALYZER_H
#define ANALYZER_H

//
// This class analyzes wavelet coefficients so that implications of SPECK quantization
// level `q` can be easier to grasp.
//

namespace sperr {

class Analyzer {
 public:
  // Calculate the mean square error (MSE) of an array given a quantization level q.
  auto calc_mse(const vecd_type& vec, int32_t q) const -> double;

 private:
  std::array<double, 2> m_range = {0.0, 0.0}; // range used for PSNR calculation
  int32_t m_max_coeff_bit = 0;

}; // class Analyzer

}; // namespace sperr

#endif
