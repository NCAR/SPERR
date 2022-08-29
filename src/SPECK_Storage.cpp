#include "SPECK_Storage.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <cstring>
#include <numeric>

using d2_type = std::array<double, 2>;
using b2_type = std::array<bool, 2>;
using u2_type = std::array<uint32_t, 2>;

template <typename T>
auto sperr::SPECK_Storage::copy_data(const T* p, size_t len, dims_type dims) -> RTNType
{
  static_assert(std::is_floating_point<T>::value, "!! Only floating point values are supported !!");
  if (len != dims[0] * dims[1] * dims[2])
    return RTNType::WrongDims;

  m_coeff_buf.resize(len);
  std::copy(p, p + len, m_coeff_buf.begin());

  m_dims = dims;

  return RTNType::Good;
}
template auto sperr::SPECK_Storage::copy_data(const double*, size_t, dims_type) -> RTNType;
template auto sperr::SPECK_Storage::copy_data(const float*, size_t, dims_type) -> RTNType;

auto sperr::SPECK_Storage::take_data(vecd_type&& coeffs, dims_type dims) -> RTNType
{
  if (coeffs.size() != dims[0] * dims[1] * dims[2])
    return RTNType::WrongDims;

  m_coeff_buf = std::move(coeffs);
  m_dims = dims;

  return RTNType::Good;
}

auto sperr::SPECK_Storage::release_data() -> vecd_type&&
{
  m_dims = {0, 0, 0};
  return std::move(m_coeff_buf);
}

auto sperr::SPECK_Storage::view_data() const -> const vecd_type&
{
  return m_coeff_buf;
}

auto sperr::SPECK_Storage::release_quantized_coeff() -> vecd_type&&
{
  if (!m_qz_coeff.empty()) {
    assert(m_qz_coeff.size() == m_sign_array.size());
    const auto tmp = d2_type{-1.0, 1.0};
    std::transform(m_qz_coeff.begin(), m_qz_coeff.end(), m_sign_array.cbegin(), m_qz_coeff.begin(),
                   [tmp](auto v, auto b) { return v * tmp[b]; });
  }

  return std::move(m_qz_coeff);
}

void sperr::SPECK_Storage::set_data_range(double range)
{
  m_data_range = range;
}

auto sperr::SPECK_Storage::m_prepare_encoded_bitstream() -> RTNType
{
  // Header definition: 12 bytes in total:
  // m_max_threshold_f, stream_len
  // float,             uint64_t

  assert(m_bit_buffer.size() % 8 == 0);
  const uint64_t bit_in_byte = m_bit_buffer.size() / 8;
  const size_t total_size = m_header_size + bit_in_byte;
  m_encoded_stream.resize(total_size);
  auto* const ptr = m_encoded_stream.data();

  // Fill header
  size_t pos = 0;
  std::memcpy(ptr + pos, &m_max_threshold_f, sizeof(m_max_threshold_f));
  pos += sizeof(m_max_threshold_f);

  std::memcpy(ptr + pos, &bit_in_byte, sizeof(bit_in_byte));
  pos += sizeof(bit_in_byte);
  assert(pos == m_header_size);

  // Assemble the bitstream into bytes
  auto rtn = sperr::pack_booleans(m_encoded_stream, m_bit_buffer, pos);
  return rtn;
}

auto sperr::SPECK_Storage::parse_encoded_bitstream(const void* comp_buf, size_t comp_size)
    -> RTNType
{
  // The buffer passed in is supposed to consist a header and then a compacted
  // bitstream, just like what was returned by `prepare_encoded_bitstream()`. Note:
  // header definition is documented in prepare_encoded_bitstream().

  const uint8_t* const ptr = static_cast<const uint8_t*>(comp_buf);

  // Parse the header
  size_t pos = 0;
  std::memcpy(&m_max_threshold_f, ptr + pos, sizeof(m_max_threshold_f));
  pos += sizeof(m_max_threshold_f);

  uint64_t bit_in_byte = 0;
  std::memcpy(&bit_in_byte, ptr + pos, sizeof(bit_in_byte));
  pos += sizeof(bit_in_byte);
  if (bit_in_byte != comp_size - pos)
    return RTNType::BitstreamWrongLen;

  // Unpack bits
  const auto num_of_bools = (comp_size - pos) * 8;
  m_bit_buffer.resize(num_of_bools);  // allocate enough space before passing it around
  auto rtn = sperr::unpack_booleans(m_bit_buffer, comp_buf, comp_size, pos);
  if (rtn != RTNType::Good)
    return rtn;

  return RTNType::Good;
}

