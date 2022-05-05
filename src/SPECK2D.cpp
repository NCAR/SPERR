#include "SPECK2D.h"
#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <cstring>
#include <iostream>

using d2_type = std::array<double, 2>;
using b2_type = std::array<bool, 2>;

//
// Class SPECKSet2D
//
auto sperr::SPECKSet2D::is_pixel() const -> bool
{
  return (length_x == 1 && length_y == 1);
}

auto sperr::SPECKSet2D::is_empty() const -> bool
{
  return (length_x == 0 || length_y == 0);
}

//
// Class SPECK2D
//
sperr::SPECK2D::SPECK2D()
{
  m_I.type = SetType::TypeI;
  m_dims[2] = 1;
}

#ifdef QZ_TERM
void sperr::SPECK2D::set_quantization_level(int32_t lev)
{
  m_qz_lev = lev;
}
#else
void sperr::SPECK2D::set_bit_budget(size_t budget)
{
  if (budget <= m_header_size) {
    m_budget = 0;
    return;
  }
  budget -= m_header_size;

  size_t mod = budget % 8;
  if (mod == 0)
    m_budget = budget;
  else  // we can fill up that last byte!
    m_budget = budget + 8 - mod;
}
#endif

auto sperr::SPECK2D::encode() -> RTNType
{
  if (!m_ready_to_encode())
    return RTNType::Error;
  m_encode_mode = true;

  m_initialize_sets_lists();

  m_encoded_stream.clear();
  m_bit_buffer.clear();

  // Keep signs of all coefficients
  m_sign_array.resize(m_coeff_buf.size());
  std::transform(m_coeff_buf.cbegin(), m_coeff_buf.cend(), m_sign_array.begin(),
                 [](auto e) { return e >= 0.0; });

  // Make every coefficient positive
  std::transform(m_coeff_buf.cbegin(), m_coeff_buf.cend(), m_coeff_buf.begin(),
                 [](auto v) { return std::abs(v); });

  // Find the threshold to start the algorithm
  const auto max_coeff = *std::max_element(m_coeff_buf.begin(), m_coeff_buf.end());
  m_max_coeff_bits = static_cast<int32_t>(std::floor(std::log2(max_coeff)));

#ifdef QZ_TERM
  m_threshold_arr[0] = std::pow(2.0, static_cast<double>(m_max_coeff_bits));
  std::generate(m_threshold_arr.begin(), m_threshold_arr.end(),
                [v = m_threshold_arr[0]]() mutable { return std::exchange(v, v * 0.5); });

  // If the requested termination level is already above max_coeff_bits, return right away.
  if (m_qz_lev > m_max_coeff_bits)
    return RTNType::QzLevelTooBig;

  const auto num_qz_levs = m_max_coeff_bits - m_qz_lev + 1;
  for (m_threshold_idx = 0; m_threshold_idx < num_qz_levs; m_threshold_idx++) {
    m_sorting_pass_encode();
    m_clean_LIS();
  }

  // Fill the bit buffer to multiplies of eight
  while (m_bit_buffer.size() % 8 != 0)
    m_bit_buffer.push_back(false);

#else

  m_threshold = std::pow(2.0, static_cast<double>(m_max_coeff_bits));
  for (size_t bitplane = 0; bitplane < 64; bitplane++) {
    auto rtn = m_sorting_pass_encode();
    if (rtn == RTNType::BitBudgetMet)
      break;
    assert(rtn == RTNType::Good);

    rtn = m_refinement_pass_encode();
    if (rtn == RTNType::BitBudgetMet)
      break;
    assert(rtn == RTNType::Good);

    m_threshold *= 0.5;
    m_clean_LIS();
  }
#endif

  // Finally we prepare the bitstream
  auto rtn = m_prepare_encoded_bitstream();
  return rtn;
}

