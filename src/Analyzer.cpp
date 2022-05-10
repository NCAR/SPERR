#include "Analyzer.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <numeric>  // std::accumulate()

void sperr::Analyzer::est_q_psnr(const vecd_type& coeffs)
{
  // Calculate an array of threshold values.
  const auto max_coeff = *std::max_element(coeffs.cbegin(), coeffs.cend(), 
                          [](auto a, auto b){ return std::abs(a) < std::abs(b); });
  m_max_coeff_bit = static_cast<int32_t>(std::floor(std::log2(max_coeff)));
  auto threshold_arr = std::array<double, 64>();
  threshold_arr[0] = std::pow(2.0, static_cast<double>(m_max_coeff_bit));
  std::generate(threshold_arr.begin(), threshold_arr.end(),
                [v = threshold_arr[0]]() mutable { return std::exchange(v, v * 0.5); });

  // Prepare buffers
  m_coeff_buf.resize(coeffs.size());
  m_quant_buf.resize(coeffs.size());
  m_sig_map.resize(coeffs.size());
  m_sig_map.assign(coeffs.size(), false);
  std::fill(m_quant_buf.begin(), m_quant_buf.end(), 0.0);
  std::fill(m_psnr_arr.begin(), m_psnr_arr.end(), 0.0);
  std::transform(coeffs.cbegin(), coeffs.cend(), m_coeff_buf.begin(),
                 [](auto v){ return std::abs(v); });

  // Perform quantizations
  auto recent_psnr = 0.0;
  //while (recent_psnr < 100.0) {
  while (m_current_q_idx < 3) {

    const auto thrd = threshold_arr[m_current_q_idx];
    const auto one_half = thrd * 1.5;
    const auto tmpd = std::array<double, 2>{0.0, -thrd};
    const auto tmpd_half = std::array<double, 2>{thrd * -0.5, thrd * 0.5};

    for (size_t i = 0; i < m_coeff_buf.size(); i++) {
      if (m_sig_map[i]) { // already identified as significant
        const size_t o1 = m_coeff_buf[i] >= thrd;
        m_coeff_buf[i] += tmpd[o1];
        m_quant_buf[i] += tmpd_half[o1];
      }
      else if (m_coeff_buf[i] >= thrd) {
        m_coeff_buf[i] -= thrd;
        m_quant_buf[i] = one_half;
        m_sig_map[i] = true;
      }

      if (i < 10)
        std::printf("Ana: m_quant_buf[%ld] = %f\n", i, m_quant_buf[i]);
    }
    std::printf("\n");

    recent_psnr = 100.0;
    m_psnr_arr[m_current_q_idx++] = recent_psnr;

  } // end of while()

}