auto sperr::SPECK_Storage::view_encoded_bitstream() const -> const vec8_type&
{
  return m_encoded_stream;
}

auto sperr::SPECK_Storage::release_encoded_bitstream() -> vec8_type&&
{
  return std::move(m_encoded_stream);
}

auto sperr::SPECK_Storage::get_speck_stream_size(const void* buf) const -> uint64_t
{
  // Given the header definition in `prepare_encoded_bitstream()`, directly
  // go retrieve the value stored in the last 8 bytes of the header
  //
  const uint8_t* const ptr = static_cast<const uint8_t*>(buf);
  uint64_t bit_in_byte;
  std::memcpy(&bit_in_byte, ptr + m_header_size - 8, sizeof(bit_in_byte));

  return (m_header_size + size_t(bit_in_byte));
}

void sperr::SPECK_Storage::set_dimensions(dims_type dims)
{
  m_dims = dims;
}

auto sperr::SPECK_Storage::set_comp_params(size_t budget, double psnr, double pwe) -> RTNType
{
  // First set those ones that only need a plain copy
  m_target_psnr = psnr;
  m_target_pwe = pwe;

  // Second set bit budget, which would require a little manipulation.
  if (budget == sperr::max_size) {
    m_encode_budget = sperr::max_size;
    return RTNType::Good;
  }

  if (budget <= m_header_size * 8) {
    m_encode_budget = 0;
    return RTNType::Error;
  }
  budget -= m_header_size * 8;

  assert(budget % 8 == 0);
  m_encode_budget = budget;

  return RTNType::Good;
}

auto sperr::SPECK_Storage::m_refinement_pass_encode() -> RTNType
{
  // First, process significant pixels previously found.
  //
  const auto tmpd = d2_type{0.0, -m_threshold};
  const auto tmpdq = d2_type{m_threshold * -0.5, m_threshold * 0.5};

  assert(m_encode_budget >= m_bit_buffer.size());
  if (m_encode_budget - m_bit_buffer.size() > m_LSP_mask_sum) {  // No need to check BitBudgetMet
    for (size_t i = 0; i < m_LSP_mask.size(); i++) {
      if (m_LSP_mask[i]) {
        const bool o1 = m_coeff_buf[i] >= m_threshold;
        m_coeff_buf[i] += tmpd[o1];
        m_bit_buffer.push_back(o1);

        if (m_mode_cache == CompMode::FixedPWE)
          m_qz_coeff[i] += tmpdq[o1];
      }
    }
  }
  else {  // Need to check BitBudgetMet
    for (size_t i = 0; i < m_LSP_mask.size(); i++) {
      if (m_LSP_mask[i]) {
        const bool o1 = m_coeff_buf[i] >= m_threshold;
        m_coeff_buf[i] += tmpd[o1];
        m_bit_buffer.push_back(o1);

        if (m_mode_cache == CompMode::FixedPWE)
          m_qz_coeff[i] += tmpdq[o1];

        if (m_bit_buffer.size() >= m_encode_budget)
          return RTNType::BitBudgetMet;
      }
    }
  }

  // Second, mark newly found significant pixels in `m_LSP_mask`.
  //
  for (auto idx : m_LSP_new)
    m_LSP_mask[idx] = true;
  m_LSP_mask_sum += m_LSP_new.size();
  m_LSP_new.clear();

  return RTNType::Good;
}

