#include "SPECK3D.h"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstring>
#include <numeric>

#include "Kokkos_Core.hpp"
#include "Kokkos_StdAlgorithms.hpp"

using d2_type = std::array<double, 2>;
using u2_type = std::array<uint32_t, 2>;

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

  // Cache the current compression mode
  m_mode_cache = sperr::compression_mode(m_encode_budget, m_target_psnr, m_target_pwe);
  if (m_mode_cache == sperr::CompMode::Unknown)
    return RTNType::CompModeUnknown;

  // Sanity check
  if (m_mode_cache == CompMode::FixedPSNR) {
    if (m_data_range == sperr::max_d)
      return RTNType::DataRangeNotSet;
  }
  else
    m_data_range = sperr::max_d;

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

  // Use `m_qz_coeff` to keep track of would-be quantized coefficients.
  if (m_mode_cache == CompMode::FixedPWE)
    m_qz_coeff.assign(m_coeff_buf.size(), 0.0);
  else
    m_qz_coeff.clear();

  // Mark every coefficient as insignificant
  m_LSP_mask.assign(m_coeff_buf.size(), false);

  // Decide the starting threshold for quantization.
  size_t num_bitplanes = 128;
  const auto max_coeff = *std::max_element(m_coeff_buf.cbegin(), m_coeff_buf.cend());
  if (m_mode_cache == CompMode::FixedPWE || m_mode_cache == CompMode::FixedPSNR) {
    const auto terminal_threshold = m_estimate_finest_q();
    auto max_t = terminal_threshold;
    num_bitplanes = 1;
    while (max_t * 2.0 < max_coeff) {
      max_t *= 2.0;
      num_bitplanes++;
    }
    m_max_threshold = max_t;
    m_threshold = max_t;
  }
  else {  // FixedSize mode
    // When max_coeff is between 0.0 and 1.0, std::log2(max_coeff) will become a
    // negative value. std::floor() will always find the smaller integer value,
    // which will always reconstruct to a bitplane value that is smaller than max_coeff.
    //
    const auto max_coeff_bit = std::floor(std::log2(max_coeff));
    m_max_threshold = std::pow(2.0, max_coeff_bit);
    m_threshold = std::pow(2.0, max_coeff_bit);
  }

  auto rtn = RTNType::Good;
  for (size_t bitplane = 0; bitplane < num_bitplanes; bitplane++) {
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

  // Finally we prepare the bitstream
  rtn = m_prepare_encoded_bitstream();

  return rtn;
}

