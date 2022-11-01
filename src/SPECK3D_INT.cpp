#include "SPECK3D_INT.h"

#include <algorithm>
#include <cassert>
#include <numeric>

auto sperr::Set3D::is_pixel() const -> bool 
{ 
  return (length_x == 1 && length_y == 1 && length_z == 1); 
}

auto sperr::Set3D::is_empty() const -> bool 
{ 
  return (length_z == 0 || length_y == 0 || length_x == 0); 
}

void sperr::SPECK3D_INT::set_dims(dims_type dims)
{
  m_dims = dims;
}
  
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
    auto subsets = m_partition_S_XYZ(big);
    big = subsets[0];  // Reference `m_partition_S_XYZ()` for subset ordering
    // Also put the rest subsets in appropriate positions in `m_LIS`.
    std::for_each(std::next(subsets.cbegin()), subsets.cend(),
                  [&](const auto& s) { m_LIS[s.part_level].emplace_back(s); });
    xf++;
  }

  // One of these two conditions could happen if num_of_xforms_xy != num_of_xforms_z
  if (xf < num_of_xforms_xy) {
    while (xf < num_of_xforms_xy) {
      auto subsets = m_partition_S_XY(big);
      big = subsets[0];
      // Also put the rest subsets in appropriate positions in `m_LIS`.
      std::for_each(std::next(subsets.cbegin()), subsets.cend(),
                    [&](const auto& s) { m_LIS[s.part_level].emplace_back(s); });
      xf++;
    }
  }
  else if (xf < num_of_xforms_z) {
    while (xf < num_of_xforms_z) {
      auto subsets = m_partition_S_Z(big);
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

auto sperr::SPECK3D_INT::m_partition_S_XYZ(const Set3D& set) -> std::array<Set3D, 8>
{
  const auto split_x = std::array<uint32_t, 2>{set.length_x - set.length_x / 2, set.length_x / 2};
  const auto split_y = std::array<uint32_t, 2>{set.length_y - set.length_y / 2, set.length_y / 2};
  const auto split_z = std::array<uint32_t, 2>{set.length_z - set.length_z / 2, set.length_z / 2};

  auto next_part_lev = set.part_level;
  next_part_lev += split_x[1] > 0 ? 1 : 0;
  next_part_lev += split_y[1] > 0 ? 1 : 0;
  next_part_lev += split_z[1] > 0 ? 1 : 0;

  std::array<Set3D, 8> subsets;

#pragma GCC unroll 8
  for (auto& s : subsets)
    s.part_level = next_part_lev;

  constexpr auto offsets = std::array<size_t, 3>{1, 2, 4};

  // The actual figuring out where it starts/ends part...
  //
  // subset (0, 0, 0)
  constexpr auto idx0 = 0 * offsets[0] + 0 * offsets[1] + 0 * offsets[2];
  auto& sub0 = subsets[idx0];
  sub0.start_x = set.start_x;
  sub0.length_x = split_x[0];
  sub0.start_y = set.start_y;
  sub0.length_y = split_y[0];
  sub0.start_z = set.start_z;
  sub0.length_z = split_z[0];

  // subset (1, 0, 0)
  constexpr auto idx1 = 1 * offsets[0] + 0 * offsets[1] + 0 * offsets[2];
  auto& sub1 = subsets[idx1];
  sub1.start_x = set.start_x + split_x[0];
  sub1.length_x = split_x[1];
  sub1.start_y = set.start_y;
  sub1.length_y = split_y[0];
  sub1.start_z = set.start_z;
  sub1.length_z = split_z[0];

  // subset (0, 1, 0)
  constexpr auto idx2 = 0 * offsets[0] + 1 * offsets[1] + 0 * offsets[2];
  auto& sub2 = subsets[idx2];
  sub2.start_x = set.start_x;
  sub2.length_x = split_x[0];
  sub2.start_y = set.start_y + split_y[0];
  sub2.length_y = split_y[1];
  sub2.start_z = set.start_z;
  sub2.length_z = split_z[0];

  // subset (1, 1, 0)
  constexpr auto idx3 = 1 * offsets[0] + 1 * offsets[1] + 0 * offsets[2];
  auto& sub3 = subsets[idx3];
  sub3.start_x = set.start_x + split_x[0];
  sub3.length_x = split_x[1];
  sub3.start_y = set.start_y + split_y[0];
  sub3.length_y = split_y[1];
  sub3.start_z = set.start_z;
  sub3.length_z = split_z[0];

  // subset (0, 0, 1)
  constexpr auto idx4 = 0 * offsets[0] + 0 * offsets[1] + 1 * offsets[2];
  auto& sub4 = subsets[idx4];
  sub4.start_x = set.start_x;
  sub4.length_x = split_x[0];
  sub4.start_y = set.start_y;
  sub4.length_y = split_y[0];
  sub4.start_z = set.start_z + split_z[0];
  sub4.length_z = split_z[1];

  // subset (1, 0, 1)
  constexpr auto idx5 = 1 * offsets[0] + 0 * offsets[1] + 1 * offsets[2];
  auto& sub5 = subsets[idx5];
  sub5.start_x = set.start_x + split_x[0];
  sub5.length_x = split_x[1];
  sub5.start_y = set.start_y;
  sub5.length_y = split_y[0];
  sub5.start_z = set.start_z + split_z[0];
  sub5.length_z = split_z[1];

  // subset (0, 1, 1)
  constexpr auto idx6 = 0 * offsets[0] + 1 * offsets[1] + 1 * offsets[2];
  auto& sub6 = subsets[idx6];
  sub6.start_x = set.start_x;
  sub6.length_x = split_x[0];
  sub6.start_y = set.start_y + split_y[0];
  sub6.length_y = split_y[1];
  sub6.start_z = set.start_z + split_z[0];
  sub6.length_z = split_z[1];

  // subset (1, 1, 1)
  constexpr auto idx7 = 1 * offsets[0] + 1 * offsets[1] + 1 * offsets[2];
  auto& sub7 = subsets[idx7];
  sub7.start_x = set.start_x + split_x[0];
  sub7.length_x = split_x[1];
  sub7.start_y = set.start_y + split_y[0];
  sub7.length_y = split_y[1];
  sub7.start_z = set.start_z + split_z[0];
  sub7.length_z = split_z[1];

  return subsets;
}

auto sperr::SPECK3D_INT::m_partition_S_XY(const Set3D& set) -> std::array<Set3D, 4>
{
  std::array<Set3D, 4> subsets;

  const auto split_x = std::array<uint32_t, 2>{set.length_x - set.length_x / 2, set.length_x / 2};
  const auto split_y = std::array<uint32_t, 2>{set.length_y - set.length_y / 2, set.length_y / 2};

  for (auto& s : subsets) {
    s.part_level = set.part_level;
    if (split_x[1] > 0)
      s.part_level++;
    if (split_y[1] > 0)
      s.part_level++;
  }

  //
  // The actual figuring out where it starts/ends part...
  //
  const auto offsets = std::array<size_t, 3>{1, 2, 4};
  // subset (0, 0, 0)
  size_t sub_i = 0 * offsets[0] + 0 * offsets[1] + 0 * offsets[2];
  auto& sub0 = subsets[sub_i];
  sub0.start_x = set.start_x;
  sub0.length_x = split_x[0];
  sub0.start_y = set.start_y;
  sub0.length_y = split_y[0];
  sub0.start_z = set.start_z;
  sub0.length_z = set.length_z;

  // subset (1, 0, 0)
  sub_i = 1 * offsets[0] + 0 * offsets[1] + 0 * offsets[2];
  auto& sub1 = subsets[sub_i];
  sub1.start_x = set.start_x + split_x[0];
  sub1.length_x = split_x[1];
  sub1.start_y = set.start_y;
  sub1.length_y = split_y[0];
  sub1.start_z = set.start_z;
  sub1.length_z = set.length_z;

  // subset (0, 1, 0)
  sub_i = 0 * offsets[0] + 1 * offsets[1] + 0 * offsets[2];
  auto& sub2 = subsets[sub_i];
  sub2.start_x = set.start_x;
  sub2.length_x = split_x[0];
  sub2.start_y = set.start_y + split_y[0];
  sub2.length_y = split_y[1];
  sub2.start_z = set.start_z;
  sub2.length_z = set.length_z;

  // subset (1, 1, 0)
  sub_i = 1 * offsets[0] + 1 * offsets[1] + 0 * offsets[2];
  auto& sub3 = subsets[sub_i];
  sub3.start_x = set.start_x + split_x[0];
  sub3.length_x = split_x[1];
  sub3.start_y = set.start_y + split_y[0];
  sub3.length_y = split_y[1];
  sub3.start_z = set.start_z;
  sub3.length_z = set.length_z;

  return subsets;
}

auto sperr::SPECK3D_INT::m_partition_S_Z(const Set3D& set) -> std::array<Set3D, 2>
{
  std::array<Set3D, 2> subsets;

  const auto split_z = std::array<uint32_t, 2>{set.length_z - set.length_z / 2, set.length_z / 2};

  for (auto& s : subsets) {
    s.part_level = set.part_level;
    if (split_z[1] > 0)
      s.part_level++;
  }

  //
  // The actual figuring out where it starts/ends part...
  //
  // subset (0, 0, 0)
  auto& sub0 = subsets[0];
  sub0.start_x = set.start_x;
  sub0.length_x = set.length_x;
  sub0.start_y = set.start_y;
  sub0.length_y = set.length_y;
  sub0.start_z = set.start_z;
  sub0.length_z = split_z[0];

  // subset (0, 0, 1)
  auto& sub1 = subsets[1];
  sub1.start_x = set.start_x;
  sub1.length_x = set.length_x;
  sub1.start_y = set.start_y;
  sub1.length_y = set.length_y;
  sub1.start_z = set.start_z + split_z[0];
  sub1.length_z = split_z[1];

  return subsets;
}