auto sperr::SPECK2D::decode() -> RTNType
{
  if (!m_ready_to_decode())
    return RTNType::Error;
  m_encode_mode = false;

#ifndef QZ_TERM
  // By default, decode all the available bits
  if (m_budget == 0 || m_budget > m_bit_buffer.size())
    m_budget = m_bit_buffer.size();
#endif

  // initialize coefficients to be zero, and signs to be all positive
  const auto coeff_len = m_dims[0] * m_dims[1];
  m_coeff_buf.assign(coeff_len, 0.0);
  m_sign_array.assign(coeff_len, true);

  m_initialize_sets_lists();
  m_bit_idx = 0;

#ifdef QZ_TERM
  m_threshold_arr[0] = std::pow(2.0, static_cast<double>(m_max_coeff_bits));
  std::generate(m_threshold_arr.begin(), m_threshold_arr.end(),
                [v = m_threshold_arr[0]]() mutable { return std::exchange(v, v * 0.5); });

  for (m_threshold_idx = 0; m_threshold_idx < m_threshold_arr.size(); m_threshold_idx++) {
    auto rtn = m_sorting_pass_decode();
    assert(rtn == RTNType::Good);
    if (m_bit_idx > m_bit_buffer.size())
      return RTNType::Error;
    // This is the actual termination condition in QZ_TERM mode.
    assert(m_max_coeff_bits >= m_qz_lev);
    if (m_threshold_idx >= static_cast<size_t>(m_max_coeff_bits - m_qz_lev))
      break;
    m_clean_LIS();
  }

  // We should not have more than 7 unprocessed bits left in the bit buffer!
  if (m_bit_idx > m_bit_buffer.size() || m_bit_buffer.size() - m_bit_idx >= 8)
    return RTNType::BitstreamWrongLen;

#else

  m_threshold = std::pow(2.0, static_cast<double>(m_max_coeff_bits));
  for (size_t bitplane = 0; bitplane < 64; bitplane++) {
    auto rtn = m_sorting_pass_decode();
    if (rtn == RTNType::BitBudgetMet)
      break;
    assert(rtn == RTNType::Good);
    rtn = m_refinement_pass_decode();
    if (rtn == RTNType::BitBudgetMet)
      break;
    assert(rtn == RTNType::Good);
    m_threshold *= 0.5;
    m_clean_LIS();
  }

  // If decoding finished before all newly-identified significant pixels are initialized in
  // `m_refinement_pass_decode()`, we have them initialized here.
  for (auto idx : m_LSP_new)
    m_coeff_buf[idx] = m_threshold * 1.5;
  m_LSP_new.clear();
#endif

  // Restore coefficient signs
  const auto tmp = d2_type{-1.0, 1.0};
  for (size_t i = 0; i < m_sign_array.size(); i++)
    m_coeff_buf[i] *= tmp[m_sign_array[i]];

  return RTNType::Good;
}

void sperr::SPECK2D::m_initialize_sets_lists()
{
  // prepare m_LIS
  const auto num_of_parts =
      std::max(sperr::num_of_partitions(m_dims[0]), sperr::num_of_partitions(m_dims[1]));
  m_LIS.resize(num_of_parts + 1);
  for (auto& list : m_LIS)
    list.clear();
  m_LIP.clear();
  m_LIP.reserve(m_coeff_buf.size() / 2);

  // prepare the root, S
  auto S = m_produce_root();
  m_LIS[S.part_level].emplace_back(S);

  // prepare m_I
  m_I.part_level = sperr::num_of_xforms(std::min(m_dims[0], m_dims[1]));
  m_I.start_x = S.length_x;
  m_I.start_y = S.length_y;
  m_I.length_x = m_dims[0];
  m_I.length_y = m_dims[1];

#ifndef QZ_TERM
  // clear lists and reserve space.
  m_LSP_new.clear();
  m_LSP_new.reserve(m_coeff_buf.size() / 8);
  m_LSP_old.clear();
  m_LSP_old.reserve(m_coeff_buf.size() / 2);
  m_bit_buffer.reserve(m_budget);
#endif
}

