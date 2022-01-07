#include "SPECK3D.h"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstring>
#include <numeric>

//
// Class SPECKSet3D
//
auto sperr::SPECKSet3D::is_pixel() const -> bool
{
  return (length_x == 1 && length_y == 1 && length_z == 1);
}

auto sperr::SPECKSet3D::is_empty() const -> bool
{
  return (length_z == 0 || length_y == 0 || length_x == 0);
}

//
// Class SPECK3D
//
#ifdef QZ_TERM
void sperr::SPECK3D::set_quantization_term_level(int32_t lev)
{
  m_qz_term_lev = lev;
}
#else
void sperr::SPECK3D::set_bit_budget(size_t budget)
{
  if (budget <= m_header_size) {
    m_budget = 0;
    return;
  }
  budget -= m_header_size;

  size_t mod = budget % 8;
  if (mod == 0)
    m_budget = budget;
  else  // we can fill up the last byte
    m_budget = budget + 8 - mod;
}
#endif

void sperr::SPECK3D::m_clean_LIS()
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

auto sperr::SPECK3D::encode() -> RTNType
{
  if (m_ready_to_encode() == false)
    return RTNType::Error;
  m_encode_mode = true;

  m_initialize_sets_lists();

  m_encoded_stream.clear();
  m_bit_buffer.clear();

  // Keep signs of all coefficients
  m_sign_array.reserve(m_coeff_buf.size());
  std::transform(m_coeff_buf.cbegin(), m_coeff_buf.cend(), m_sign_array.begin(),
                 [](auto e) { return e >= 0.0; });

  auto const max_coeff = sperr::make_coeff_positive(m_coeff_buf);

  // When max_coeff is between 0.0 and 1.0, std::log2(max_coeff) will become a
  // negative value. std::floor() will always find the smaller integer value,
  // which will always reconstruct to a bitplane value that is smaller than
  // max_coeff. Also, when max_coeff is close to 0.0, std::log2(max_coeff) can
  // have a pretty big magnitude, so we use int32_t here.
  //
  m_max_coeff_bits = int32_t(std::floor(std::log2(max_coeff)));
  m_threshold_idx = 0;
  m_threshold_arr[0] = std::pow(2.0, double(m_max_coeff_bits));
  std::transform(m_threshold_arr.cbegin(),           //
                 std::prev(m_threshold_arr.cend()),  //
                 std::next(m_threshold_arr.begin()), [](auto e) { return e * 0.5; });

#ifdef QZ_TERM
  // If the requested termination level is already above max_coeff_bits, return right away.
  if (m_qz_term_lev > m_max_coeff_bits)
    return RTNType::QzLevelTooBig;

  const auto num_qz_levs = m_max_coeff_bits - m_qz_term_lev + 1;

  for (m_threshold_idx = 0; m_threshold_idx < num_qz_levs; m_threshold_idx++) {
    m_sorting_pass_encode();
    m_clean_LIS();
  }

  // If the bit buffer has the last byte half-empty, let's fill in zero's.
  // The decoding process will not read them.
  while (m_bit_buffer.size() % 8 != 0)
    m_bit_buffer.push_back(false);

#else

  // We say that we run 64 iterations at most.
  for (int iteration = 0; iteration < 64; iteration++) {
    // The following two functions only return `BitBudgetMet` or `Good`.
    if (m_sorting_pass_encode() == RTNType::BitBudgetMet)
      break;

    if (m_refinement_pass_encode() == RTNType::BitBudgetMet)
      break;

    m_threshold_idx++;
    m_clean_LIS();
  }
#endif

  // Finally we prepare the bitstream
  auto rtn = m_prepare_encoded_bitstream();

  return rtn;
}

