#include "SPECK2D_INT.h"

auto sperr::Set2D::is_pixel() const -> bool
{
  return (length_x == 1 && length_y == 1);
}

auto sperr::Set2D::is_empty() const -> bool
{
  return (length_x == 0 || length_y == 0);
}

template <typename T>
void sperr::SPECK2D_INT<T>::m_clean_LIS()
{
  for (auto& list : m_LIS) {
    auto it = std::remove_if(list.begin(), list.end(), [](auto& s) { return s.type == SetType::Garbage; });
    list.erase(it, list.end());
  }
}

template <typename T>
auto sperr::SPECK2D_INT<T>::m_partition_S(Set2D set) const -> std::array<Set2D, 4>
{
  auto subsets = std::array<Set2D, 4>();

  const auto detail_len_x = set.length_x / 2;
  const auto detail_len_y = set.length_y / 2;
  const auto approx_len_x = set.length_x - detail_len_x;
  const auto approx_len_y = set.length_y - detail_len_y;

  // Put generated subsets in the list the same order as they are in QccPack.
  auto& BR = subsets[0];  // Bottom right set
  BR.part_level = set.part_level + 1;
  BR.start_x = set.start_x + approx_len_x;
  BR.start_y = set.start_y + approx_len_y;
  BR.length_x = detail_len_x;
  BR.length_y = detail_len_y;

  auto& BL = subsets[1];  // Bottom left set
  BL.part_level = set.part_level + 1;
  BL.start_x = set.start_x;
  BL.start_y = set.start_y + approx_len_y;
  BL.length_x = approx_len_x;
  BL.length_y = detail_len_y;

  auto& TR = subsets[2];  // Top right set
  TR.part_level = set.part_level + 1;
  TR.start_x = set.start_x + approx_len_x;
  TR.start_y = set.start_y;
  TR.length_x = detail_len_x;
  TR.length_y = approx_len_y;

  auto& TL = subsets[3];  // Top left set
  TL.part_level = set.part_level + 1;
  TL.start_x = set.start_x;
  TL.start_y = set.start_y;
  TL.length_x = approx_len_x;
  TL.length_y = approx_len_y;

  return subsets;
}
