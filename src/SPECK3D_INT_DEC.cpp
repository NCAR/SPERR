#include "SPECK3D_INT_DEC.h"

#include <algorithm>
#include <cassert>
#include <cstring>  // std::memcpy()
#include <numeric>

template <typename T>
void sperr::SPECK3D_INT_DEC<T>::m_sorting_pass()
{
  // Since we have a separate representation of LIP, let's process that list first
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
  //
  for (size_t tmp = 1; tmp <= m_LIS.size(); tmp++) {
    // From the end of m_LIS to its front
    size_t idx1 = m_LIS.size() - tmp;
    for (size_t idx2 = 0; idx2 < m_LIS[idx1].size(); idx2++) {
      size_t dummy = 0;
      m_process_S(idx1, idx2, dummy, true);
    }
  }
}

template <typename T>
void sperr::SPECK3D_INT_DEC<T>::m_process_S(size_t idx1, size_t idx2, size_t& counter, bool read)
{
  auto& set = m_LIS[idx1][idx2];

  bool is_sig = true;

  if (read)
    is_sig = m_bit_buffer.rbit();

  if (is_sig) {
    counter++;
    m_code_S(idx1, idx2);
    set.make_empty();  // this current set is gonna be discarded.
  }
}

template <typename T>
void sperr::SPECK3D_INT_DEC<T>::m_process_P(size_t idx, size_t& counter, bool read)
{
  bool is_sig = true;

  if (read)
    is_sig = m_bit_buffer.rbit();

  if (is_sig) {
    counter++;  // Let's increment the counter first!
    m_sign_array[idx] = m_bit_buffer.rbit();

    m_LSP_new.push_back(idx);
    m_LIP_mask.write_false(idx);
  }
}

template <typename T>
void sperr::SPECK3D_INT_DEC<T>::m_code_S(size_t idx1, size_t idx2)
{
  auto [subsets, next_lev] = m_partition_S_XYZ(m_LIS[idx1][idx2], uint16_t(idx1));
  const auto set_end =
      std::remove_if(subsets.begin(), subsets.end(), [](auto& s) { return s.is_empty(); });
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
      auto idx = it->start_z * m_dims[0] * m_dims[1] + it->start_y * m_dims[0] + it->start_x;
      m_LIP_mask.write_true(idx);
      m_process_P(idx, sig_counter, read);
    }
    else {
      m_LIS[next_lev].emplace_back(*it);
      const auto newidx2 = m_LIS[next_lev].size() - 1;
      m_process_S(next_lev, newidx2, sig_counter, read);
    }
  }
}

template class sperr::SPECK3D_INT_DEC<uint64_t>;
template class sperr::SPECK3D_INT_DEC<uint32_t>;
template class sperr::SPECK3D_INT_DEC<uint16_t>;
template class sperr::SPECK3D_INT_DEC<uint8_t>;