auto sperr::SPECK3D::decode() -> RTNType
{
  if (m_ready_to_decode() == false)
    return RTNType::Error;
  m_encode_mode = false;

#ifndef QZ_TERM
  // By default, decode all the available bits
  if (m_budget == 0 || m_budget > m_bit_buffer.size())
    m_budget = m_bit_buffer.size();
#endif

  // initialize coefficients to be zero, and sign array to be all positive
  const auto coeff_len = m_dims[0] * m_dims[1] * m_dims[2];
  m_coeff_buf.assign(coeff_len, 0.0);
  m_sign_array.assign(coeff_len, true);

  m_initialize_sets_lists();

  m_bit_idx = 0;
  m_threshold_idx = 0;
  m_threshold_arr[0] = std::pow(2.0, double(m_max_coeff_bits));
  for (size_t i = 1; i < m_threshold_arr.size(); i++)
    m_threshold_arr[i] = m_threshold_arr[i - 1] * 0.5;

  for (size_t bitplane = 0; bitplane < 64; bitplane++) {
#ifdef QZ_TERM
    m_sorting_pass_decode();
    if (m_bit_idx > m_bit_buffer.size())
      return RTNType::Error;
    // This is the actual terminating condition in QZ_TERM mode.
    if (m_threshold_idx >= m_max_coeff_bits - m_qz_term_lev)
      break;
#else
    auto rtn = m_sorting_pass_decode();
    if (rtn == RTNType::BitBudgetMet)
      break;
    assert(rtn == RTNType::Good);
    // We need to do a refinement pass in fixed size mode.
    rtn = m_refinement_pass_decode();
    if (rtn == RTNType::BitBudgetMet)
      break;
    assert(rtn == RTNType::Good);
#endif

    m_threshold_idx++;
    m_clean_LIS();
  }

#ifdef QZ_TERM
  // We should not have more than 7 bits left in the bit buffer!
  if (m_bit_idx > m_bit_buffer.size() || m_bit_buffer.size() - m_bit_idx >= 8)
    return RTNType::Error;
#else
  // If decoding finished before all newly-sig pixels are initialized, we finish them here!
  for (auto idx : m_LSP_new)
    m_coeff_buf[idx] = m_threshold_arr[m_threshold_idx] * 1.5;
  m_LSP_new.clear();
#endif

  // Restore coefficient signs by setting some of them negative
  for (size_t i = 0; i < m_sign_array.size(); i++) {
    auto tmp = arrd2_type{-m_coeff_buf[i], m_coeff_buf[i]};
    m_coeff_buf[i] = tmp[m_sign_array[i]];
  }

  return RTNType::Good;
}

void sperr::SPECK3D::m_initialize_sets_lists()
{
  std::array<size_t, 3> num_of_parts;  // how many times each dimension could be partitioned?
  num_of_parts[0] = sperr::num_of_partitions(m_dims[0]);
  num_of_parts[1] = sperr::num_of_partitions(m_dims[1]);
  num_of_parts[2] = sperr::num_of_partitions(m_dims[2]);
  size_t num_of_sizes = 1;
  for (size_t i = 0; i < 3; i++)
    num_of_sizes += num_of_parts[i];

  // Initialize LIS
  // Note that `m_LIS` is a two-dimensional array. We want to keep the memory
  // allocated in the secondary array, so we don't clear `m_LIS` itself, but
  // clear the every secondary arrays inside of it. Also note that we don't
  // shrink the size of `m_LIS`. This is OK as long as the extra lists at the
  // end are cleared.
  if (m_LIS.size() < num_of_sizes)
    m_LIS.resize(num_of_sizes);
  for (auto& list : m_LIS)
    list.clear();

  // Starting from a set representing the whole volume, identify the smaller
  // sets and put them in LIS accordingly.
  SPECKSet3D big;
  big.length_x = uint32_t(m_dims[0]);  // Truncate 64-bit int to 32-bit, but should be OK.
  big.length_y = uint32_t(m_dims[1]);  // Truncate 64-bit int to 32-bit, but should be OK.
  big.length_z = uint32_t(m_dims[2]);  // Truncate 64-bit int to 32-bit, but should be OK.

  const auto num_of_xforms_xy = sperr::num_of_xforms(std::min(m_dims[0], m_dims[1]));
  const auto num_of_xforms_z = sperr::num_of_xforms(m_dims[2]);
  size_t xf = 0;

  while (xf < num_of_xforms_xy && xf < num_of_xforms_z) {
    auto subsets = m_partition_S_XYZ(big);
    big = subsets[0];  // Reference `m_partition_S_XYZ()` for subset ordering
    // Iterate the rest subsets.
    for (size_t i = 1; i < subsets.size(); i++) {
      const auto parts = subsets[i].part_level;
      m_LIS[parts].emplace_back(subsets[i]);
    }
    xf++;
  }

  // One of these two conditions could happen if num_of_xforms_xy != num_of_xforms_z
  if (xf < num_of_xforms_xy) {
    while (xf < num_of_xforms_xy) {
      auto subsets = m_partition_S_XY(big);
      big = subsets[0];
      // Iterate the rest subsets.
      std::for_each(subsets.cbegin(), subsets.cend(),
                    [&](auto const& s) { return m_LIS[s.part_level].emplace_back(s); });
      xf++;
    }
  }
  else if (xf < num_of_xforms_z) {
    while (xf < num_of_xforms_z) {
      auto subsets = m_partition_S_Z(big);
      big = subsets[0];
      const auto parts = subsets[1].part_level;
      m_LIS[parts].emplace_back(subsets[1]);
      xf++;
    }
  }

  // Right now big is the set that's most likely to be significant, so insert
  // it at the front of it's corresponding vector. One-time expense.
  const auto parts = big.part_level;
  m_LIS[parts].insert(m_LIS[parts].begin(), big);

  m_LIP.clear();

#ifndef QZ_TERM
  // Note that `m_LSP_old` usually grow close to the full length, so we reserve space now.
  m_LSP_new.clear();
  m_LSP_old.clear();
  m_LSP_old.reserve(m_dims[0] * m_dims[1] * m_dims[2]);
  m_bit_buffer.reserve(m_budget);
#endif
}