auto sperr::SPECK_Storage::m_refinement_pass_decode() -> RTNType
{
  // First, process significant pixels previously found.
  //
  const auto tmpd = d2_type{m_threshold * -0.5, m_threshold * 0.5};

  assert(m_bit_buffer.size() >= m_bit_idx);
  if (m_bit_buffer.size() - m_bit_idx > m_LSP_mask_sum) {  // No need to check BitBudgetMet
    for (size_t i = 0; i < m_LSP_mask.size(); i++) {
      if (m_LSP_mask[i])
        m_coeff_buf[i] += tmpd[m_bit_buffer[m_bit_idx++]];
    }
  }
  else {  // Need to check BitBudgetMet
    for (size_t i = 0; i < m_LSP_mask.size(); i++) {
      if (m_LSP_mask[i]) {
        m_coeff_buf[i] += tmpd[m_bit_buffer[m_bit_idx++]];
        if (m_bit_idx >= m_bit_buffer.size())
          return RTNType::BitBudgetMet;
      }
    }
  }

  // Second, mark newly found significant pixels in `m_LSP_mark`
  //
  for (auto idx : m_LSP_new)
    m_LSP_mask[idx] = true;
  m_LSP_mask_sum += m_LSP_new.size();
  m_LSP_new.clear();

  return RTNType::Good;
}

auto sperr::SPECK_Storage::m_termination_check(size_t bitplane_idx) const -> RTNType
{
  assert(m_mode_cache != CompMode::Unknown);

  switch (m_mode_cache) {
    case CompMode::FixedPSNR: {
      // Encoding terminates when both conditions are met:
      // 1) the pre-estimated level of quantization is reached, and
      // 2) the estimated PSNR is bigger than `m_target_psnr`.
      if (bitplane_idx + 1 >= m_num_bitplanes) {
        const auto mse = m_estimate_mse();
        const auto psnr = std::log10(m_data_range * m_data_range / mse) * 10.0;
        if (psnr > m_target_psnr)
          return RTNType::PSNRReached;
        else
          return RTNType::DontTerminate;
      }
      else
        return RTNType::DontTerminate;
    }
    case CompMode::FixedPWE: {
      if (bitplane_idx + 1 >= m_num_bitplanes)
        return RTNType::PWEAlmostReached;
      else
        return RTNType::DontTerminate;
    }
    default:
      // This is the fixed-size mode, which should always not terminate.
      return RTNType::DontTerminate;
  }
}

auto sperr::SPECK_Storage::m_estimate_mse() const -> double
{
  const auto half_t = m_threshold * 0.5;
  const auto len = m_coeff_buf.size();
  const size_t stride_size = 4096;
  const size_t num_strides = len / stride_size;
  const size_t remainder_size = len - stride_size * num_strides;
  auto tmp_buf = vecd_type(num_strides + 1);

  for (size_t i = 0; i < num_strides; i++) {
    tmp_buf[i] = 0.0;
    for (size_t j = i * stride_size; j < (i + 1) * stride_size; j++) {
      auto diff = m_coeff_buf[j];
      if (m_LSP_mask[j])
        diff = std::remainder(m_coeff_buf[j] + half_t, m_threshold);
      tmp_buf[i] += diff * diff;
    }
  }

  // Let's also process the last stride.
  tmp_buf[num_strides] = 0.0;
  for (size_t j = num_strides * stride_size; j < len; j++) {
    auto diff = m_coeff_buf[j];
    if (m_LSP_mask[j])
      diff = std::remainder(m_coeff_buf[j] + half_t, m_threshold);
    tmp_buf[num_strides] += diff * diff;
  }

  const auto total_sum = std::accumulate(tmp_buf.cbegin(), tmp_buf.cend(), 0.0);
  const auto mse = total_sum / static_cast<double>(len);

  return mse;
}

