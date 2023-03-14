#include "SPECK3D_INT_DEC.h"

#include <algorithm>
#include <cassert>
#include <cstring>  // std::memcpy()
#include <numeric>

void sperr::SPECK3D_INT_DEC::decode()
{
  //if (m_ready_to_decode() == false)
  //  return RTNType::Error;

  m_initialize_lists();

  // initialize coefficients to be zero, and sign array to be all positive
  const auto coeff_len = m_dims[0] * m_dims[1] * m_dims[2];
  m_coeff_buf.assign(coeff_len, uint_t{0});
  m_sign_array.assign(coeff_len, true);

  // Mark every coefficient as insignificant
  //m_LSP_mask.assign(m_coeff_buf.size(), false);
  m_LSP_mask.resize(m_coeff_buf.size());
  m_LSP_mask.reset();
  m_bit_itr = m_bit_buffer.cbegin();

  // Restore the biggest `m_threshold`
  m_threshold = 1;
  for (uint8_t i = 1; i < m_num_bitplanes; i++)
    m_threshold *= uint_t{2};

  for (uint8_t bitplane = 0; bitplane < m_num_bitplanes; bitplane++) {
    m_sorting_pass();
    m_refinement_pass_decode();

    m_threshold /= uint_t{2};
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

void sperr::SPECK3D_INT_DEC::m_process_S(size_t idx1, size_t idx2, size_t& counter, bool read)
{
  auto& set = m_LIS[idx1][idx2];

  bool is_sig = true;

  if (read) {
    is_sig = *m_bit_itr;
    ++m_bit_itr;
  }

  if (is_sig) {
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
  auto subsets = m_partition_S_XYZ(m_LIS[idx1][idx2]);
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