auto sperr::SPECK3D::m_sorting_pass_encode() -> RTNType
{
#ifndef QZ_TERM
  // Note that a large portion of the content in `m_LIP` will go to `m_LSP_new`,
  //   and `m_LSP_new` is empty at this point, so cheapest to re-allocate right now!
  if (m_LSP_new.capacity() < m_LIP.size())
    m_LSP_new.reserve(std::max(m_LSP_new.capacity() * 2, m_LIP.size()));
#endif

  // Since we have a separate representation of LIP, let's process that list first!
  for (auto& pixel_idx : m_LIP) {
    if (m_coeff_buf[pixel_idx] >= m_threshold_arr[m_threshold_idx]) {
      // Record that this pixel is significant
      m_bit_buffer.push_back(true);
#ifndef QZ_TERM
      if (m_bit_buffer.size() >= m_budget)  // Test bit budget
        return RTNType::BitBudgetMet;
#endif

      // Record if this pixel is positive or negative
      m_bit_buffer.push_back(m_sign_array[pixel_idx]);
#ifndef QZ_TERM
      if (m_bit_buffer.size() >= m_budget)  // Test bit budget
        return RTNType::BitBudgetMet;
#endif

#ifdef QZ_TERM
      m_quantize_P_encode(pixel_idx);
#else
      // Refine this pixel while it's still in the cache!
      m_coeff_buf[pixel_idx] -= m_threshold_arr[m_threshold_idx];
      m_LSP_new.push_back(pixel_idx);
#endif
      pixel_idx = m_u64_garbage_val;
    }
    else {
      // Record that this pixel isn't significant
      m_bit_buffer.push_back(false);
#ifndef QZ_TERM
      if (m_bit_buffer.size() >= m_budget)  // Test bit budget
        return RTNType::BitBudgetMet;
#endif
    }
  }

  // Then we process regular sets in LIS.
  //
  for (size_t tmp = 1; tmp <= m_LIS.size(); tmp++) {
    // From the end of m_LIS to its front
    size_t idx1 = m_LIS.size() - tmp;
    for (size_t idx2 = 0; idx2 < m_LIS[idx1].size(); idx2++) {
      size_t dummy = 0;

#ifdef QZ_TERM
      m_process_S_encode(idx1, idx2, SigType::Dunno, dummy, true);
#else
      auto rtn = m_process_S_encode(idx1, idx2, SigType::Dunno, dummy, true);
      if (rtn == RTNType::BitBudgetMet)
        return rtn;
      assert(rtn == RTNType::Good);
#endif
    }
  }

  return RTNType::Good;
}

auto sperr::SPECK3D::m_sorting_pass_decode() -> RTNType
{
#ifndef QZ_TERM
  // Note that a large portion of the content in `m_LIP` will go to `m_LSP_new`,
  //   and `m_LSP_new` is empty at this point, so cheapest to re-allocate right now.
  if (m_LSP_new.capacity() < m_LIP.size())
    m_LSP_new.reserve(std::max(m_LSP_new.capacity() * 2, m_LIP.size()));
#endif

  // Since we have a separate representation of LIP, let's process that list first
  for (auto& pixel_idx : m_LIP) {
#ifndef QZ_TERM
    if (m_bit_idx >= m_budget)  // Check bit budget
      return RTNType::BitBudgetMet;
#endif
    if (m_bit_buffer[m_bit_idx++]) {  // Is this pixel significant?

#ifndef QZ_TERM
      if (m_bit_idx >= m_budget)  // Check bit budget
        return RTNType::BitBudgetMet;
#endif
      m_sign_array[pixel_idx] = m_bit_buffer[m_bit_idx++];  // What sign is this pixel

#ifdef QZ_TERM
      m_quantize_P_decode(pixel_idx);
#else
      m_LSP_new.push_back(pixel_idx);
#endif
      pixel_idx = m_u64_garbage_val;
    }
  }

  // Then we process regular sets in LIS.
  for (size_t tmp = 1; tmp <= m_LIS.size(); tmp++) {
    // From the end of m_LIS to its front
    size_t idx1 = m_LIS.size() - tmp;
    for (size_t idx2 = 0; idx2 < m_LIS[idx1].size(); idx2++) {
      size_t dummy = 0;

#ifdef QZ_TERM
      m_process_S_decode(idx1, idx2, dummy, true);
#else
      auto rtn = m_process_S_decode(idx1, idx2, dummy, true);
      if (rtn == RTNType::BitBudgetMet)
        return rtn;
      assert(rtn == RTNType::Good);
#endif
    }
  }

  return RTNType::Good;
}