//
// Private methods
//
auto sperr::SPECK2D::m_sorting_pass_encode() -> RTNType
{
  // First, process all insignificant pixels
  //
  size_t dummy = 0;
  for (size_t idx = 0; idx < m_LIP.size(); idx++) {
    auto rtn = m_process_P_encode(idx, dummy, true);
#ifndef QZ_TERM
    if (rtn == RTNType::BitBudgetMet)
      return rtn;
#endif
    assert(rtn == RTNType::Good);
  }

  // Second, process all insignificant sets
  //
  for (size_t tmp = 0; tmp < m_LIS.size(); tmp++) {
    // From the end to the front of m_LIS, smaller sets first.
    size_t idx1 = m_LIS.size() - 1 - tmp;
    for (size_t idx2 = 0; idx2 < m_LIS[idx1].size(); idx2++) {
      auto rtn = m_process_S_encode(idx1, idx2, dummy, true);
#ifndef QZ_TERM
      if (rtn == RTNType::BitBudgetMet)
        return rtn;
#endif
      assert(rtn == RTNType::Good);
    }
  }

  auto rtn = m_process_I(true);
#ifndef QZ_TERM
  if (rtn == RTNType::BitBudgetMet)
    return rtn;
#endif
  assert(rtn == RTNType::Good);

  return RTNType::Good;
}

auto sperr::SPECK2D::m_sorting_pass_decode() -> RTNType
{
  // First, process all insignificant pixels
  //
  size_t dummy = 0;
  for (size_t idx = 0; idx < m_LIP.size(); idx++) {
    auto rtn = m_process_P_decode(idx, dummy, true);
#ifndef QZ_TERM
    if (rtn == RTNType::BitBudgetMet)
      return rtn;
#endif
    assert(rtn == RTNType::Good);
  }

  // Second, process all insignificant sets
  //
  for (size_t tmp = 0; tmp < m_LIS.size(); tmp++) {
    // From the end to the front of m_LIS, smaller sets first.
    size_t idx1 = m_LIS.size() - 1 - tmp;
    for (size_t idx2 = 0; idx2 < m_LIS[idx1].size(); idx2++) {
      auto rtn = m_process_S_decode(idx1, idx2, dummy, true);
#ifndef QZ_TERM
      if (rtn == RTNType::BitBudgetMet)
        return rtn;
#endif
      assert(rtn == RTNType::Good);
    }
  }

  auto rtn = m_process_I(true);
#ifndef QZ_TERM
  if (rtn == RTNType::BitBudgetMet)
    return rtn;
#endif
  assert(rtn == RTNType::Good);

  return RTNType::Good;
}

#ifdef QZ_TERM
void sperr::SPECK2D::m_quantize_P_encode(size_t idx)
{
  auto coeff = m_coeff_buf[idx] - m_threshold_arr[m_threshold_idx];
  const auto tmpb = b2_type{false, true};
  assert(m_max_coeff_bits >= m_qz_lev);
  const size_t num_qz_levs = m_max_coeff_bits - m_qz_lev + 1;
  auto tmpd = d2_type{0.0, 0.0};
  for (auto i = m_threshold_idx + 1; i < num_qz_levs; i++) {
    tmpd[1] = m_threshold_arr[i];
    const size_t o1 = coeff >= m_threshold_arr[i];
    coeff -= tmpd[o1];
    m_bit_buffer.push_back(tmpb[o1]);
  }
  m_coeff_buf[idx] = coeff;
}

