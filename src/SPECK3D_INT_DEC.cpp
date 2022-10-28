#include "SPECK3D_INT_DEC.h"

#include <algorithm>
#include <cassert>
#include <numeric>

void sperr::SPECK3D_INT_DEC::set_threshold_dims(uint64_t t, dims_type dims)
{
  m_threshold = static_cast<int_t>(t);
  m_dims = dims;
}

void sperr::SPECK3D_INT_DEC::set_bitstream(vecb_type stream)
{
  m_bit_buffer = std::move(stream);
}

auto sperr::SPECK3D_INT_DEC::release_coeffs() -> veci_t&&
{
  return std::move(m_coeff_buf);
}

auto sperr::SPECK3D_INT_DEC::release_signs() -> vecb_type&&
{
  return std::move(m_sign_array);
}

void sperr::SPECK3D_INT_DEC::decode()
{
  //if (m_ready_to_decode() == false)
  //  return RTNType::Error;

  m_initialize_sets_lists();

  // initialize coefficients to be zero, and sign array to be all positive
  const auto coeff_len = m_dims[0] * m_dims[1] * m_dims[2];
  m_coeff_buf.assign(coeff_len, int_t{0});
  m_sign_array.assign(coeff_len, true);

  // Mark every coefficient as insignificant
  m_LSP_mask.assign(m_coeff_buf.size(), false);
  m_bit_itr = m_bit_buffer.cbegin();

  // Find out how many bitplanes to proces
  size_t num_bitplanes = 1;
  auto thrd = m_threshold;
  while (thrd / int_t{2} > int_t{0}) {
    thrd /= int_t{2};
    num_bitplanes++;
  }

  for (size_t bitplane = 0; bitplane < num_bitplanes; bitplane++) {
    m_sorting_pass();
    m_refinement_pass();

    m_threshold /= int_t{2};
    m_clean_LIS();
  }
}


void sperr::SPECK3D_INT_DEC::m_sorting_pass()
{
  // Since we have a separate representation of LIP, let's process that list first
  //
  size_t dummy = 0;
  for (size_t loc = 0; loc < m_LIP.size(); loc++)
    m_process_P(loc, dummy, true);

  // Then we process regular sets in LIS.
  //
  for (size_t tmp = 1; tmp <= m_LIS.size(); tmp++) {
    // From the end of m_LIS to its front
    size_t idx1 = m_LIS.size() - tmp;
    for (size_t idx2 = 0; idx2 < m_LIS[idx1].size(); idx2++)
      m_process_S(idx1, idx2, dummy, true);
  }
}

void sperr::SPECK3D_INT_DEC::m_refinement_pass()
{
  // First, process significant pixels previously found.
  //
  const auto tmpd = d2_type{m_threshold * -0.5, m_threshold * 0.5};

  assert(m_bit_buffer.size() >= m_bit_idx);
  if (m_bit_buffer.size() - m_bit_idx > m_LSP_mask_cnt) {  // No need to check BitBudgetMet
    for (size_t i = 0; i < m_LSP_mask.size(); i++) {
      if (m_LSP_mask[i])
        m_coeff_buf[i] += tmpd[m_bit_buffer[m_bit_idx++]];
    }
  }
  else {  // Need to check BitBudgetMet
    for (size_t i = 0; i < m_LSP_mask.size(); i++) {
      if (m_LSP_mask[i]) {
        m_coeff_buf[i] += tmpd[m_bit_buffer[m_bit_idx++]];
        if (m_bit_idx >= m_bit_buffer.size())
          return RTNType::BitBudgetMet;
      }
    }
  }

  // Second, mark newly found significant pixels in `m_LSP_mark`
  //
  for (auto idx : m_LSP_new)
    m_LSP_mask[idx] = true;
  m_LSP_mask_cnt += m_LSP_new.size();
  m_LSP_new.clear();

  return RTNType::Good;
}

void sperr::SPECK3D_INT_DEC::m_process_S(size_t idx1, size_t idx2, size_t& counter, bool read)
{
  auto& set = m_LIS[idx1][idx2];

  if (read) {
    set.signif = *m_bit_itr ? SigType::Sig : SigType::Insig;
    ++m_bit_itr;
  }
  else {
    set.signif = SigType::Sig;
  }

  if (set.signif == SigType::Sig) {
    counter++;  // Let's increment the counter first!
    m_code_S(idx1, idx2);
    set.type = SetType::Garbage;  // this current set is gonna be discarded.
  }
}

void sperr::SPECK3D_INT_DEC::m_process_P(size_t loc, size_t& counter, bool read)
{
  bool is_sig = true;
  const auto pixel_idx = m_LIP[loc];

  if (read) {
    is_sig = *m_bit_itr;
    ++m_bit_itr;
  }

  if (is_sig) {
    counter++;  // Let's increment the counter first!
    m_sign_array[pixel_idx] = *m_bit_itr;
    ++m_bit_itr;

    m_coeff_buf[pixel_idx] = m_threshold;
    m_LSP_new.push_back(pixel_idx);

    m_LIP[loc] = m_u64_garbage_val;
  }
}

void sperr::SPECK3D_INT_DEC::m_code_S(size_t idx1, size_t idx2)
{
  auto subsets = sperr::partition_S_XYZ(m_LIS[idx1][idx2]);
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
      m_LIP.push_back(it->start_z * m_dims[0] * m_dims[1] + it->start_y * m_dims[0] + it->start_x);
      m_process_P(m_LIP.size() - 1, sig_counter, read);
    }
    else {
      const auto newidx1 = it->part_level;
      m_LIS[newidx1].emplace_back(*it);
      const auto newidx2 = m_LIS[newidx1].size() - 1;
      m_process_S(newidx1, newidx2, sig_counter, read);
    }
  }
}