auto sperr::SPECK3D::m_process_P_encode(size_t loc, SigType sig, size_t& counter, bool output)
    -> RTNType
{
  auto& pixel_idx = m_LIP[loc];

  // Decide the significance of this pixel
  assert(sig != SigType::NewlySig);
  bool is_sig = false;
  if (sig == SigType::Dunno) {
    is_sig = (m_coeff_buf[pixel_idx] >= m_threshold_arr[m_threshold_idx]);
  }
  else {
    is_sig = (sig == SigType::Sig);
  }

  if (output) {
    // Output significance bit
    m_bit_buffer.push_back(is_sig);
#ifndef QZ_TERM
    if (m_bit_buffer.size() >= m_budget)  // Test bit budget
      return RTNType::BitBudgetMet;
#endif
  }

  if (is_sig) {
    // Let's increment the counter first!
    counter++;
    // Output pixel sign
    m_bit_buffer.push_back(m_sign_array[pixel_idx]);
#ifndef QZ_TERM
    if (m_bit_buffer.size() >= m_budget)  // Test bit budget
      return RTNType::BitBudgetMet;
#endif

#ifdef QZ_TERM
    m_quantize_P_encode(pixel_idx);
#else
    // refine this pixel while it's still in the cache
    m_coeff_buf[pixel_idx] -= m_threshold_arr[m_threshold_idx];
    m_LSP_new.push_back(pixel_idx);
#endif
    pixel_idx = m_u64_garbage_val;
  }

  return RTNType::Good;
}

#ifdef QZ_TERM
void sperr::SPECK3D::m_quantize_P_encode(size_t idx)
{
  // Since only identified significant pixels come here, it's immediately
  // subject to a QZ operation based on the current threshold.
  auto coeff = m_coeff_buf[idx] - m_threshold_arr[m_threshold_idx];

  auto tmpd = arrd2_type{0.0, 0.0};
  const auto tmpb = arrb2_type{false, true};
  const auto num_qz_levs = m_max_coeff_bits - m_qz_term_lev + 1;
  for (auto i = m_threshold_idx + 1; i < num_qz_levs; i++) {
    tmpd[1] = m_threshold_arr[i];
    size_t o1 = coeff >= m_threshold_arr[i];  // C++ guarantees this conversion
    coeff -= tmpd[o1];
    m_bit_buffer.push_back(tmpb[o1]);
  }
  m_coeff_buf[idx] = coeff;
}
void sperr::SPECK3D::m_quantize_P_decode(size_t idx)
{
  // Since only identified significant pixels come here, it's immediately
  // subject to a QZ operation based on the current threshold.
  auto coeff = m_threshold_arr[m_threshold_idx] * 1.5;

  const auto num_qz_levs = m_max_coeff_bits - m_qz_term_lev + 1;
  for (auto i = m_threshold_idx + 1; i < num_qz_levs; i++) {
    // C++ standard guarantees the conversion between bool and int.
    auto tmp = arrd2_type{-m_threshold_arr[i + 1], m_threshold_arr[i + 1]};
    coeff += tmp[m_bit_buffer[m_bit_idx++]];
  }
  m_coeff_buf[idx] = coeff;
}
//
// Finish QZ_TERM specific functions
//
#else
//
// Start fixed-size specific functions
//
auto sperr::SPECK3D::m_refinement_pass_encode() -> RTNType
{
  // First, process `m_LSP_old`.
  //
  // In fixed-size mode, we either process all elements in `m_LSP_old`,
  // or process a portion of them that meets the total bit budget.
  //
  const auto tmpb = arrb2_type{false, true};
  const auto tmpd = arrd2_type{0.0, -m_threshold_arr[m_threshold_idx]};
  const size_t n_to_process = std::min(m_LSP_old.size(), m_budget - m_bit_buffer.size());
  for (size_t i = 0; i < n_to_process; i++) {
    const size_t loc = m_LSP_old[i];
    const size_t o1 = m_coeff_buf[loc] >= m_threshold_arr[m_threshold_idx];
    m_coeff_buf[loc] += tmpd[o1];
    m_bit_buffer.push_back(tmpb[o1]);
  }
  if (m_bit_buffer.size() >= m_budget)
    return RTNType::BitBudgetMet;

  // Second, attached `m_LSP_new` to the end of `m_LSP_old`, and then clear it.
  // (`m_LSP_old` has reserved the full coeff length capacity in advance.)
  //
  m_LSP_old.insert(m_LSP_old.end(), m_LSP_new.cbegin(), m_LSP_new.cend());
  m_LSP_new.clear();

  return RTNType::Good;
}
auto sperr::SPECK3D::m_refinement_pass_decode() -> RTNType
{
  // First, process `m_LSP_old`
  //
  const size_t num_bits = std::min(m_budget - m_bit_idx, m_LSP_old.size());
  const double half_T = m_threshold_arr[m_threshold_idx] * 0.5;
  const double neg_half_T = m_threshold_arr[m_threshold_idx] * -0.5;
  const double one_half_T = m_threshold_arr[m_threshold_idx] * 1.5;

  const auto tmp = arrd2_type{neg_half_T, half_T};
  for (size_t i = 0; i < num_bits; i++)
    m_coeff_buf[m_LSP_old[i]] += tmp[m_bit_buffer[m_bit_idx + i]];
  m_bit_idx += num_bits;
  if (m_bit_idx >= m_budget)
    return RTNType::BitBudgetMet;

  // Second, process `m_LSP_new`
  //
  for (auto idx : m_LSP_new)
    m_coeff_buf[idx] = one_half_T;

  // Third, attached `m_LSP_new` to the end of `m_LSP_old`.
  // (`m_LSP_old` has reserved the full coeff length capacity in advance.)
  //
  m_LSP_old.insert(m_LSP_old.end(), m_LSP_new.cbegin(), m_LSP_new.cend());

  // Fourth, clear `m_LSP_new`.
  //
  m_LSP_new.clear();

  return RTNType::Good;
}
#endif

