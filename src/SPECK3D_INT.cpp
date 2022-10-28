#include "SPECK3D_INT.h"

#include <algorithm>
#include <cassert>
#include <numeric>

void sperr::SPECK3D_INT::m_clean_LIS()
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

void sperr::SPECK3D_INT::m_initialize_sets_lists()
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