void sperr::SPECK2D::m_quantize_P_decode(size_t idx)
{
  auto coeff = m_threshold_arr[m_threshold_idx] * 1.5;
  assert(m_max_coeff_bits >= m_qz_lev);
  const size_t num_qz_levs = m_max_coeff_bits - m_qz_lev + 1;
  for (auto i = m_threshold_idx + 1; i < num_qz_levs; i++) {
    const auto tmp = d2_type{-m_threshold_arr[i + 1], m_threshold_arr[i + 1]};
    coeff += tmp[m_bit_buffer[m_bit_idx++]];
  }
  m_coeff_buf[idx] = coeff;
}

#else

auto sperr::SPECK2D::m_refinement_pass_encode() -> RTNType
{
  // First, process `m_LSP_old`
  //
  const auto tmpb = b2_type{false, true};
  const auto tmpd = d2_type{0.0, -m_threshold};
  const auto n_to_process = std::min(m_LSP_old.size(), m_budget - m_bit_buffer.size());
  for (size_t i = 0; i < n_to_process; i++) {
    auto loc = m_LSP_old[i];
    const size_t o1 = m_coeff_buf[loc] >= m_threshold;
    m_coeff_buf[loc] += tmpd[o1];
    m_bit_buffer.push_back(tmpb[o1]);
  }
  if (m_bit_buffer.size() >= m_budget)
    return RTNType::BitBudgetMet;

  // Second, process `m_LSP_new`
  //
  for (auto idx : m_LSP_new)
    m_coeff_buf[idx] -= m_threshold;

  // Third, attach `m_LSP_new` to the end of `m_LSP_old`, and clear `m_LSP_new`.
  //
  m_LSP_old.insert(m_LSP_old.end(), m_LSP_new.cbegin(), m_LSP_new.cend());
  m_LSP_new.clear();

  return RTNType::Good;
}

auto sperr::SPECK2D::m_refinement_pass_decode() -> RTNType
{
  // First, process `m_LSP_old`
  //
  const auto tmp = d2_type{m_threshold * -0.5, m_threshold * 0.5};
  const auto n_to_process = std::min(m_LSP_old.size(), m_budget - m_bit_idx);

  for (size_t i = 0; i < n_to_process; i++)
    m_coeff_buf[m_LSP_old[i]] += tmp[m_bit_buffer[m_bit_idx + i]];
  m_bit_idx += n_to_process;
  if (m_bit_idx >= m_budget)
    return RTNType::BitBudgetMet;

  // Second, process `m_LSP_new`
  //
  for (auto idx : m_LSP_new)
    m_coeff_buf[idx] = m_threshold * 1.5;

  // Third, attach `m_LSP_new` to the end of `m_LSP_old`, and clear `m_LSP_new`.
  //
  m_LSP_old.insert(m_LSP_old.end(), m_LSP_new.cbegin(), m_LSP_new.cend());
  m_LSP_new.clear();

  return RTNType::Good;
}
#endif

auto sperr::SPECK2D::m_process_P_encode(size_t loc, size_t& counter, bool need_decide_sig)
    -> RTNType
{
  const auto pixel_idx = m_LIP[loc];
  bool is_sig = true;

  if (need_decide_sig) {
#ifdef QZ_TERM
    const auto thrd = m_threshold_arr[m_threshold_idx];
#else
    const auto thrd = m_threshold;
#endif

    const size_t o1 = (m_coeff_buf[pixel_idx] >= thrd);
    const auto tmpb = b2_type{false, true};
    is_sig = tmpb[o1];
    m_bit_buffer.push_back(is_sig);
#ifndef QZ_TERM
    if (m_bit_buffer.size() >= m_budget)
      return RTNType::BitBudgetMet;
#endif
  }

  if (is_sig) {
    counter++;
    m_bit_buffer.push_back(m_sign_array[pixel_idx]);
#ifndef QZ_TERM
    if (m_bit_buffer.size() >= m_budget)
      return RTNType::BitBudgetMet;
#endif

#ifdef QZ_TERM
    m_quantize_P_encode(pixel_idx);
#else
    m_LSP_new.push_back(pixel_idx);
#endif

    m_LIP[loc] = m_u64_garbage_val;
  }

  return RTNType::Good;
}

