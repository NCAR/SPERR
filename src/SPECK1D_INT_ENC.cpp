#include "SPECK1D_INT_ENC.h"

#include <algorithm>
#include <cassert>
#include <cstring>  // std::memcpy()
#include <numeric>

void sperr::SPECK1D_INT_ENC::encode()
{
  m_bit_buffer.rewind();
  m_total_bits = 0;
  m_initialize_lists();

  // Mark every coefficient as insignificant
  m_LSP_mask.resize(m_coeff_buf.size());
  m_LSP_mask.reset();

  // Decide the starting threshold.
  const auto max_coeff = *std::max_element(m_coeff_buf.cbegin(), m_coeff_buf.cend());
  m_num_bitplanes = 1;
  m_threshold = 1;
  while (m_threshold * uint_t{2} <= max_coeff) {
    m_threshold *= uint_t{2};
    m_num_bitplanes++;
  }

  for (uint8_t bitplane = 0; bitplane < m_num_bitplanes; bitplane++) {
    m_sorting_pass();
    m_refinement_pass_encode();

    m_threshold /= uint_t{2};
    m_clean_LIS();
  }

  // Flush the bitstream, and record the total number of bits
  m_total_bits = m_bit_buffer.wtell();
  m_bit_buffer.flush();
}

void sperr::SPECK1D_INT_ENC::m_sorting_pass()
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

void sperr::SPECK1D_INT_ENC::m_process_S(size_t idx1,
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
  //    way that at least one of its 2 subsets' significance become known as well.
  // 2) If sig is significant, then we directly proceed to `m_code_s()`, with its
  //    subsets' significance is unknown.
  // 3) if sig is insignificant, then this set is not processed.

  auto subset_sigs = std::array<SigType, 2>{SigType::Dunno, SigType::Dunno};

  if (sig == SigType::Dunno) {
    auto set_sig = m_decide_significance(set);
    sig = set_sig.first;
    if (sig == SigType::Sig) {
      assert(set_sig.second >= 0);
      if (set_sig.second < set.length - set.length / 2)
        subset_sigs = {SigType::Sig, SigType::Dunno};
      else
        subset_sigs = {SigType::Insig, SigType::Sig};
    }
  }

  if (output)
    m_bit_buffer.wbit(sig == SigType::Sig);

  if (sig == SigType::Sig) {
    counter++;  // Let's increment the counter first!
    m_code_S(idx1, idx2, subset_sigs);
    set.type = SetType::Garbage;  // this current set is gonna be discarded.
  }
}

void sperr::SPECK1D_INT_ENC::m_process_P(size_t loc, SigType sig, size_t& counter, bool output)
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
    m_bit_buffer.wbit(is_sig);

  if (is_sig) {
    counter++;  // Let's increment the counter first!
    m_bit_buffer.wbit(m_sign_array[pixel_idx]);

    assert(m_coeff_buf[pixel_idx] >= m_threshold);
    m_coeff_buf[pixel_idx] -= m_threshold;
    m_LSP_new.push_back(pixel_idx);
    m_LIP[loc] = m_u64_garbage_val;
  }
}

void sperr::SPECK1D_INT_ENC::m_code_S(size_t idx1, size_t idx2, std::array<SigType, 2> subset_sigs)
{
  auto subsets = m_partition_set(m_LIS[idx1][idx2]);
  auto sig_counter = size_t{0};
  auto output = bool{true};

  // Process the 1st subset
  const auto& set0 = subsets[0];
  assert(set0.length != 0);
  if (set0.length == 1) {
    m_LIP.push_back(set0.start);
    m_process_P(m_LIP.size() - 1, subset_sigs[0], sig_counter, output);
  }
  else {
    const auto newidx1 = set0.part_level;
    m_LIS[newidx1].emplace_back(set0);
    const auto newidx2 = m_LIS[newidx1].size() - 1;
    m_process_S(newidx1, newidx2, subset_sigs[0], sig_counter, output);
  }

  // Process the 2nd subset
  if (sig_counter == 0) {
    output = false;
    subset_sigs[1] = SigType::Sig;
  }
  const auto& set1 = subsets[1];
  assert(set1.length != 0);
  if (set1.length == 1) {
    m_LIP.push_back(set1.start);
    m_process_P(m_LIP.size() - 1, subset_sigs[1], sig_counter, output);
  }
  else {
    const auto newidx1 = set1.part_level;
    m_LIS[newidx1].emplace_back(set1);
    const auto newidx2 = m_LIS[newidx1].size() - 1;
    m_process_S(newidx1, newidx2, subset_sigs[1], sig_counter, output);
  }
}

auto sperr::SPECK1D_INT_ENC::m_decide_significance(const Set1D& set) const
    -> std::pair<SigType, int64_t>
{
  assert(set.length != 0);

  const auto gtr = [thld = m_threshold](auto v) { return v >= thld; };

  auto first = m_coeff_buf.cbegin() + set.start;
  auto last = first + set.length;
  auto found = std::find_if(first, last, gtr);
  if (found != last)
    return {SigType::Sig, std::distance(first, found)};
  else
    return {SigType::Insig, 0};
}