auto sperr::SPECK3D::m_decide_significance(const SPECKSet3D& set) const
    -> std::pair<SigType, std::array<uint32_t, 3>>
{
  assert(!set.is_empty());

  const size_t slice_size = m_dims[0] * m_dims[1];

  const auto gtr = [thld = m_threshold_arr[m_threshold_idx]](auto v) { return v >= thld; };

  for (auto z = set.start_z; z < (set.start_z + set.length_z); z++) {
    const size_t slice_offset = z * slice_size;
    for (auto y = set.start_y; y < (set.start_y + set.length_y); y++) {
      auto first = m_coeff_buf.begin() + (slice_offset + y * m_dims[0] + set.start_x);
      auto last = first + set.length_x;
      auto found = std::find_if(first, last, gtr);
      if (found != last) {
        auto xyz = std::array<uint32_t, 3>();
        xyz[0] = std::distance(first, found);
        xyz[1] = y - set.start_y;
        xyz[2] = z - set.start_z;
        return {SigType::Sig, xyz};
      }
    }
  }

  return {SigType::Insig, {0, 0, 0}};
}

auto sperr::SPECK3D::m_process_S_encode(size_t idx1,
                                        size_t idx2,
                                        SigType sig,
                                        size_t& counter,
                                        bool output) -> RTNType
{
  // Significance type cannot be NewlySig!
  assert(sig != SigType::NewlySig);

  auto& set = m_LIS[idx1][idx2];

  // Strategy to decide the significance of this set;
  // 1) If sig == dunno, then find the significance of this set. We do it in a
  //    way that some of its 8 subsets' significance become known as well.
  // 2) If sig is significant, then we directly proceed to `m_code_s()`, but its
  //    subsets' significance is unknown.
  // 3) if sig is insignificant, then this set is not processed.

  std::array<SigType, 8> subset_sigs;
  subset_sigs.fill(SigType::Dunno);

  if (sig == SigType::Dunno) {
    auto set_sig = m_decide_significance(set);
    set.signif = set_sig.first;
    if (set.signif == SigType::Sig) {
      // Try to deduce the significance of some of its subsets.
      // Step 1: which one of the 8 subsets makes it significant?
      //         (Refer to m_partition_S_XYZ() for subset ordering.)
      auto xyz = set_sig.second;
      size_t sub_i = 0;
      sub_i += (xyz[0] < (set.length_x - set.length_x / 2)) ? 0 : 1;
      sub_i += (xyz[1] < (set.length_y - set.length_y / 2)) ? 0 : 2;
      sub_i += (xyz[2] < (set.length_z - set.length_z / 2)) ? 0 : 4;
      subset_sigs[sub_i] = SigType::Sig;

      // Step 2: if it's the 5th, 6th, 7th, or 8th subset significant, then
      //         the first four subsets must be insignificant. Again, this is
      //         based on the ordering of subsets.
      // In a cube there is 30% - 40% chance this condition meets.
      if (sub_i >= 4) {
        for (size_t i = 0; i < 4; i++)
          subset_sigs[i] = SigType::Insig;
      }
    }
  }
  else {
    set.signif = sig;
  }
  bool is_sig = (set.signif == SigType::Sig);
  if (output) {
    m_bit_buffer.push_back(is_sig);
#ifndef QZ_TERM
    if (m_bit_buffer.size() >= m_budget)
      return RTNType::BitBudgetMet;
#endif
  }

  if (is_sig) {
    // Let's increment the counter first!
    counter++;

#ifdef QZ_TERM
    m_code_S_encode(idx1, idx2, subset_sigs);
#else
    auto rtn = m_code_S_encode(idx1, idx2, subset_sigs);
    if (rtn == RTNType::BitBudgetMet)
      return rtn;
    assert(rtn == RTNType::Good);
#endif
    set.type = SetType::Garbage;  // this current set is gonna be discarded.
  }

  return RTNType::Good;
}

auto sperr::SPECK3D::m_process_P_decode(size_t loc, size_t& counter, bool read) -> RTNType
{
  bool is_sig;
  if (read) {
#ifndef QZ_TERM
    if (m_bit_idx >= m_budget)  // Check bit budget
      return RTNType::BitBudgetMet;
#endif
    is_sig = m_bit_buffer[m_bit_idx++];
  }
  else {
    is_sig = true;
  }

  if (is_sig) {
    // Let's increment the counter first!
    counter++;
    auto& pixel_idx = m_LIP[loc];

#ifndef QZ_TERM
    if (m_bit_idx >= m_budget)  // Check bit budget
      return RTNType::BitBudgetMet;
#endif
    m_sign_array[pixel_idx] = m_bit_buffer[m_bit_idx++];

#ifdef QZ_TERM
    m_quantize_P_decode(pixel_idx);
#else
    m_LSP_new.push_back(pixel_idx);
#endif
    pixel_idx = m_u64_garbage_val;
  }

  return RTNType::Good;
}