auto sperr::SPECK2D::m_process_P_decode(size_t loc, size_t& counter, bool need_decide_sig)
    -> RTNType
{
  const auto pixel_idx = m_LIP[loc];
  bool is_sig = true;

  if (need_decide_sig) {
#ifndef QZ_TERM
    if (m_bit_idx >= m_budget)
      return RTNType::BitBudgetMet;
#endif
    is_sig = m_bit_buffer[m_bit_idx++];
  }

  if (is_sig) {
    counter++;
#ifndef QZ_TERM
    if (m_bit_idx >= m_budget)
      return RTNType::BitBudgetMet;
#endif
    m_sign_array[pixel_idx] = m_bit_buffer[m_bit_idx++];

#ifdef QZ_TERM
    m_quantize_P_decode(pixel_idx);
#else
    m_LSP_new.push_back(pixel_idx);
#endif

    m_LIP[loc] = m_u64_garbage_val;
  }

  return RTNType::Good;
}

auto sperr::SPECK2D::m_process_S_encode(size_t idx1,
                                        size_t idx2,
                                        size_t& counter,
                                        bool need_decide_sig) -> RTNType
{
  auto& set = m_LIS[idx1][idx2];
  assert(!set.is_pixel());
  set.signif = SigType::Sig;

  if (need_decide_sig) {
    set.signif = m_decide_set_S_significance(set);
    m_bit_buffer.push_back(set.signif == SigType::Sig);
#ifndef QZ_TERM
    if (m_bit_buffer.size() >= m_budget)
      return RTNType::BitBudgetMet;
#endif
  }

  if (set.signif == SigType::Sig) {
    counter++;  // increment the significant counter first!
    auto rtn = m_code_S_encode(idx1, idx2);
#ifndef QZ_TERM
    if (rtn == RTNType::BitBudgetMet)
      return rtn;
#endif
    assert(rtn == RTNType::Good);
    set.type = SetType::Garbage;  // This particular set will be discarded.
  }

  return RTNType::Good;
}

auto sperr::SPECK2D::m_process_S_decode(size_t idx1,
                                        size_t idx2,
                                        size_t& counter,
                                        bool need_decide_sig) -> RTNType
{
  auto& set = m_LIS[idx1][idx2];
  assert(!set.is_pixel());
  set.signif = SigType::Sig;

  if (need_decide_sig) {
    const auto tmps = std::array<SigType, 2>{SigType::Insig, SigType::Sig};
#ifndef QZ_TERM
    if (m_bit_idx >= m_budget)
      return RTNType::BitBudgetMet;
#endif
    set.signif = tmps[m_bit_buffer[m_bit_idx++]];
  }

  if (set.signif == SigType::Sig) {
    counter++;  // increment the significant counter first!
    auto rtn = m_code_S_decode(idx1, idx2);
#ifndef QZ_TERM
    if (rtn == RTNType::BitBudgetMet)
      return rtn;
#endif
    assert(rtn == RTNType::Good);
    set.type = SetType::Garbage;  // This particular object will be discarded.
  }

  return RTNType::Good;
}

