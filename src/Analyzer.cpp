#include "Analyzer.h"

#include <algorithm>
#include <cassert>
#include <numeric>  // std::accumulate()

auto sperr::Analyzer::calc_mse(const vecd_type& vec, int32_t q) const -> double
{
  const auto max_coeff = *std::max_element(vec.cbegin(), vec.cend(), 
                          [](auto a, auto b){ return std::abs(a) < std::abs(b); });
  int32_t max_coeff_bit = static_cast<int32_t>(std::floor(std::log2(max_coeff)));
  auto threshold_arr = std::array<double, 64>();
  threshold_arr[0] = std::pow(2.0, static_cast<double>(max_coeff_bit));
  std::generate(threshold_arr.begin(), threshold_arr.end(),
                [v = threshold_arr[0]]() mutable { return std::exchange(v, v * 0.5); });

  const size_t stride_size = 4096;
  const size_t num_strides = vec.size() / stride_size;
  const size_t remainder_size = vec.size() - stride_size * num_strides;
  auto sum_vec = std::vector<double>(num_strides + 1, 0.0);

  for (size_t stride = 0; stride < num_strides; i++) {

  }

}
