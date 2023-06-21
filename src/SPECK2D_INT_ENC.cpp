#include "SPECK2D_INT_ENC.h"

#include <cassert>

template <typename T>
void sperr::SPECK2D_INT_ENC<T>::m_sorting_pass()
{
  // First, process all insignificant pixels.
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

  // Second, process all TypeS sets.
  //
  for (size_t tmp = 1; tmp <= m_LIS.size(); tmp++) {
    auto idx1 = m_LIS.size() - tmp;
    for (size_t idx2 = 0; idx2 < m_LIS[idx1].size(); idx2++) {
      size_t dummy = 0;
      m_process_S(idx1, idx2, dummy, true);
    }
  }

  // Third, process the sole TypeI set.
  //
  m_process_I(true);
}

template <typename T>
void sperr::SPECK2D_INT_ENC<T>::m_process_S(size_t idx1, size_t idx2, size_t& counter, bool need_decide)
{
  auto& set = m_LIS[idx1][idx2];
  assert(!set.is_pixel());
  bool is_sig = true;

  if (need_decide) {
    is_sig = m_decide_S_significance(set);
    m_bit_buffer.wbit(is_sig);
  }

  if (is_sig) {
    counter++;
    m_code_S(idx1, idx2);
    set.type = SetType::Garbage;
  }
}

template <typename T>
void sperr::SPECK2D_INT_ENC<T>::m_process_P(size_t idx, size_t& counter, bool need_decide)
{
  bool is_sig = true;

  if (need_decide) {
    is_sig = (m_coeff_buf[idx] >= m_threshold);
    m_bit_buffer.wbit(is_sig);
  }

  if (is_sig) {
    counter++;
    m_coeff_buf[idx] -= m_threshold;
    m_bit_buffer.wbit(m_sign_array[idx]);
    m_LSP_new.push_back(idx);
    m_LIP_mask.write_false(idx);
  }
}

template <typename T>
void sperr::SPECK2D_INT_ENC<T>::m_process_I(bool need_decide)
{
  if (m_I.part_level > 0) {  // m_I is not empty yet
    bool is_sig = true;
    if (need_decide)
      is_sig = m_decide_I_significance();
    m_bit_buffer.wbit(is_sig);

    if (is_sig)
      m_code_I();
  }
}

template <typename T>
void sperr::SPECK2D_INT_ENC<T>::m_code_S(size_t idx1, size_t idx2)
{
  auto set = m_LIS[idx1][idx2];
  auto subsets = m_partition_S(set);
  const auto set_end = 
      std::remove_if(subsets.begin(), subsets.end(), [](auto s) { return s.is_empty(); });
  const auto set_end_m1 = set_end - 1;

  auto counter = size_t{0};
  for (auto it = subsets.begin(); it != set_end; ++it) {
    auto need_decide = (it != set_end_m1) || (counter != 0);
    if (it->is_pixel()) {
      auto pixel_idx = it->start_y * m_dims[0] + it->start_x;
      m_LIP_mask.write_true(pixel_idx);
      m_process_P(pixel_idx, counter, need_decide);
    }
    else {
      auto newidx1 = it->part_level;
      m_LIS[newidx1].push_back(*it);
      auto newidx2 = m_LIS[newidx1].size() - 1;
      m_process_S(newidx1, newidx2, counter, need_decide);
    }
  }
}

template <typename T>
void sperr::SPECK2D_INT_ENC<T>::m_code_I()
{
  auto subsets = m_partition_I();

  auto counter = size_t{0};
  for (auto& set : subsets) {
    if (!set.is_empty()) {
      auto newidx1 = set.part_level;
      m_LIS[newidx1].push_back(set);
      m_process_S(newidx1, m_LIS[newidx1].size() - 1, counter, true);
    }
  }

  m_process_I(counter != 0);
}

template <typename T>
auto sperr::SPECK2D_INT_ENC<T>::m_decide_S_significance(Set2D set) const -> bool
{
  assert(!set.is_empty());

  const auto gtr = [thrd = m_threshold](auto v) { return v >= thrd; };
  for (auto y = set.start_y; y < (set.start_y + set.length_y); y++) {
    auto first = m_coeff_buf.cbegin() + y * m_dims[0] + set.start_x;
    if (std::any_of(first, first + set.length_x, gtr))
      return true;
  }
  return false;
}

template <typename T>
auto sperr::SPECK2D_INT_ENC<T>::m_decide_I_significance() const -> bool
{
  const auto gtr = [thrd = m_threshold](auto v) { return v >= thrd; };

  // First, test the bottom rectangle.
  // It's stored in a contiguous chunk of memory till the buffer end.
  //
  assert(m_I.length_x == m_dims[0]);
  auto first = m_coeff_buf.cbegin() + size_t{m_I.start_y} * size_t{m_I.length_x};
  if (std::any_of(first, m_coeff_buf.cend(), gtr))
    return true;

  // Second, test the rectangle that's directly to the right of the missing top-left corner.
  //
  for (auto y = 0; y < m_I.start_y; y++) {
    first = m_coeff_buf.cbegin() + y * m_dims[0] + m_I.start_x;
    auto last = m_coeff_buf.cbegin() + (y + 1) * m_dims[0];
    if (std::any_of(first, last, gtr))
      return true;
  }
  return false;
}

template class sperr::SPECK2D_INT_ENC<uint8_t>;
template class sperr::SPECK2D_INT_ENC<uint16_t>;
template class sperr::SPECK2D_INT_ENC<uint32_t>;
template class sperr::SPECK2D_INT_ENC<uint64_t>;
