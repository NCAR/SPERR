#include "SPECK3D_INT.h"

#include <algorithm>
#include <cassert>
#include <cstring>
#include <numeric>

template <typename T>
void sperr::SPECK3D_INT<T>::m_clean_LIS()
{
  for (auto& list : m_LIS) {
    auto it = std::remove_if(list.begin(), list.end(),
                             [](const auto& s) { return s.type == SetType::Garbage; });
    list.erase(it, list.end());
  }
}

template <typename T>
void sperr::SPECK3D_INT<T>::m_initialize_lists()
{
  std::array<size_t, 3> num_of_parts;  // how many times each dimension could be partitioned?
  num_of_parts[0] = sperr::num_of_partitions(m_dims[0]);
  num_of_parts[1] = sperr::num_of_partitions(m_dims[1]);
  num_of_parts[2] = sperr::num_of_partitions(m_dims[2]);
  size_t num_of_sizes = std::accumulate(num_of_parts.cbegin(), num_of_parts.cend(), 1ul);

  // Initialize LIS
  const auto total_vals = m_dims[0] * m_dims[1] * m_dims[2];
  assert(total_vals > 0);
  if (m_LIS.size() < num_of_sizes)
    m_LIS.resize(num_of_sizes);
  std::for_each(m_LIS.begin(), m_LIS.end(), [](auto& list) { list.clear(); });
  m_LIP_mask.resize(total_vals);
  m_LIP_mask.reset();

  // Starting from a set representing the whole volume, identify the smaller
  //    subsets and put them in the LIS accordingly.
  //    Note that it truncates 64-bit ints to 16-bit ints here, but should be OK.
  Set3D big;
  big.length_x = static_cast<uint16_t>(m_dims[0]);  
  big.length_y = static_cast<uint16_t>(m_dims[1]);
  big.length_z = static_cast<uint16_t>(m_dims[2]);

  const auto num_xforms_xy = sperr::num_of_xforms(std::min(m_dims[0], m_dims[1]));
  const auto num_xforms_z = sperr::num_of_xforms(m_dims[2]);

  // Same logic as the choice of dyadic/wavelet_packet transform in CDF97.cpp.
  if ((num_xforms_xy == num_xforms_z) || (num_xforms_xy >= 5 && num_xforms_xy >= 5)) {
    auto num_xforms = std::min(num_xforms_xy, num_xforms_z);
    for (size_t i = 0; i < num_xforms; i++) {
      auto subsets = m_partition_S_XYZ(big);
      big = subsets[0];
      std::for_each(std::next(subsets.cbegin()), subsets.cend(),
                    [&](const auto& s) { m_LIS[s.part_level].emplace_back(s); });
    }
  }
  else {
    size_t xf = 0;
    while (xf < num_xforms_xy && xf < num_xforms_z) {
      auto subsets = m_partition_S_XYZ(big);
      big = subsets[0];
      std::for_each(std::next(subsets.cbegin()), subsets.cend(),
                    [&](const auto& s) { m_LIS[s.part_level].emplace_back(s); });
      xf++;
    }

    // One of these two conditions will happen.
    if (xf < num_xforms_xy) {
      while (xf < num_xforms_xy) {
        auto subsets = m_partition_S_XY(big);
        big = subsets[0];
        std::for_each(std::next(subsets.cbegin()), subsets.cend(),
                      [&](const auto& s) { m_LIS[s.part_level].emplace_back(s); });
        xf++;
      }
    }
    else if (xf < num_xforms_z) {
      while (xf < num_xforms_z) {
        auto subsets = m_partition_S_Z(big);
        big = subsets[0];
        const auto parts = subsets[1].part_level;
        m_LIS[parts].emplace_back(subsets[1]);
        xf++;
      }
    }
  }

  // Right now big is the set that's most likely to be significant, so insert
  // it at the front of it's corresponding vector. One-time expense.
  const auto parts = big.part_level;
  m_LIS[parts].insert(m_LIS[parts].begin(), big);

  // Experiment with morton curves.
  m_morton_valid = false;
}