auto sperr::SPECK3D::decode() -> RTNType
{
  if (m_ready_to_decode() == false)
    return RTNType::Error;

  m_initialize_sets_lists();

  // initialize coefficients to be zero, and sign array to be all positive
  const auto coeff_len = m_dims[0] * m_dims[1] * m_dims[2];
  m_coeff_buf.assign(coeff_len, 0.0);
  m_sign_array.assign(coeff_len, true);

  // Mark every coefficient as insignificant
  m_LSP_mask.assign(m_coeff_buf.size(), false);
  m_LSP_mask_cnt = 0;
  m_bit_idx = 0;
  m_threshold = m_max_threshold;

  for (size_t bitplane = 0; bitplane < 128; bitplane++) {
    auto rtn = m_sorting_pass_decode();
    if (rtn == RTNType::BitBudgetMet) {
      break;
    }
    assert(rtn == RTNType::Good);

    rtn = m_refinement_pass_decode();
    if (rtn == RTNType::BitBudgetMet) {
      break;
    }
    assert(rtn == RTNType::Good);

    m_threshold *= 0.5;
    m_clean_LIS();
  }

  // Restore coefficient signs by setting some of them negative
  const auto tmp = d2_type{-1.0, 1.0};
  for (size_t i = 0; i < m_sign_array.size(); i++)
    m_coeff_buf[i] *= tmp[m_sign_array[i]];

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
  // Note that `m_LIS` is a two-dimensional array. We want to keep the memory allocated in the
  // secondary array, so we don't clear `m_LIS` itself, but clear the every secondary arrays
  // inside of it. Also note that we don't shrink the size of `m_LIS`. This is OK as long as
  // the extra lists at the end are cleared.
  //
  if (m_LIS.size() < num_of_sizes)
    m_LIS.resize(num_of_sizes);
  for (auto& list : m_LIS)
    list.clear();

  // Starting from a set representing the whole volume, identify the smaller
  // sets and put them in LIS accordingly.
  SPECKSet3D big;
  big.length_x =
      static_cast<uint32_t>(m_dims[0]);  // Truncate 64-bit int to 32-bit, but should be OK.
  big.length_y =
      static_cast<uint32_t>(m_dims[1]);  // Truncate 64-bit int to 32-bit, but should be OK.
  big.length_z =
      static_cast<uint32_t>(m_dims[2]);  // Truncate 64-bit int to 32-bit, but should be OK.

  const auto num_of_xforms_xy = sperr::num_of_xforms(std::min(m_dims[0], m_dims[1]));
  const auto num_of_xforms_z = sperr::num_of_xforms(m_dims[2]);
  size_t xf = 0;

  while (xf < num_of_xforms_xy && xf < num_of_xforms_z) {
    auto subsets = m_partition_S_XYZ(big);
    big = subsets[0];  // Reference `m_partition_S_XYZ()` for subset ordering
    // Also put the rest subsets in appropriate positions in `m_LIS`.
    std::for_each(std::next(subsets.cbegin()), subsets.cend(),
                  [&](const auto& s) { m_LIS[s.part_level].emplace_back(s); });
    xf++;
  }

  // One of these two conditions could happen if num_of_xforms_xy != num_of_xforms_z
  if (xf < num_of_xforms_xy) {
    while (xf < num_of_xforms_xy) {
      auto subsets = m_partition_S_XY(big);
      big = subsets[0];
      // Also put the rest subsets in appropriate positions in `m_LIS`.
      std::for_each(std::next(subsets.cbegin()), subsets.cend(),
                    [&](const auto& s) { m_LIS[s.part_level].emplace_back(s); });
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
  m_LIP.reserve(m_coeff_buf.size() / 4);

  m_LSP_new.clear();
  m_LSP_new.reserve(m_coeff_buf.size() / 8);
  m_LSP_mask.reserve(m_coeff_buf.size());
  m_bit_buffer.reserve(m_coeff_buf.size());
}

auto sperr::SPECK3D::m_sorting_pass_encode() -> RTNType
{
  // Since we have a separate representation of LIP, let's process that list first!
  //
  size_t dummy = 0;
  for (size_t loc = 0; loc < m_LIP.size(); loc++) {
    auto rtn = m_process_P_encode(loc, SigType::Dunno, dummy, true);
    if (rtn == RTNType::BitBudgetMet)
      return RTNType::BitBudgetMet;
    assert(rtn == RTNType::Good);
  }

  // Then we process regular sets in LIS.
  //
  for (size_t tmp = 1; tmp <= m_LIS.size(); tmp++) {
    // From the end of m_LIS to its front
    size_t idx1 = m_LIS.size() - tmp;
    for (size_t idx2 = 0; idx2 < m_LIS[idx1].size(); idx2++) {
      auto rtn = m_process_S_encode(idx1, idx2, SigType::Dunno, dummy, true);
      if (rtn == RTNType::BitBudgetMet)
        return rtn;
      assert(rtn == RTNType::Good);
    }
  }

  return RTNType::Good;
}

auto sperr::SPECK3D::m_sorting_pass_decode() -> RTNType
{
  // Since we have a separate representation of LIP, let's process that list first
  //
  size_t dummy = 0;
  for (size_t loc = 0; loc < m_LIP.size(); loc++) {
    auto rtn = m_process_P_decode(loc, dummy, true);
    if (rtn == RTNType::BitBudgetMet)
      return RTNType::BitBudgetMet;
    assert(rtn == RTNType::Good);
  }

  // Then we process regular sets in LIS.
  //
  for (size_t tmp = 1; tmp <= m_LIS.size(); tmp++) {
    // From the end of m_LIS to its front
    size_t idx1 = m_LIS.size() - tmp;
    for (size_t idx2 = 0; idx2 < m_LIS[idx1].size(); idx2++) {
      auto rtn = m_process_S_decode(idx1, idx2, dummy, true);
      if (rtn == RTNType::BitBudgetMet)
        return rtn;
      assert(rtn == RTNType::Good);
    }
  }

  return RTNType::Good;
}

auto sperr::SPECK3D::m_process_P_encode(size_t loc, SigType sig, size_t& counter, bool output)
    -> RTNType
{
  const auto pixel_idx = m_LIP[loc];

  // Decide the significance of this pixel
  assert(sig != SigType::NewlySig);
  bool is_sig = false;
  if (sig == SigType::Dunno)
    is_sig = (m_coeff_buf[pixel_idx] >= m_threshold);
  else
    is_sig = (sig == SigType::Sig);

  if (output) {
    m_bit_buffer.push_back(is_sig);
    if (m_bit_buffer.size() >= m_encode_budget)
      return RTNType::BitBudgetMet;
  }

  if (is_sig) {
    counter++;  // Let's increment the counter first!
    m_bit_buffer.push_back(m_sign_array[pixel_idx]);
    if (m_bit_buffer.size() >= m_encode_budget)
      return RTNType::BitBudgetMet;

    m_coeff_buf[pixel_idx] -= m_threshold;
    m_LSP_new.push_back(pixel_idx);
    m_LIP[loc] = m_u64_garbage_val;

    if (m_mode_cache == CompMode::FixedPWE)
      m_qz_coeff[pixel_idx] = m_threshold * 1.5;
  }

  return RTNType::Good;
}

auto sperr::SPECK3D::m_decide_significance(const SPECKSet3D& set) const
    -> std::pair<SigType, std::array<uint32_t, 3>>
{
  assert(!set.is_empty());

  namespace KE = Kokkos::Experimental;
  using exespace = Kokkos::DefaultExecutionSpace;

  const size_t slice_size = m_dims[0] * m_dims[1];

  const auto gtr = [thld = m_threshold](auto v) { return v >= thld; };

  for (auto z = set.start_z; z < (set.start_z + set.length_z); z++) {
    const size_t slice_offset = z * slice_size;
    for (auto y = set.start_y; y < (set.start_y + set.length_y); y++) {
      auto first = m_coeff_buf.cbegin() + (slice_offset + y * m_dims[0] + set.start_x);
      auto last = first + set.length_x;
      auto found = std::find_if(first, last, gtr);
      //auto found = KE::find_if(exespace(), first, last, gtr);
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
    if (m_bit_buffer.size() >= m_encode_budget)
      return RTNType::BitBudgetMet;
  }

  if (is_sig) {
    counter++;  // Let's increment the counter first!
    auto rtn = m_code_S_encode(idx1, idx2, subset_sigs);
    if (rtn == RTNType::BitBudgetMet)
      return rtn;
    assert(rtn == RTNType::Good);
    set.type = SetType::Garbage;  // this current set is gonna be discarded.
  }

  return RTNType::Good;
}

auto sperr::SPECK3D::m_process_P_decode(size_t loc, size_t& counter, bool read) -> RTNType
{
  bool is_sig = true;
  const auto pixel_idx = m_LIP[loc];

  if (read) {
    if (m_bit_idx >= m_bit_buffer.size())
      return RTNType::BitBudgetMet;
    is_sig = m_bit_buffer[m_bit_idx++];
  }

  if (is_sig) {
    counter++;  // Let's increment the counter first!
    if (m_bit_idx >= m_bit_buffer.size())
      return RTNType::BitBudgetMet;
    m_sign_array[pixel_idx] = m_bit_buffer[m_bit_idx++];

    m_coeff_buf[pixel_idx] = m_threshold * 1.5;
    m_LSP_new.push_back(pixel_idx);

    m_LIP[loc] = m_u64_garbage_val;
  }

  return RTNType::Good;
}

auto sperr::SPECK3D::m_process_S_decode(size_t idx1, size_t idx2, size_t& counter, bool read)
    -> RTNType
{
  auto& set = m_LIS[idx1][idx2];

  if (read) {
    if (m_bit_idx >= m_bit_buffer.size())
      return RTNType::BitBudgetMet;
    set.signif = m_bit_buffer[m_bit_idx++] ? SigType::Sig : SigType::Insig;
  }
  else {
    set.signif = SigType::Sig;
  }

  if (set.signif == SigType::Sig) {
    counter++;  // Let's increment the counter first!
    auto rtn = m_code_S_decode(idx1, idx2);
    if (rtn == RTNType::BitBudgetMet)
      return rtn;
    assert(rtn == RTNType::Good);
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
      auto rtn = m_process_P_encode(m_LIP.size() - 1, *sig_it, sig_counter, output);
      if (rtn == RTNType::BitBudgetMet)
        return rtn;
      assert(rtn == RTNType::Good);
    }
    else {
      const auto newidx1 = it->part_level;
      m_LIS[newidx1].emplace_back(*it);
      const auto newidx2 = m_LIS[newidx1].size() - 1;
      auto rtn = m_process_S_encode(newidx1, newidx2, *sig_it, sig_counter, output);
      if (rtn == RTNType::BitBudgetMet)
        return rtn;
      assert(rtn == RTNType::Good);
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
      auto rtn = m_process_P_decode(m_LIP.size() - 1, sig_counter, read);
      if (rtn == RTNType::BitBudgetMet)
        return rtn;
      assert(rtn == RTNType::Good);
    }
    else {
      const auto newidx1 = it->part_level;
      m_LIS[newidx1].emplace_back(*it);
      const auto newidx2 = m_LIS[newidx1].size() - 1;
      auto rtn = m_process_S_decode(newidx1, newidx2, sig_counter, read);
      if (rtn == RTNType::BitBudgetMet)
        return rtn;
      assert(rtn == RTNType::Good);
    }
  }

  return RTNType::Good;
}

auto sperr::SPECK3D::m_ready_to_encode() const -> bool
{
  if (std::any_of(m_dims.cbegin(), m_dims.cend(), [](auto v) { return v == 0; }))
    return false;
  if (m_coeff_buf.size() != m_dims[0] * m_dims[1] * m_dims[2])
    return false;

  return true;
}

auto sperr::SPECK3D::m_ready_to_decode() const -> bool
{
  if (std::any_of(m_dims.cbegin(), m_dims.cend(), [](auto v) { return v == 0; }))
    return false;
  if (m_bit_buffer.empty())
    return false;

  return true;
}

auto sperr::SPECK3D::m_partition_S_XYZ(const SPECKSet3D& set) const -> std::array<SPECKSet3D, 8>
{
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
