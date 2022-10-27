#include "SPECK3D_INT_ENC.h"

#include <algorithm>
#include <cassert>
#include <numeric>

auto sperr::SPECK3D_INT_ENC::take_input(veci_t&& coeffs, vecb_type&& signs, dims_type dims)
    -> RTNType
{
  const auto total_vals = dims[0] * dims[1] * dims[2];
  if (coeffs.size() != total_vals || signs.size() != total_vals)
    return RTNType::WrongDims;

  m_coeff_buf = std::move(coeffs);
  m_sign_array = std::move(signs);
  m_dims = dims;
  return RTNType::Good;
}

void sperr::SPECK3D_INT_ENC::m_clean_LIS()
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

void sperr::SPECK3D_INT_ENC::m_initialize_sets_lists()
{
  std::array<size_t, 3> num_of_parts;  // how many times each dimension could be partitioned?
  num_of_parts[0] = sperr::num_of_partitions(m_dims[0]);
  num_of_parts[1] = sperr::num_of_partitions(m_dims[1]);
  num_of_parts[2] = sperr::num_of_partitions(m_dims[2]);
  size_t num_of_sizes = std::accumulate(num_of_parts.cbegin(), num_of_parts.cend(), 1ul);

  // Initialize LIS
  if (m_LIS.size() < num_of_sizes)
    m_LIS.resize(num_of_sizes);
  std::for_each(m_LIS.begin(), m_LIS.end(), [](auto& list) { list.clear(); });

  // Starting from a set representing the whole volume, identify the smaller
  // sets and put them in LIS accordingly.
  Set3D big;
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
    auto subsets = sperr::partition_S_XYZ(big);
    big = subsets[0];  // Reference `partition_S_XYZ()` for subset ordering
    // Also put the rest subsets in appropriate positions in `m_LIS`.
    std::for_each(std::next(subsets.cbegin()), subsets.cend(),
                  [&](const auto& s) { m_LIS[s.part_level].emplace_back(s); });
    xf++;
  }

  // One of these two conditions could happen if num_of_xforms_xy != num_of_xforms_z
  if (xf < num_of_xforms_xy) {
    while (xf < num_of_xforms_xy) {
      auto subsets = sperr::partition_S_XY(big);
      big = subsets[0];
      // Also put the rest subsets in appropriate positions in `m_LIS`.
      std::for_each(std::next(subsets.cbegin()), subsets.cend(),
                    [&](const auto& s) { m_LIS[s.part_level].emplace_back(s); });
      xf++;
    }
  }
  else if (xf < num_of_xforms_z) {
    while (xf < num_of_xforms_z) {
      auto subsets = sperr::partition_S_Z(big);
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
  m_bit_buffer.reserve(m_coeff_buf.size());  // a reasonable starting point
}

auto sperr::SPECK3D_INT_ENC::m_decide_significance(const Set3D& set) const
    -> std::pair<SigType, std::array<uint32_t, 3>>
{
  assert(!set.is_empty());

  const size_t slice_size = m_dims[0] * m_dims[1];

  const auto gtr = [thld = m_threshold](auto v) { return v >= thld; };

  for (auto z = set.start_z; z < (set.start_z + set.length_z); z++) {
    const size_t slice_offset = z * slice_size;
    for (auto y = set.start_y; y < (set.start_y + set.length_y); y++) {
      auto first = m_coeff_buf.cbegin() + (slice_offset + y * m_dims[0] + set.start_x);
      auto last = first + set.length_x;
      auto found = std::find_if(first, last, gtr);
      if (found != last) {
        const auto x = static_cast<uint32_t>(std::distance(first, found));
        return {SigType::Sig, {x, y - set.start_y, z - set.start_z}};
      }
    };
  }

  return {SigType::Insig, {0u, 0u, 0u}};
}

void sperr::SPECK3D_INT_ENC::encode()
{
  // if (m_ready_to_encode() == false)
  //   return RTNType::Error;

  // Cache the current compression mode
  // m_mode_cache = sperr::compression_mode(m_encode_budget, m_target_psnr, m_target_pwe);
  // if (m_mode_cache == sperr::CompMode::Unknown)
  //  return RTNType::CompModeUnknown;

  // Sanity check
  // if (m_mode_cache == CompMode::FixedPSNR) {
  //  if (m_data_range == sperr::max_d)
  //    return RTNType::DataRangeNotSet;
  //}
  // else
  //  m_data_range = sperr::max_d;

  m_initialize_sets_lists();

  // m_encoded_stream.clear();
  m_bit_buffer.clear();

  // Keep signs of all coefficients
  // m_sign_array.resize(m_coeff_buf.size());
  // std::transform(m_coeff_buf.cbegin(), m_coeff_buf.cend(), m_sign_array.begin(),
  //               [](auto e) { return e >= 0.0; });

  // Make every coefficient positive
  // std::transform(m_coeff_buf.cbegin(), m_coeff_buf.cend(), m_coeff_buf.begin(),
  //               [](auto v) { return std::abs(v); });

  // Use `m_qz_coeff` to keep track of would-be quantized coefficients.
  // if (m_mode_cache == CompMode::FixedPWE)
  //  m_qz_coeff.assign(m_coeff_buf.size(), 0.0);
  // else
  //  m_qz_coeff.clear();

  // Mark every coefficient as insignificant
  m_LSP_mask.assign(m_coeff_buf.size(), false);

  // Decide the starting threshold.
  const auto max_coeff = *std::max_element(m_coeff_buf.cbegin(), m_coeff_buf.cend());
  size_t num_bitplanes = 1;
  m_threshold = 1;
  while (m_threshold * int_t{2} <= max_coeff) {
    m_threshold *= int_t{2};
    num_bitplanes++;
  }

  // if (m_mode_cache == CompMode::FixedPWE || m_mode_cache == CompMode::FixedPSNR) {
  //   const auto terminal_threshold = m_estimate_finest_q();
  //   auto max_t = terminal_threshold;
  //   num_bitplanes = 1;
  //   while (max_t * 2.0 < max_coeff) {
  //     max_t *= 2.0;
  //     num_bitplanes++;
  //   }
  //   m_max_threshold_f = static_cast<float>(max_t);
  // }
  // else {  // FixedSize mode
  //   const auto max_coeff_bit = std::floor(std::log2(max_coeff));
  //   m_max_threshold_f = static_cast<float>(std::pow(2.0, max_coeff_bit));
  // }
  // m_threshold = static_cast<double>(m_max_threshold_f);

  for (size_t bitplane = 0; bitplane < num_bitplanes; bitplane++) {
    m_sorting_pass();
    m_refinement_pass();

    m_threshold /= int_t{2};
    m_clean_LIS();
  }

  // If the bit buffer has the last byte half-empty, let's fill in zero's.
  // The decoding process will not read them anyway.
  while (m_bit_buffer.size() % 8 != 0)
    m_bit_buffer.push_back(false);

  // Finally we prepare the bitstream
  // rtn = m_prepare_encoded_bitstream();
}

void sperr::SPECK3D_INT_ENC::m_process_S(size_t idx1,
                                         size_t idx2,
                                         SigType sig,
                                         size_t& counter,
                                         bool output)
{
  // Significance type cannot be NewlySig!
  assert(sig != SigType::NewlySig);

  auto& set = m_LIS[idx1][idx2];

  // Strategy to decide the significance of this set;
  // 1) If sig == dunno, then find the significance of this set. We do it in a
  //    way that some of its 8 subsets' significance become known as well.
  // 2) If sig is significant, then we directly proceed to `m_code_s()`, with its
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
  if (output)
    m_bit_buffer.push_back(is_sig);

  if (is_sig) {
    counter++;  // Let's increment the counter first!
    m_code_S(idx1, idx2, subset_sigs);
    set.type = SetType::Garbage;  // this current set is gonna be discarded.
  }
}

void sperr::SPECK3D_INT_ENC::m_process_P(size_t loc, SigType sig, size_t& counter, bool output)
{
  const auto pixel_idx = m_LIP[loc];

  // Decide the significance of this pixel
  assert(sig != SigType::NewlySig);
  bool is_sig = false;
  if (sig == SigType::Dunno)
    is_sig = (m_coeff_buf[pixel_idx] >= m_threshold);
  else
    is_sig = (sig == SigType::Sig);

  if (output)
    m_bit_buffer.push_back(is_sig);

  if (is_sig) {
    counter++;  // Let's increment the counter first!
    m_bit_buffer.push_back(m_sign_array[pixel_idx]);

    assert(m_coeff_buf[pixel_idx] >= m_threshold);
    m_coeff_buf[pixel_idx] -= m_threshold;
    m_LSP_new.push_back(pixel_idx);
    m_LIP[loc] = m_u64_garbage_val;
  }
}

void sperr::SPECK3D_INT_ENC::m_code_S(size_t idx1, size_t idx2, std::array<SigType, 8> subset_sigs)
{
  auto subsets = partition_S_XYZ(m_LIS[idx1][idx2]);

  // Since some subsets could be empty, let's put empty sets at the end.
  //
  for (size_t i = 0; i < subsets.size(); i++) {
    if (subsets[i].is_empty())
      subset_sigs[i] = SigType::Garbage;  // SigType::Garbage is only used locally here.
  }
  std::remove(subset_sigs.begin(), subset_sigs.end(), SigType::Garbage);
  const auto set_end =
      std::remove_if(subsets.begin(), subsets.end(), [](auto& s) { return s.is_empty(); });
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
      m_process_P(m_LIP.size() - 1, *sig_it, sig_counter, output);
    }
    else {
      const auto newidx1 = it->part_level;
      m_LIS[newidx1].emplace_back(*it);
      const auto newidx2 = m_LIS[newidx1].size() - 1;
      m_process_S(newidx1, newidx2, *sig_it, sig_counter, output);
    }
    ++sig_it;
  }
}