auto sperr::SPECK_Storage::m_estimate_rmse(double q) const -> double
{
  const auto half_q = q * 0.5;
  const auto len = m_coeff_buf.size();
  const size_t stride_size = 4096;
  const size_t num_strides = len / stride_size;
  const size_t remainder_size = len - stride_size * num_strides;
  auto tmp_buf = vecd_type(num_strides + 1);

  for (size_t i = 0; i < num_strides; i++) {
    const auto beg = m_coeff_buf.cbegin() + i * stride_size;
    tmp_buf[i] = std::accumulate(beg, beg + stride_size, 0.0, [half_q, q](auto init, auto v) {
      auto diff = v >= q ? std::remainder(v + half_q, q) : v;
      return init + diff * diff;
    });
  }

  // Let's also process the last stride.
  tmp_buf[num_strides] = 0.0;
  tmp_buf[num_strides] = std::accumulate(m_coeff_buf.cbegin() + num_strides * stride_size,
                                         m_coeff_buf.cend(), 0.0, [half_q, q](auto init, auto v) {
                                           auto diff = v >= q ? std::remainder(v + half_q, q) : v;
                                           return init + diff * diff;
                                         });
  const auto total_sum = std::accumulate(tmp_buf.cbegin(), tmp_buf.cend(), 0.0);
  const auto mse = total_sum / static_cast<double>(len);

  return std::sqrt(mse);
}

auto sperr::SPECK_Storage::m_estimate_finest_q() const -> double
{
  if (m_mode_cache != CompMode::FixedPWE)
    return 0.0;

  const auto rmse_high = m_target_pwe * 0.5; // 2 sigma, 4.55%
  auto q = 2 * std::sqrt(3.0) * 0.4 * m_target_pwe; // 2.5 sigma, 1.24%
  while (m_estimate_rmse(q) > rmse_high)
    q /= 1.5;

  //auto q_min = 0.0;
  //auto q_max = q * 2.0;
  //while (m_estimate_rmse(q_max) < rmse_low)
  //  q_max *= 2.0;
  //const int n_itr = 16;
  //for (int itr = 0; itr < n_itr; itr++) {
  //  const auto rmse = m_estimate_rmse(q);
  //  if (rmse < rmse_low)
  //    q_min = q;
  //  else if (rmse >= rmse_high)
  //    q_max = q;
  //  else {
  //    std::printf("Meet requirement at itr = %d, final q = %.2e\n", itr, q);
  //    break;
  //  }
  //  if (itr == n_itr - 1)
  //    std::printf("Not meet requirement at itr = %d\n", itr);

  //  q = (q_min + q_max) * 0.5;
  //}

  return q;







  // const auto total_vals_d = static_cast<double>(m_dims[0] * m_dims[1] * m_dims[2]);
  // Iteration 1, didn't work too well
  // const auto sqrt3 = std::sqrt(3.0);
  // double p = 0.0;
  // double q = sqrt3 * m_target_pwe / std::sqrt(p * 3.0 + 1.0);
  // for (int i = 0; i < 10; i++) {
  //   auto cnt = std::count_if(m_coeff_buf.cbegin(), m_coeff_buf.cend(),
  //                            [q](auto v) { return std::abs(v) < q; });
  //   auto p_new = static_cast<double>(cnt) / total_vals_d;
  //   q = sqrt3 * m_target_pwe / std::sqrt(p_new * 3.0 + 1.0);
  //   if (std::abs(p - p_new) < 0.01) {
  //     std::printf("p * 100 = %.2f\n", p_new * 100.0);
  //     break;
  //   }
  //   else
  //     p = p_new;
  // }

  // const auto tmse = m_target_pwe * m_target_pwe * 0.25;
  // const auto converge_thrd = tmse * 1e-3;
  // const auto var = sperr::calc_variance(m_coeff_buf.data(), m_coeff_buf.size());
  // const auto sigma = std::sqrt(var);
  // const auto b = sigma / std::sqrt(2.0);
  // auto qmin = 0.0, q = 0.0;
  // auto qmax = m_target_pwe * 4.0;

  // for (size_t i = 0; i < 1024; i++) {
  //   q = (qmin + qmax) * 0.5;
  //   const auto h = std::exp(q / b);
  //   const auto emse = 2.0 * b * (b + q / (1.0 - h)) - q * (0.75 * q + b) / h;
  //   if (std::abs(tmse - emse) < converge_thrd) {
  //     std::printf("%lu, %.4e, %.4e, %.4e, %.4e, %.4e\n",
  //                 i, m_target_pwe, tmse, emse, sigma, q);
  //     break;
  //   }
  //   else if (emse > tmse)
  //     qmax = q;
  //   else
  //     qmin = q;

  //  if (i == 1023)
  //    std::printf("!!Failed to converge!!\n");
  //}
  // return q;
}
