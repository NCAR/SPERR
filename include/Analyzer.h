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
  // Estimate q-psnr curve until a target PSNR.
  // A valid data range must have already been set by set_range().
  void est_q_psnr(const vecd_type& coeffs, double target);

  // Calculate PSNR with all elements in `a` are used with their absolute values.
  auto calc_psnr(const vecd_type& a, const vecd_type& b) -> double;

  void set_range(double);
  void extract_range(const vecd_type&);
  

 private:
  std::array<double, 64> m_psnr_arr;
  std::array<int32_t, 64> m_q_arr;
  std::vector<double> m_coeff_buf;  // buffer storing coefficients to help with quantization
  std::vector<double> m_quant_buf;  // buffer storing quantized values
  std::vector<bool>   m_sig_map;

  std::vector<double> m_tmp_buf; // temporary use only

  size_t m_current_q_idx = 0;
  double m_range = 0.0; // data range used for PSNR calculation
  


}; // class Analyzer

}; // namespace sperr

#endif