void sperr::SPECK3D_INT_ENC::m_sorting_pass()
{
  // Since we have a separate representation of LIP, let's process that list first!
  //
  size_t dummy = 0;
  for (size_t loc = 0; loc < m_LIP.size(); loc++)
    m_process_P(loc, SigType::Dunno, dummy, true);

  // Then we process regular sets in LIS.
  //
  for (size_t tmp = 1; tmp <= m_LIS.size(); tmp++) {
    // From the end of m_LIS to its front
    size_t idx1 = m_LIS.size() - tmp;
    for (size_t idx2 = 0; idx2 < m_LIS[idx1].size(); idx2++)
      m_process_S(idx1, idx2, SigType::Dunno, dummy, true);
  }
}

void sperr::SPECK3D_INT_ENC::m_refinement_pass()
{
  // First, process significant pixels previously found.
  //
  const auto tmp1 = std::array<int_t, 2>{0, m_threshold};

  for (size_t i = 0; i < m_LSP_mask.size(); i++) {
    if (m_LSP_mask[i]) {
      const bool o1 = m_coeff_buf[i] >= m_threshold;
      m_bit_buffer.push_back(o1);
      m_coeff_buf[i] -= tmp1[o1];
    }
  }

  // Second, mark newly found significant pixels in `m_LSP_mask`.
  //
  for (auto idx : m_LSP_new)
    m_LSP_mask[idx] = true;
  m_LSP_new.clear();
}