template <typename T>
auto sperr::SPECK3D_INT<T>::m_partition_S_XYZ(const Set3D& set) const -> std::array<Set3D, 8>
{
  // Integer promotion rules (https://en.cppreference.com/w/c/language/conversion) say that types
  //    shorter than `int` are implicitly promoted to be `int` to perform calculations, so just
  //    keep them as `int` here because they'll involve in calculations later.
  //
  const auto split_x = std::array<int, 2>{set.length_x - set.length_x / 2, set.length_x / 2};
  const auto split_y = std::array<int, 2>{set.length_y - set.length_y / 2, set.length_y / 2};
  const auto split_z = std::array<int, 2>{set.length_z - set.length_z / 2, set.length_z / 2};

  auto next_part_lev = set.part_level;
  const auto tmp = std::array<uint16_t, 2>{0, 1};
  next_part_lev += tmp[split_x[1] != 0];
  next_part_lev += tmp[split_y[1] != 0];
  next_part_lev += tmp[split_z[1] != 0];

  std::array<Set3D, 8> subsets;
  constexpr auto offsets = std::array<size_t, 3>{1, 2, 4};

  //
  // The actual figuring out where it starts/ends part...
  //
  // subset (0, 0, 0)
  constexpr auto idx0 = 0 * offsets[0] + 0 * offsets[1] + 0 * offsets[2];
  auto& sub0 = subsets[idx0];
  sub0.start_x = set.start_x;
  sub0.start_y = set.start_y;
  sub0.start_z = set.start_z;
  sub0.length_x = split_x[0];
  sub0.length_y = split_y[0];
  sub0.length_z = split_z[0];
  sub0.morton_offset = set.morton_offset;
  sub0.part_level = next_part_lev;

  // subset (1, 0, 0)
  constexpr auto idx1 = 1 * offsets[0] + 0 * offsets[1] + 0 * offsets[2];
  auto& sub1 = subsets[idx1];
  sub1.start_x = set.start_x + split_x[0];
  sub1.start_y = set.start_y;
  sub1.start_z = set.start_z;
  sub1.length_x = split_x[1];
  sub1.length_y = split_y[0];
  sub1.length_z = split_z[0];
  sub1.morton_offset = sub0.morton_offset + sub0.num_elem();
  sub1.part_level = next_part_lev;

  // subset (0, 1, 0)
  constexpr auto idx2 = 0 * offsets[0] + 1 * offsets[1] + 0 * offsets[2];
  auto& sub2 = subsets[idx2];
  sub2.start_x = set.start_x;
  sub2.start_y = set.start_y + split_y[0];
  sub2.start_z = set.start_z;
  sub2.length_x = split_x[0];
  sub2.length_y = split_y[1];
  sub2.length_z = split_z[0];
  sub2.morton_offset = sub1.morton_offset + sub1.num_elem();
  sub2.part_level = next_part_lev;

  // subset (1, 1, 0)
  constexpr auto idx3 = 1 * offsets[0] + 1 * offsets[1] + 0 * offsets[2];
  auto& sub3 = subsets[idx3];
  sub3.start_x = set.start_x + split_x[0];
  sub3.start_y = set.start_y + split_y[0];
  sub3.start_z = set.start_z;
  sub3.length_x = split_x[1];
  sub3.length_y = split_y[1];
  sub3.length_z = split_z[0];
  sub3.morton_offset = sub2.morton_offset + sub2.num_elem();
  sub3.part_level = next_part_lev;

  // subset (0, 0, 1)
  constexpr auto idx4 = 0 * offsets[0] + 0 * offsets[1] + 1 * offsets[2];
  auto& sub4 = subsets[idx4];
  sub4.start_x = set.start_x;
  sub4.start_y = set.start_y;
  sub4.start_z = set.start_z + split_z[0];
  sub4.length_x = split_x[0];
  sub4.length_y = split_y[0];
  sub4.length_z = split_z[1];
  sub4.morton_offset = sub3.morton_offset + sub3.num_elem();
  sub4.part_level = next_part_lev;

  // subset (1, 0, 1)
  constexpr auto idx5 = 1 * offsets[0] + 0 * offsets[1] + 1 * offsets[2];
  auto& sub5 = subsets[idx5];
  sub5.start_x = set.start_x + split_x[0];
  sub5.start_y = set.start_y;
  sub5.start_z = set.start_z + split_z[0];
  sub5.length_x = split_x[1];
  sub5.length_y = split_y[0];
  sub5.length_z = split_z[1];
  sub5.morton_offset = sub4.morton_offset + sub4.num_elem();
  sub5.part_level = next_part_lev;

  // subset (0, 1, 1)
  constexpr auto idx6 = 0 * offsets[0] + 1 * offsets[1] + 1 * offsets[2];
  auto& sub6 = subsets[idx6];
  sub6.start_x = set.start_x;
  sub6.start_y = set.start_y + split_y[0];
  sub6.start_z = set.start_z + split_z[0];
  sub6.length_x = split_x[0];
  sub6.length_y = split_y[1];
  sub6.length_z = split_z[1];
  sub6.morton_offset = sub5.morton_offset + sub5.num_elem();
  sub6.part_level = next_part_lev;

  // subset (1, 1, 1)
  constexpr auto idx7 = 1 * offsets[0] + 1 * offsets[1] + 1 * offsets[2];
  auto& sub7 = subsets[idx7];
  sub7.start_x = set.start_x + split_x[0];
  sub7.start_y = set.start_y + split_y[0];
  sub7.start_z = set.start_z + split_z[0];
  sub7.length_x = split_x[1];
  sub7.length_y = split_y[1];
  sub7.length_z = split_z[1];
  sub7.morton_offset = sub6.morton_offset + sub6.num_elem();
  sub7.part_level = next_part_lev;

  return subsets;
}