auto sperr::SPECK3D::m_process_S_decode(size_t idx1, size_t idx2, size_t& counter, bool read)
    -> RTNType
{
  auto& set = m_LIS[idx1][idx2];

  if (read) {
#ifndef QZ_TERM
    if (m_bit_idx >= m_budget)
      return RTNType::BitBudgetMet;
#endif
    set.signif = m_bit_buffer[m_bit_idx++] ? SigType::Sig : SigType::Insig;
  }
  else {
    set.signif = SigType::Sig;
  }

  if (set.signif == SigType::Sig) {
    // Let's increment the counter first!
    counter++;

#ifdef QZ_TERM
    m_code_S_decode(idx1, idx2);
#else
    auto rtn = m_code_S_decode(idx1, idx2);
    if (rtn == RTNType::BitBudgetMet)
      return rtn;
    assert(rtn == RTNType::Good);
#endif

    set.type = SetType::Garbage;  // this current set is gonna be discarded.
  }

  return RTNType::Good;
}

auto sperr::SPECK3D::m_code_S_encode(size_t idx1, size_t idx2, std::array<SigType, 8> subset_sigs)
    -> RTNType
{
  auto subsets = m_partition_S_XYZ(m_LIS[idx1][idx2]);

  // Since some subsets could be empty, let's put empty sets at the end.
  for (size_t i = 0; i < 8; i++) {
    if (subsets[i].is_empty())
      subset_sigs[i] = SigType::Garbage;  // SigType::Garbage is only used locally here.
  }
  // Also need to organize `subset_sigs` to maintain the relative order with subsets.
  std::remove(subset_sigs.begin(), subset_sigs.end(), SigType::Garbage);

  const auto set_end =
      std::remove_if(subsets.begin(), subsets.end(), [](const auto& s) { return s.is_empty(); });
  const auto set_end_m1 = set_end - 1;

  auto sig_it = subset_sigs.begin();
  size_t sig_counter = 0;
  for (auto it = subsets.begin(); it != set_end; ++it) {
    // If we're looking at the last subset, and no prior subset is found to be
    // significant, then we know that this last one *is* significant.
    bool output = true;
    if (it == set_end_m1 && sig_counter == 0) {
      output = false;
      *sig_it = SigType::Sig;
    }

    if (it->is_pixel()) {
      m_LIP.push_back(it->start_z * m_dims[0] * m_dims[1] + it->start_y * m_dims[0] + it->start_x);

#ifdef QZ_TERM
      m_process_P_encode(m_LIP.size() - 1, *sig_it, sig_counter, output);
#else
      auto rtn = m_process_P_encode(m_LIP.size() - 1, *sig_it, sig_counter, output);
      if (rtn == RTNType::BitBudgetMet)
        return rtn;
      assert(rtn == RTNType::Good);
#endif
    }
    else {
      const auto newidx1 = it->part_level;
      m_LIS[newidx1].emplace_back(*it);
      const auto newidx2 = m_LIS[newidx1].size() - 1;

#ifdef QZ_TERM
      m_process_S_encode(newidx1, newidx2, *sig_it, sig_counter, output);
#else
      auto rtn = m_process_S_encode(newidx1, newidx2, *sig_it, sig_counter, output);
      if (rtn == RTNType::BitBudgetMet)
        return rtn;
      assert(rtn == RTNType::Good);
#endif
    }
    ++sig_it;
  }

  return RTNType::Good;
}

auto sperr::SPECK3D::m_code_S_decode(size_t idx1, size_t idx2) -> RTNType
{
  auto subsets = m_partition_S_XYZ(m_LIS[idx1][idx2]);
  const auto set_end =
      std::remove_if(subsets.begin(), subsets.end(), [](const auto& s) { return s.is_empty(); });
  const auto set_end_m1 = set_end - 1;
  size_t sig_counter = 0;

  for (auto it = subsets.begin(); it != set_end; ++it) {
    // If we're looking at the last subset, and no prior subset is found to be
    // significant, then we know that this last one *is* significant.
    bool read = true;
    if (it == set_end_m1 && sig_counter == 0) {
      read = false;
    }
    if (it->is_pixel()) {
      m_LIP.push_back(it->start_z * m_dims[0] * m_dims[1] + it->start_y * m_dims[0] + it->start_x);

#ifdef QZ_TERM
      m_process_P_decode(m_LIP.size() - 1, sig_counter, read);
#else
      auto rtn = m_process_P_decode(m_LIP.size() - 1, sig_counter, read);
      if (rtn == RTNType::BitBudgetMet)
        return rtn;
      assert(rtn == RTNType::Good);
#endif
    }
    else {
      const auto newidx1 = it->part_level;
      m_LIS[newidx1].emplace_back(*it);
      const auto newidx2 = m_LIS[newidx1].size() - 1;

#ifdef QZ_TERM
      m_process_S_decode(newidx1, newidx2, sig_counter, read);
#else
      auto rtn = m_process_S_decode(newidx1, newidx2, sig_counter, read);
      if (rtn == RTNType::BitBudgetMet)
        return rtn;
      assert(rtn == RTNType::Good);
#endif
    }
  }

  return RTNType::Good;
}

