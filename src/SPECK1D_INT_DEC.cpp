#include "SPECK1D_INT_DEC.h"

#include <algorithm>
#include <cassert>
#include <cstring>  // std::memcpy()
#include <numeric>

void sperr::SPECK1D_INT_DEC::decode()
{
  m_initialize_lists();

  // initialize coefficients to be zero, and sign array to be all positive
  const auto coeff_len = m_dims[0];
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


void sperr::SPECK1D_INT_DEC::m_sorting_pass()
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

void sperr::SPECK1D_INT_DEC::m_process_S(size_t idx1, size_t idx2, size_t& counter, bool read)
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

void sperr::SPECK1D_INT_DEC::m_process_P(size_t loc, size_t& counter, bool read)
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

void sperr::SPECK1D_INT_DEC::m_code_S(size_t idx1, size_t idx2)
{
  auto subsets = m_partition_set(m_LIS[idx1][idx2]);
  auto sig_counter = size_t{0};
  auto read = bool{true};

  // Process the 1st subset
  const auto& set0 = subsets[0];
  assert(set0.length != 0);
  if (set0.length == 1) {
    m_LIP.push_back(set0.start);
    m_process_P(m_LIP.size() - 1, sig_counter, read);
  }
  else {
    const auto newidx1 = set0.part_level;
    m_LIS[newidx1].push_back(set0);
    m_process_S(newidx1, m_LIS[newidx1].size() - 1, sig_counter, read);
  }

  // Process the 2nd subset
  if (sig_counter == 0)
    read = false;
  const auto& set1 = subsets[1];
  assert(set1.length != 0);
  if (set1.length == 1 ) {
    m_LIP.push_back(set1.start);
    m_process_P(m_LIP.size() - 1, sig_counter, read);
  }
  else {
    const auto newidx1 = set1.part_level;
    m_LIS[newidx1].push_back(set1);
    m_process_S(newidx1, m_LIS[newidx1].size() - 1, sig_counter, read);
  }
}



