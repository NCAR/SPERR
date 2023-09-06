#include "SPECK3D_INT_ENC.h"

#include <algorithm>
#include <cassert>
#include <cstring>  // std::memcpy()
#include <numeric>

template <typename T>
void sperr::SPECK3D_INT_ENC<T>::m_deposit_set(Set3D set)
{
  switch (set.num_elem()) {
    case 0:
      break;
    case 1: {
      auto idx = set.start_z * m_dims[0] * m_dims[1] + set.start_y * m_dims[0] + set.start_x;
      m_morton_buf[set.get_morton()] = m_coeff_buf[idx];
      break;
    }
    case 2: {
      // We directly deposit the 2 elements in `set` instead of performing another partition.
      //
      // Deposit the 1st element.
      auto idx = set.start_z * m_dims[0] * m_dims[1] + set.start_y * m_dims[0] + set.start_x;
      auto idx_morton = set.get_morton();
      m_morton_buf[idx_morton] = m_coeff_buf[idx];

      // Deposit the 2nd element.
      if (set.length_x == 2)
        idx++;
      else if (set.length_y == 2)
        idx += m_dims[0];
      else
        idx += m_dims[0] * m_dims[1];
      m_morton_buf[idx_morton + 1] = m_coeff_buf[idx];

      break;
    }
    default: {
      auto [subsets, lev] = m_partition_S_XYZ(set, 0);
      for (auto& sub : subsets)
        m_deposit_set(sub);
    }
  }
}

template <typename T>
void sperr::SPECK3D_INT_ENC<T>::m_additional_initialization()
{
  // For the encoder, this function re-organizes the coefficients in a morton order.
  //
  m_morton_buf.resize(m_dims[0] * m_dims[1] * m_dims[2]);

  // The same traversing order as in `SPECK3D_INT::m_sorting_pass()`
  size_t morton_offset = 0;
  for (size_t tmp = 1; tmp <= m_LIS.size(); tmp++) {
    auto idx1 = m_LIS.size() - tmp;
    for (size_t idx2 = 0; idx2 < m_LIS[idx1].size(); idx2++) {
      auto& set = m_LIS[idx1][idx2];
      set.set_morton(morton_offset);
      m_deposit_set(set);
      morton_offset += set.num_elem();
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
    auto first = m_morton_buf.cbegin() + set.get_morton();
    auto last = m_morton_buf.cbegin() + set.get_morton() + set.num_elem();
    is_sig = std::any_of(first, last, [thld = m_threshold](auto v) { return v >= thld; });
    m_bit_buffer.wbit(is_sig);
  }

  if (is_sig) {
    counter++;
    m_code_S(idx1, idx2);
    set.make_empty();  // this current set is gonna be discarded.
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

template class sperr::SPECK3D_INT_ENC<uint64_t>;
template class sperr::SPECK3D_INT_ENC<uint32_t>;
template class sperr::SPECK3D_INT_ENC<uint16_t>;
template class sperr::SPECK3D_INT_ENC<uint8_t>;