auto sperr::SPECK2D::m_code_S_encode(size_t idx1, size_t idx2) -> RTNType
{
  const auto& set = m_LIS[idx1][idx2];
  auto subsets = m_partition_S(set);
  // Put empty subsets at the end of this list
  const auto set_end =
      std::remove_if(subsets.begin(), subsets.end(), [](const auto& s) { return s.is_empty(); });
  const auto set_end_m1 = set_end - 1;

  // We count how many subsets are significant, and if no significant set encountered until
  // the last non-empty one, then the last one must be significant.
  auto sig_counter = size_t{0};
  for (auto it = subsets.begin(); it != set_end; ++it) {
    auto need_decide_sig = (it != set_end_m1) || (sig_counter != 0);
    auto rtn = RTNType::Good;
    if (it->is_pixel()) {
      const auto pixel_idx = it->start_y * m_dims[0] + it->start_x;
      m_LIP.push_back(pixel_idx);
      rtn = m_process_P_encode(m_LIP.size() - 1, sig_counter, need_decide_sig);
    }
    else {
      size_t newidx1 = it->part_level;
      m_LIS[newidx1].emplace_back(*it);
      size_t newidx2 = m_LIS[newidx1].size() - 1;
      rtn = m_process_S_encode(newidx1, newidx2, sig_counter, need_decide_sig);
    }
#ifndef QZ_TERM
    if (rtn == RTNType::BitBudgetMet)
      return rtn;
#endif
    assert(rtn == RTNType::Good);
  }

  return RTNType::Good;
}

auto sperr::SPECK2D::m_code_S_decode(size_t idx1, size_t idx2) -> RTNType
{
  const auto& set = m_LIS[idx1][idx2];
  auto subsets = m_partition_S(set);
  // Put empty subsets at the end of this list
  const auto set_end =
      std::remove_if(subsets.begin(), subsets.end(), [](const auto& s) { return s.is_empty(); });
  const auto set_end_m1 = set_end - 1;

  // We count how many subsets are significant, and if no significant set encountered until
  // the last non-empty one, then the last one must be significant.
  auto sig_counter = size_t{0};
  for (auto it = subsets.begin(); it != set_end; ++it) {
    auto need_decide_sig = (it != set_end_m1) || (sig_counter != 0);
    auto rtn = RTNType::Good;
    if (it->is_pixel()) {
      const auto pixel_idx = it->start_y * m_dims[0] + it->start_x;
      m_LIP.push_back(pixel_idx);
      rtn = m_process_P_decode(m_LIP.size() - 1, sig_counter, need_decide_sig);
    }
    else {
      size_t newidx1 = it->part_level;
      m_LIS[newidx1].emplace_back(*it);
      size_t newidx2 = m_LIS[newidx1].size() - 1;
      rtn = m_process_S_decode(newidx1, newidx2, sig_counter, need_decide_sig);
    }
#ifndef QZ_TERM
    if (rtn == RTNType::BitBudgetMet)
      return rtn;
#endif
    assert(rtn == RTNType::Good);
  }

  return RTNType::Good;
}

auto sperr::SPECK2D::m_partition_S(const SPECKSet2D& set) const -> std::array<SPECKSet2D, 4>
{
  auto subsets = std::array<SPECKSet2D, 4>();

  // The top-left set will have these bigger dimensions in case that
  // the current set has odd dimensions.
  const auto detail_len_x = set.length_x / 2;
  const auto detail_len_y = set.length_y / 2;
  const auto approx_len_x = set.length_x - detail_len_x;
  const auto approx_len_y = set.length_y - detail_len_y;

  // Put generated subsets in the list the same order as did in QccPack.
  auto& BR = subsets[0];  // Bottom right set
  BR.part_level = set.part_level + 1;
  BR.start_x = set.start_x + approx_len_x;
  BR.start_y = set.start_y + approx_len_y;
  BR.length_x = detail_len_x;
  BR.length_y = detail_len_y;

  auto& BL = subsets[1];  // Bottom left set
  BL.part_level = set.part_level + 1;
  BL.start_x = set.start_x;
  BL.start_y = set.start_y + approx_len_y;
  BL.length_x = approx_len_x;
  BL.length_y = detail_len_y;

  auto& TR = subsets[2];  // Top right set
  TR.part_level = set.part_level + 1;
  TR.start_x = set.start_x + approx_len_x;
  TR.start_y = set.start_y;
  TR.length_x = detail_len_x;
  TR.length_y = approx_len_y;

  auto& TL = subsets[3];  // Top left set
  TL.part_level = set.part_level + 1;
  TL.start_x = set.start_x;
  TL.start_y = set.start_y;
  TL.length_x = approx_len_x;
  TL.length_y = approx_len_y;

  return subsets;
}