template <typename T>
auto sperr::SPECK3D_INT<T>::m_partition_S_XY(const Set3D& set) const -> std::array<Set3D, 4>
{
  // This partition scheme is only used during initialization; no need to calculate morton offset.

  const auto split_x = std::array<int, 2>{set.length_x - set.length_x / 2, set.length_x / 2};
  const auto split_y = std::array<int, 2>{set.length_y - set.length_y / 2, set.length_y / 2};

  std::array<Set3D, 4> subsets;
  for (auto& s : subsets) {
    s.part_level = set.part_level;
    if (split_x[1] > 0)
      s.part_level++;
    if (split_y[1] > 0)
      s.part_level++;
  }

  const auto offsets = std::array<size_t, 3>{1, 2, 4};

  // The actual figuring out where it starts/ends part...
  //
  // subset (0, 0, 0)
  size_t sub_i = 0 * offsets[0] + 0 * offsets[1] + 0 * offsets[2];
  auto& sub0 = subsets[sub_i];
  sub0.start_x = set.start_x;
  sub0.start_y = set.start_y;
  sub0.start_z = set.start_z;
  sub0.length_x = split_x[0];
  sub0.length_y = split_y[0];
  sub0.length_z = set.length_z;

  // subset (1, 0, 0)
  sub_i = 1 * offsets[0] + 0 * offsets[1] + 0 * offsets[2];
  auto& sub1 = subsets[sub_i];
  sub1.start_x = set.start_x + split_x[0];
  sub1.start_y = set.start_y;
  sub1.start_z = set.start_z;
  sub1.length_x = split_x[1];
  sub1.length_y = split_y[0];
  sub1.length_z = set.length_z;

  // subset (0, 1, 0)
  sub_i = 0 * offsets[0] + 1 * offsets[1] + 0 * offsets[2];
  auto& sub2 = subsets[sub_i];
  sub2.start_x = set.start_x;
  sub2.start_y = set.start_y + split_y[0];
  sub2.start_z = set.start_z;
  sub2.length_x = split_x[0];
  sub2.length_y = split_y[1];
  sub2.length_z = set.length_z;

  // subset (1, 1, 0)
  sub_i = 1 * offsets[0] + 1 * offsets[1] + 0 * offsets[2];
  auto& sub3 = subsets[sub_i];
  sub3.start_x = set.start_x + split_x[0];
  sub3.start_y = set.start_y + split_y[0];
  sub3.start_z = set.start_z;
  sub3.length_x = split_x[1];
  sub3.length_y = split_y[1];
  sub3.length_z = set.length_z;

  return subsets;
}

template <typename T>
auto sperr::SPECK3D_INT<T>::m_partition_S_Z(const Set3D& set) const -> std::array<Set3D, 2>
{
  // This partition scheme is only used during initialization; no need to calculate morton offset.

  const auto split_z = std::array<int, 2>{set.length_z - set.length_z / 2, set.length_z / 2};

  std::array<Set3D, 2> subsets;
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

template class sperr::SPECK3D_INT<uint64_t>;
template class sperr::SPECK3D_INT<uint32_t>;
template class sperr::SPECK3D_INT<uint16_t>;
template class sperr::SPECK3D_INT<uint8_t>;
