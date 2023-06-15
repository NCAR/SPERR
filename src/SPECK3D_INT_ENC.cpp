#include "SPECK3D_INT_ENC.h"

#include <algorithm>
#include <cassert>
#include <cstring>  // std::memcpy()
#include <numeric>

template <typename T>
void sperr::SPECK3D_INT_ENC<T>::m_sorting_pass()
{
  // Since we have a separate representation of LIP, let's process that list first!
  //
  const auto bits_x64 = m_LIP_mask.size() - m_LIP_mask.size() % 64;

  for (size_t i = 0; i < bits_x64; i += 64) {
    const auto value = m_LIP_mask.read_long(i);
    if (value != 0) {
      for (size_t j = 0; j < 64; j++) {
        if ((value >> j) & uint64_t{1}) {
          size_t dummy = 0;
          m_process_P(i + j, SigType::Dunno, dummy, true);
        }
      }
    }
  }
  for (auto i = bits_x64; i < m_LIP_mask.size(); i++) {
    if (m_LIP_mask.read_bit(i)) {
      size_t dummy = 0;
      m_process_P(i, SigType::Dunno, dummy, true);
    }
  }

  // Then we process regular sets in LIS.
  //
  for (size_t tmp = 1; tmp <= m_LIS.size(); tmp++) {
    // From the end of m_LIS to its front
    size_t idx1 = m_LIS.size() - tmp;
    for (size_t idx2 = 0; idx2 < m_LIS[idx1].size(); idx2++) {
      size_t dummy = 0;
      m_process_S(idx1, idx2, SigType::Dunno, dummy, true);
    }
  }
}

template <typename T>
void sperr::SPECK3D_INT_ENC<T>::m_process_S(size_t idx1,
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
    sig = set_sig ? SigType::Sig : SigType::Insig;
    if (set_sig) {
      // Try to deduce the significance of some of its subsets.
      // Step 1: which one of the 8 subsets makes it significant?
      //         (Refer to m_partition_S_XYZ() for subset ordering.)
      auto xyz = *set_sig;
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

  if (output)
    m_bit_buffer.wbit(sig == SigType::Sig);

  if (sig == SigType::Sig) {
    counter++;  // Let's increment the counter first!
    m_code_S(idx1, idx2, subset_sigs);
    set.type = SetType::Garbage;  // this current set is gonna be discarded.
  }
}

template <typename T>
void sperr::SPECK3D_INT_ENC<T>::m_process_P(size_t idx, SigType sig, size_t& counter, bool output)
{
  // Decide the significance of this pixel
  assert(sig != SigType::NewlySig);
  bool is_sig = false;
  if (sig == SigType::Dunno)
    is_sig = (m_coeff_buf[idx] >= m_threshold);
  else
    is_sig = (sig == SigType::Sig);

  if (output)
    m_bit_buffer.wbit(is_sig);

  if (is_sig) {
    counter++;  // Let's increment the counter first!
    m_bit_buffer.wbit(m_sign_array[idx]);

    assert(m_coeff_buf[idx] >= m_threshold);
    m_coeff_buf[idx] -= m_threshold;
    m_LSP_new.push_back(idx);
    m_LIP_mask.write_false(idx);
  }
}

template <typename T>
void sperr::SPECK3D_INT_ENC<T>::m_code_S(size_t idx1,
                                         size_t idx2,
                                         std::array<SigType, 8> subset_sigs)
{
  auto subsets = m_partition_S_XYZ(m_LIS[idx1][idx2]);

  // Since some subsets could be empty, let's put empty sets at the end.
  //
  for (size_t i = 0; i < subsets.size(); i++) {
    if (subsets[i].is_empty())
      subset_sigs[i] = SigType::Garbage;  // SigType::Garbage is only used locally here.
  }
  auto sig_end = std::remove(subset_sigs.begin(), subset_sigs.end(), SigType::Garbage);
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
      auto idx = it->start_z * m_dims[0] * m_dims[1] + it->start_y * m_dims[0] + it->start_x;
      m_LIP_mask.write_true(idx);
      m_process_P(idx, *sig_it, sig_counter, output);
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

template <typename T>
auto sperr::SPECK3D_INT_ENC<T>::m_decide_significance(const Set3D& set) const
    -> std::optional<std::array<uint32_t, 3>>
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
        auto arr = std::array<uint32_t, 3>{x, y - set.start_y, z - set.start_z};
        return std::optional<std::array<uint32_t, 3>>(arr);
      }
    }
  }

  return std::optional<std::array<uint32_t, 3>>();
}

template class sperr::SPECK3D_INT_ENC<uint64_t>;
template class sperr::SPECK3D_INT_ENC<uint32_t>;
template class sperr::SPECK3D_INT_ENC<uint16_t>;
template class sperr::SPECK3D_INT_ENC<uint8_t>;
