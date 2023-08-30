#include "SPECK3D_INT_ENC.h"

#include <algorithm>
#include <cassert>
#include <cstring>  // std::memcpy()
#include <numeric>

template <typename T>
void sperr::SPECK3D_INT_ENC<T>::m_deposit_set(const Set3D& set)
{
  switch (set.num_elem()) {
    case 0:
      break;
    case 1: {
      auto idx = set.start_z * m_dims[0] * m_dims[1] + set.start_y * m_dims[0] + set.start_x;
      m_morton_buf[set.morton_offset] = m_coeff_buf[idx];
      break;
    }
    case 2: {
      // We directly deposit the 2 elements in `set` instead of performing another partition.
      //
      // Deposit the 1st element.
      auto idx = set.start_z * m_dims[0] * m_dims[1] + set.start_y * m_dims[0] + set.start_x;
      m_morton_buf[set.morton_offset] = m_coeff_buf[idx];

      // Deposit the 2nd element.
      if (set.length_x == 2)
        idx++;
      else if (set.length_y == 2)
        idx += m_dims[0];
      else
        idx += m_dims[0] * m_dims[1];
      m_morton_buf[set.morton_offset + 1] = m_coeff_buf[idx];

      break;
    }
    default: {
      auto subsets = m_partition_S_XYZ(set);
      for (auto& sub : subsets)
        m_deposit_set(sub);
    }
  }
}

template <typename T>
void sperr::SPECK3D_INT_ENC<T>::m_construct_morton_buf()
{
  m_morton_buf.resize(m_dims[0] * m_dims[1] * m_dims[2]);

  // The same traversing order as in `m_sorting_pass()`
  size_t morton_offset = 0;
  for (size_t tmp = 1; tmp <= m_LIS.size(); tmp++) {
    auto idx1 = m_LIS.size() - tmp;
    for (size_t idx2 = 0; idx2 < m_LIS[idx1].size(); idx2++) {
      auto& set = m_LIS[idx1][idx2];
      set.morton_offset = morton_offset;
      m_deposit_set(set);
      morton_offset += set.num_elem();
    }
  }
}

template <typename T>
void sperr::SPECK3D_INT_ENC<T>::m_sorting_pass()
{
  if (!m_morton_valid) {
    m_construct_morton_buf();
    m_morton_valid = true;
  }

  // Since we have a separate representation of LIP, let's process that list first!
  //
  const auto bits_x64 = m_LIP_mask.size() - m_LIP_mask.size() % 64;

  for (size_t i = 0; i < bits_x64; i += 64) {
    const auto value = m_LIP_mask.read_long(i);
    if (value != 0) {
      for (size_t j = 0; j < 64; j++) {
        if ((value >> j) & uint64_t{1}) {
          size_t dummy = 0;
          m_process_P(i + j, dummy, true);
        }
      }
    }
  }
  for (auto i = bits_x64; i < m_LIP_mask.size(); i++) {
    if (m_LIP_mask.read_bit(i)) {
      size_t dummy = 0;
      m_process_P(i, dummy, true);
    }
  }

  // Then we process regular sets in LIS.
  // (From the end of m_LIS to its front.)
  //
  for (size_t tmp = 1; tmp <= m_LIS.size(); tmp++) {
    auto idx1 = m_LIS.size() - tmp;
    for (size_t idx2 = 0; idx2 < m_LIS[idx1].size(); idx2++) {
      size_t dummy = 0;
      m_process_S(idx1, idx2, dummy, true);
    }
  }
}

template <typename T>
void sperr::SPECK3D_INT_ENC<T>::m_process_S(size_t idx1, size_t idx2, size_t& counter, bool output)
{
  auto& set = m_LIS[idx1][idx2];
  auto is_sig = true;

  // If need to output, it means the current set has unknown significance.
  if (output) {
    auto first = m_morton_buf.cbegin() + set.morton_offset;
    auto last = m_morton_buf.cbegin() + set.morton_offset + set.num_elem();
    is_sig = std::any_of(first, last, [thld = m_threshold](auto v) { return v >= thld; });
    m_bit_buffer.wbit(is_sig);
  }

  if (is_sig) {
    counter++;  // Let's increment the counter first!
    m_code_S(idx1, idx2);
    set.type = SetType::Garbage;  // this current set is gonna be discarded.
  }
}

template <typename T>
void sperr::SPECK3D_INT_ENC<T>::m_process_P(size_t idx, size_t& counter, bool output)
{
  bool is_sig = true;

  if (output) {
    is_sig = (m_coeff_buf[idx] >= m_threshold);
    m_bit_buffer.wbit(is_sig);
  }

  if (is_sig) {
    counter++;  // Let's increment the counter first!
    assert(m_coeff_buf[idx] >= m_threshold);
    m_coeff_buf[idx] -= m_threshold;

    m_bit_buffer.wbit(m_sign_array[idx]);
    m_LSP_new.push_back(idx);
    m_LIP_mask.write_false(idx);
  }
}

template <typename T>
void sperr::SPECK3D_INT_ENC<T>::m_code_S(size_t idx1, size_t idx2)
{
  auto subsets = m_partition_S_XYZ(m_LIS[idx1][idx2]);

  // Since some subsets could be empty, let's put empty sets at the end.
  //
  const auto set_end =
      std::remove_if(subsets.begin(), subsets.end(), [](auto& s) { return s.is_empty(); });
  const auto set_end_m1 = set_end - 1;

  size_t sig_counter = 0;
  for (auto it = subsets.begin(); it != set_end; ++it) {
    // If we're looking at the last subset, and no prior subset is found to be
    // significant, then we know that this last one *is* significant.
    bool output = true;
    if (it == set_end_m1 && sig_counter == 0)
      output = false;

    if (it->is_pixel()) {
      auto idx = it->start_z * m_dims[0] * m_dims[1] + it->start_y * m_dims[0] + it->start_x;
      m_LIP_mask.write_true(idx);
      m_process_P(idx, sig_counter, output);
    }
    else {
      const auto newidx1 = it->part_level;
      m_LIS[newidx1].emplace_back(*it);
      const auto newidx2 = m_LIS[newidx1].size() - 1;
      m_process_S(newidx1, newidx2, sig_counter, output);
    }
  }
}

template class sperr::SPECK3D_INT_ENC<uint64_t>;
template class sperr::SPECK3D_INT_ENC<uint32_t>;
template class sperr::SPECK3D_INT_ENC<uint16_t>;
template class sperr::SPECK3D_INT_ENC<uint8_t>;
