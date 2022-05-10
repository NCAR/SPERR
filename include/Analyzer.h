#ifndef ANALYZER_H
#define ANALYZER_H

//
// This class analyzes wavelet coefficients so that implications of SPECK quantization
// level `q` can be easier to grasp.
//

#include "sperr_helper.h"

namespace sperr {

class Analyzer {
 public:
  // Calculate the mean square error (MSE) of an array given a quantization level q.
  void est_q_psnr(const vecd_type& coeffs);

 private:
  std::array<double, 2> m_range = {0.0, 0.0}; // range used for PSNR calculation
  std::array<double, 64> m_psnr_arr;
  std::vector<double> m_coeff_buf;  // buffer storing coefficients to help with quantization
  std::vector<double> m_quant_buf;  // buffer storing quantized values
  std::vector<bool>   m_sig_map;

  int32_t m_max_coeff_bit = 0;
  size_t m_current_q_idx = 0;
  


}; // class Analyzer

}; // namespace sperr

#endif