auto sperr::SPECK3D::m_ready_to_encode() const -> bool
{
  if (m_coeff_buf.empty())
    return false;
  if (std::any_of(m_dims.begin(), m_dims.end(), [](auto v) { return v == 0; }))
    return false;

#ifndef QZ_TERM
  if (m_budget == 0 || m_budget > m_dims[0] * m_dims[1] * m_dims[2] * 64)
    return false;
#endif

  return true;
}

auto sperr::SPECK3D::m_ready_to_decode() const -> bool
{
  if (m_bit_buffer.empty())
    return false;
  if (std::any_of(m_dims.begin(), m_dims.end(), [](auto v) { return v == 0; }))
    return false;

  return true;
}

auto sperr::SPECK3D::m_partition_S_XYZ(const SPECKSet3D& set) const -> std::array<SPECKSet3D, 8>
{
  using u2_type = std::array<uint32_t, 2>;

  const auto split_x = u2_type{set.length_x - set.length_x / 2, set.length_x / 2};
  const auto split_y = u2_type{set.length_y - set.length_y / 2, set.length_y / 2};
  const auto split_z = u2_type{set.length_z - set.length_z / 2, set.length_z / 2};

  auto next_part_lev = set.part_level;
  next_part_lev += split_x[1] > 0 ? 1 : 0;
  next_part_lev += split_y[1] > 0 ? 1 : 0;
  next_part_lev += split_z[1] > 0 ? 1 : 0;

  std::array<SPECKSet3D, 8> subsets;

#pragma GCC unroll 8
  for (auto& s : subsets)
    s.part_level = next_part_lev;

  constexpr auto offsets = std::array<size_t, 3>{1, 2, 4};

  //
  // The actual figuring out where it starts/ends part...
  //
  // subset (0, 0, 0)
  constexpr auto idx0 = 0 * offsets[0] + 0 * offsets[1] + 0 * offsets[2];
  auto& sub0 = subsets[idx0];
  sub0.start_x = set.start_x;
  sub0.length_x = split_x[0];
  sub0.start_y = set.start_y;
  sub0.length_y = split_y[0];
  sub0.start_z = set.start_z;
  sub0.length_z = split_z[0];

  // subset (1, 0, 0)
  constexpr auto idx1 = 1 * offsets[0] + 0 * offsets[1] + 0 * offsets[2];
  auto& sub1 = subsets[idx1];
  sub1.start_x = set.start_x + split_x[0];
  sub1.length_x = split_x[1];
  sub1.start_y = set.start_y;
  sub1.length_y = split_y[0];
  sub1.start_z = set.start_z;
  sub1.length_z = split_z[0];

  // subset (0, 1, 0)
  constexpr auto idx2 = 0 * offsets[0] + 1 * offsets[1] + 0 * offsets[2];
  auto& sub2 = subsets[idx2];
  sub2.start_x = set.start_x;
  sub2.length_x = split_x[0];
  sub2.start_y = set.start_y + split_y[0];
  sub2.length_y = split_y[1];
  sub2.start_z = set.start_z;
  sub2.length_z = split_z[0];

  // subset (1, 1, 0)
  constexpr auto idx3 = 1 * offsets[0] + 1 * offsets[1] + 0 * offsets[2];
  auto& sub3 = subsets[idx3];
  sub3.start_x = set.start_x + split_x[0];
  sub3.length_x = split_x[1];
  sub3.start_y = set.start_y + split_y[0];
  sub3.length_y = split_y[1];
  sub3.start_z = set.start_z;
  sub3.length_z = split_z[0];

  // subset (0, 0, 1)
  constexpr auto idx4 = 0 * offsets[0] + 0 * offsets[1] + 1 * offsets[2];
  auto& sub4 = subsets[idx4];
  sub4.start_x = set.start_x;
  sub4.length_x = split_x[0];
  sub4.start_y = set.start_y;
  sub4.length_y = split_y[0];
  sub4.start_z = set.start_z + split_z[0];
  sub4.length_z = split_z[1];

  // subset (1, 0, 1)
  constexpr auto idx5 = 1 * offsets[0] + 0 * offsets[1] + 1 * offsets[2];
  auto& sub5 = subsets[idx5];
  sub5.start_x = set.start_x + split_x[0];
  sub5.length_x = split_x[1];
  sub5.start_y = set.start_y;
  sub5.length_y = split_y[0];
  sub5.start_z = set.start_z + split_z[0];
  sub5.length_z = split_z[1];

  // subset (0, 1, 1)
  constexpr auto idx6 = 0 * offsets[0] + 1 * offsets[1] + 1 * offsets[2];
  auto& sub6 = subsets[idx6];
  sub6.start_x = set.start_x;
  sub6.length_x = split_x[0];
  sub6.start_y = set.start_y + split_y[0];
  sub6.length_y = split_y[1];
  sub6.start_z = set.start_z + split_z[0];
  sub6.length_z = split_z[1];

  // subset (1, 1, 1)
  constexpr auto idx7 = 1 * offsets[0] + 1 * offsets[1] + 1 * offsets[2];
  auto& sub7 = subsets[idx7];
  sub7.start_x = set.start_x + split_x[0];
  sub7.length_x = split_x[1];
  sub7.start_y = set.start_y + split_y[0];
  sub7.length_y = split_y[1];
  sub7.start_z = set.start_z + split_z[0];
  sub7.length_z = split_z[1];

  // I tested this function and named return value optimization really works!
  // (There won't be a copy on this variable after the return.)
  return subsets;
}