auto sperr::SPECK2D::m_process_I(bool need_decide_sig) -> RTNType
{
  if (m_I.part_level == 0)  // m_I is empty at this point
    return RTNType::Good;

  if (m_encode_mode) {
    if (need_decide_sig)
      m_I.signif = m_decide_set_I_significance(m_I);
    else
      m_I.signif = SigType::Sig;

    m_bit_buffer.push_back(m_I.signif == SigType::Sig);
#ifndef QZ_TERM
    if (m_bit_buffer.size() >= m_budget)
      return RTNType::BitBudgetMet;
#endif
  }
  else {
#ifndef QZ_TERM
    if (m_bit_idx >= m_budget)
      return RTNType::BitBudgetMet;
#endif
    m_I.signif = m_bit_buffer[m_bit_idx++] ? SigType::Sig : SigType::Insig;
  }

  if (m_I.signif == SigType::Sig) {
    auto rtn = m_code_I();
#ifndef QZ_TERM
    if (rtn == RTNType::BitBudgetMet)
      return rtn;
#endif
    assert(rtn == RTNType::Good);
  }

  return RTNType::Good;
}

auto sperr::SPECK2D::m_code_I() -> RTNType
{
  auto subsets = m_partition_I();

  // We count how many subsets are significant, and if the 3 subsets resulted
  // from m_partition_I() are all insignificant, then it must be the remaining
  // `m_I` to be significant.
  auto sig_counter = size_t{0};
  for (const auto& s : subsets) {
    if (!s.is_empty()) {
      m_LIS[s.part_level].emplace_back(s);
      auto newidx1 = size_t{s.part_level};
      auto newidx2 = m_LIS[newidx1].size() - 1;
      auto rtn = RTNType::Good;
      if (m_encode_mode)
        rtn = m_process_S_encode(newidx1, newidx2, sig_counter, true);
      else
        rtn = m_process_S_decode(newidx1, newidx2, sig_counter, true);
#ifndef QZ_TERM
      if (rtn == RTNType::BitBudgetMet)
        return rtn;
#endif
      assert(rtn == RTNType::Good);
    }
  }

  auto rtn = m_process_I(sig_counter != 0);
#ifndef QZ_TERM
  if (rtn == RTNType::BitBudgetMet)
    return rtn;
#endif
  assert(rtn == RTNType::Good);

  return RTNType::Good;
}

auto sperr::SPECK2D::m_partition_I() -> std::array<SPECKSet2D, 3>
{
  auto subsets = std::array<SPECKSet2D, 3>();
  auto len_x = sperr::calc_approx_detail_len(m_dims[0], m_I.part_level);
  auto len_y = sperr::calc_approx_detail_len(m_dims[1], m_I.part_level);
  const auto approx_len_x = len_x[0];
  const auto detail_len_x = len_x[1];
  const auto approx_len_y = len_y[0];
  const auto detail_len_y = len_y[1];

  // Specify the subsets following the same order in QccPack
  auto& BR = subsets[0];  // Bottom right
  BR.part_level = m_I.part_level;
  BR.start_x = approx_len_x;
  BR.start_y = approx_len_y;
  BR.length_x = detail_len_x;
  BR.length_y = detail_len_y;

  auto& TR = subsets[1];  // Top right
  TR.part_level = m_I.part_level;
  TR.start_x = approx_len_x;
  TR.start_y = 0;
  TR.length_x = detail_len_x;
  TR.length_y = approx_len_y;

  auto& BL = subsets[2];  // Bottom left
  BL.part_level = m_I.part_level;
  BL.start_x = 0;
  BL.start_y = approx_len_y;
  BL.length_x = approx_len_x;
  BL.length_y = detail_len_y;

  // Also update m_I
  m_I.part_level--;
  m_I.start_x += detail_len_x;
  m_I.start_y += detail_len_y;

  return subsets;
}

