#include "Analyzer.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <numeric>  // std::accumulate()

void sperr::Analyzer::est_q_psnr(const vecd_type& coeffs, double target_psnr)
{
  if (m_range == 0.0)
    return;

  // Calculate an array of threshold values.
  const auto max_coeff = *std::max_element(coeffs.cbegin(), coeffs.cend(), 
                          [](auto a, auto b){ return std::abs(a) < std::abs(b); });
  m_q_arr[0] = static_cast<int32_t>(std::floor(std::log2(max_coeff)));
  std::generate(m_q_arr.begin(), m_q_arr.end(),
                [v = m_q_arr[0]]() mutable { return std::exchange(v, v - 1); });

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
  auto threshold = std::pow(2.0, static_cast<double>(m_q_arr[0]));
  m_current_q_idx = 0;

  while (recent_psnr < target_psnr) {

    const auto one_half = threshold * 1.5;
    const auto tmpd = std::array<double, 2>{0.0, -threshold};
    const auto tmpd_half = std::array<double, 2>{threshold * -0.5, threshold * 0.5};

    for (size_t i = 0; i < m_coeff_buf.size(); i++) {
      if (m_sig_map[i]) { // already identified as significant
        const size_t o1 = m_coeff_buf[i] >= threshold;
        m_coeff_buf[i] += tmpd[o1];
        m_quant_buf[i] += tmpd_half[o1];
      }
      else if (m_coeff_buf[i] >= threshold) {
        m_coeff_buf[i] -= threshold;
        m_quant_buf[i] = one_half;
        m_sig_map[i] = true;
      }
    }

    recent_psnr = calc_psnr(coeffs, m_quant_buf);
std::printf("%d, %f\n", m_q_arr[m_current_q_idx], recent_psnr);
    m_psnr_arr[m_current_q_idx] = recent_psnr;

    ++m_current_q_idx;
    threshold *= 0.5;

  } // end of while()

}

auto sperr::Analyzer::calc_psnr(const vecd_type& v1, const vecd_type& v2) -> double
{
  // Note that elements in v1 are used with their absolute values!

  assert(v1.size() == v2.size());
  if (v1.empty())
    return 0.0;
  if (m_range == 0.0)
    return std::numeric_limits<double>::infinity();

  const size_t stride_size = 4096;
  const size_t num_strides = v1.size() / stride_size;
  const size_t remainder_size = v1.size() - stride_size * num_strides;
  m_tmp_buf.resize(num_strides + 1);
  std::fill(m_tmp_buf.begin(), m_tmp_buf.end(), 0.0);

  for (size_t i = 0; i < num_strides; i++) {
    auto beg1 = v1.cbegin() + i * stride_size;
    auto end1 = beg1 + stride_size;
    auto beg2 = v2.cbegin() + i * stride_size;
    m_tmp_buf[i] = std::inner_product(beg1, end1, beg2, 0.0, std::plus<>(),
                  [](auto a, auto b){ return (std::abs(a) - b) * (std::abs(a) - b); });
  }
  m_tmp_buf[num_strides] = std::inner_product(v1.cbegin() + num_strides * stride_size, v1.cend(),
                            v2.cbegin() + num_strides * stride_size, 0.0, std::plus<>(),
                            [](auto a, auto b){ return (std::abs(a) - b) * (std::abs(a) - b); });

  const auto total_sum = std::accumulate(m_tmp_buf.cbegin(), m_tmp_buf.cend(), 0.0);
  const auto mse =  total_sum / static_cast<double>(v1.size());

  return std::log10(m_range * m_range / mse) * 10.0;
}

void sperr::Analyzer::set_range(double range)
{
  m_range = range;
}

void sperr::Analyzer::extract_range(const vecd_type& data)
{
  const auto [min, max] = std::minmax_element(data.cbegin(), data.cend());
  m_range = *max - *min;
}