auto sperr::SPECK3D::m_partition_S_XY(const SPECKSet3D& set) const -> std::array<SPECKSet3D, 4>
{
  std::array<SPECKSet3D, 4> subsets;

  using u2_type = std::array<uint32_t, 2>;
  const auto split_x = u2_type{set.length_x - set.length_x / 2, set.length_x / 2};
  const auto split_y = u2_type{set.length_y - set.length_y / 2, set.length_y / 2};

  for (auto& s : subsets) {
    s.part_level = set.part_level;
    if (split_x[1] > 0)
      s.part_level++;
    if (split_y[1] > 0)
      s.part_level++;
  }

  //
  // The actual figuring out where it starts/ends part...
  //
  const auto offsets = std::array<size_t, 3>{1, 2, 4};
  // subset (0, 0, 0)
  size_t sub_i = 0 * offsets[0] + 0 * offsets[1] + 0 * offsets[2];
  auto& sub0 = subsets[sub_i];
  sub0.start_x = set.start_x;
  sub0.length_x = split_x[0];
  sub0.start_y = set.start_y;
  sub0.length_y = split_y[0];
  sub0.start_z = set.start_z;
  sub0.length_z = set.length_z;

  // subset (1, 0, 0)
  sub_i = 1 * offsets[0] + 0 * offsets[1] + 0 * offsets[2];
  auto& sub1 = subsets[sub_i];
  sub1.start_x = set.start_x + split_x[0];
  sub1.length_x = split_x[1];
  sub1.start_y = set.start_y;
  sub1.length_y = split_y[0];
  sub1.start_z = set.start_z;
  sub1.length_z = set.length_z;

  // subset (0, 1, 0)
  sub_i = 0 * offsets[0] + 1 * offsets[1] + 0 * offsets[2];
  auto& sub2 = subsets[sub_i];
  sub2.start_x = set.start_x;
  sub2.length_x = split_x[0];
  sub2.start_y = set.start_y + split_y[0];
  sub2.length_y = split_y[1];
  sub2.start_z = set.start_z;
  sub2.length_z = set.length_z;

  // subset (1, 1, 0)
  sub_i = 1 * offsets[0] + 1 * offsets[1] + 0 * offsets[2];
  auto& sub3 = subsets[sub_i];
  sub3.start_x = set.start_x + split_x[0];
  sub3.length_x = split_x[1];
  sub3.start_y = set.start_y + split_y[0];
  sub3.length_y = split_y[1];
  sub3.start_z = set.start_z;
  sub3.length_z = set.length_z;

  return subsets;
}

auto sperr::SPECK3D::m_partition_S_Z(const SPECKSet3D& set) const -> std::array<SPECKSet3D, 2>
{
  std::array<SPECKSet3D, 2> subsets;

  const auto split_z = std::array<uint32_t, 2>{set.length_z - set.length_z / 2, set.length_z / 2};

  for (auto& s : subsets) {
    s.part_level = set.part_level;
    if (split_z[1] > 0)
      s.part_level++;
  }

  //
  // The actual figuring out where it starts/ends part...
  //
  // subset (0, 0, 0)
  auto& sub0 = subsets[0];
  sub0.start_x = set.start_x;
  sub0.length_x = set.length_x;
  sub0.start_y = set.start_y;
  sub0.length_y = set.length_y;
  sub0.start_z = set.start_z;
  sub0.length_z = split_z[0];

  // subset (0, 0, 1)
  auto& sub1 = subsets[1];
  sub1.start_x = set.start_x;
  sub1.length_x = set.length_x;
  sub1.start_y = set.start_y;
  sub1.length_y = set.length_y;
  sub1.start_z = set.start_z + split_z[0];
  sub1.length_z = split_z[1];

  return subsets;
}