auto sperr::SPECK2D::m_decide_set_S_significance(const SPECKSet2D& set) -> SigType
{
  // For TypeS sets, we test an obvious rectangle specified by this set.
  assert(set.type == SetType::TypeS);

#ifdef QZ_TERM
  const auto thrd = m_threshold_arr[m_threshold_idx];
#else
  const auto thrd = m_threshold;
#endif

  const auto gtr = [thrd](auto v) { return v >= thrd; };
  for (auto y = set.start_y; y < (set.start_y + set.length_y); y++) {
    auto first = m_coeff_buf.begin() + y * m_dims[0] + set.start_x;
    auto last = first + set.length_x;
    if (std::any_of(first, last, gtr))
      return SigType::Sig;
  }
  return SigType::Insig;
}

auto sperr::SPECK2D::m_decide_set_I_significance(const SPECKSet2D& set) -> SigType
{
  // For TypeI sets, we need to test two rectangles!
  // First rectangle: directly to the right of the missing top-left corner
  assert(set.type == SetType::TypeI);

#ifdef QZ_TERM
  const auto thrd = m_threshold_arr[m_threshold_idx];
#else
  const auto thrd = m_threshold;
#endif

  const auto gtr = [thrd](auto v) { return v >= thrd; };
  for (size_t y = 0; y < set.start_y; y++) {
    auto first = m_coeff_buf.begin() + y * m_dims[0] + set.start_x;
    auto last = m_coeff_buf.begin() + (y + 1) * m_dims[0];
    if (std::any_of(first, last, gtr))
      return SigType::Sig;
  }

  // Second rectangle: the rest area at the bottom
  // Note: this rectangle is stored in a contiguous chunk of memory till the end of the buffer :)
  auto first = m_coeff_buf.begin() + size_t{set.start_y} * size_t{set.length_x};
  if (std::any_of(first, m_coeff_buf.end(), gtr))
    return SigType::Sig;
  else
    return SigType::Insig;
}

auto sperr::SPECK2D::m_produce_root() const -> SPECKSet2D
{
  auto num_of_xforms = sperr::num_of_xforms(std::min(m_dims[0], m_dims[1]));
  auto len_x = sperr::calc_approx_detail_len(m_dims[0], num_of_xforms);
  auto len_y = sperr::calc_approx_detail_len(m_dims[1], num_of_xforms);

  SPECKSet2D S;
  S.part_level = num_of_xforms;
  S.start_x = 0;
  S.start_y = 0;
  S.length_x = len_x[0];
  S.length_y = len_y[0];

  return S;
}

void sperr::SPECK2D::m_clean_LIS()
{
  for (auto& list : m_LIS) {
    auto it = std::remove_if(list.begin(), list.end(),
                             [](const auto& s) { return s.type == SetType::Garbage; });
    list.erase(it, list.end());
  }

  // Let's also clean up m_LIP.
  auto it = std::remove(m_LIP.begin(), m_LIP.end(), m_u64_garbage_val);
  m_LIP.erase(it, m_LIP.end());
}

auto sperr::SPECK2D::m_ready_to_encode() const -> bool
{
  if (m_dims[0] == 0 || m_dims[1] == 0 || m_dims[2] != 1)
    return false;
  if (m_coeff_buf.size() != m_dims[0] * m_dims[1])
    return false;

#ifndef QZ_TERM
  if (m_budget == 0 || m_budget > m_dims[0] * m_dims[1] * 64)
    return false;
#endif

  return true;
}

auto sperr::SPECK2D::m_ready_to_decode() const -> bool
{
  if (m_dims[0] == 0 || m_dims[1] == 0 || m_dims[2] != 1)
    return false;
  if (m_bit_buffer.empty())
    return false;

  return true;
}
