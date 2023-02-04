#include "SPECK1D_INT.h"

#include <algorithm>
#include <cassert>
#include <cstring>
#include <numeric>

void sperr::SPECK1D_INT::m_clean_LIS()
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

void sperr::SPECK1D_INT::m_initialize_lists()
{
  const auto total_len = m_dims[0];
  auto num_of_parts = sperr::num_of_partitions(total_len);
  auto num_of_lists = num_of_parts + 1;
  if (m_LIS.size() < num_of_lists)
    m_LIS.resize(num_of_lists);
  for (auto& list : m_LIS)
    list.clear();

  // Put in two sets, each representing a half of the long array.
  Set1D set;
  set.length = total_len;  // Set represents the whole 1D array.
  auto sets = m_partition_set(set);
  m_LIS[sets[0].part_level].emplace_back(sets[0]);
  m_LIS[sets[1].part_level].emplace_back(sets[1]);
}

auto sperr::SPECK1D_INT::m_partition_set(const Set1D& set) const -> std::array<Set1D, 2>
{
  std::array<Set1D, 2> subsets;
  // Prepare the 1st set
  auto& set1 = subsets[0];
  set1.start = set.start;
  set1.length = set.length - set.length / 2;
  set1.part_level = set.part_level + 1;
  // Prepare the 2nd set
  auto& set2 = subsets[1];
  set2.start = set.start + set1.length;
  set2.length = set.length / 2;
  set2.part_level = set.part_level + 1;

  return subsets;
}