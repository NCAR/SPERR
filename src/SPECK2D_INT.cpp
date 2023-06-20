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

template <typename T>
auto sperr::SPECK2D_INT<T>::m_partition_I() -> std::array<Set2D, 3>
{
  auto subsets = std::array<Set2D, 3>();
  auto [approx_len_x, detail_len_x] = sperr::calc_approx_detail_len(m_dims[0], m_I.part_level);
  auto [approx_len_y, detail_len_y] = sperr::calc_approx_detail_len(m_dims[1], m_I.part_level);

  // Specify the subsets following the same order in QccPack
  auto& BR = subsets[0];  // Bottom right
  BR.part_level = m_I.part_level;
  BR.start_x = approx_len_x;
  BR.start_y = approx_len_y;
  BR.length_x = detail_len_x;
  BR.length_y = detail_len_y;

  auto& TR = subsets[1];  // Top right
  TR.part_level = m_I.part_level;
  TR.start_x = approx_len_x;
  TR.start_y = 0;
  TR.length_x = detail_len_x;
  TR.length_y = approx_len_y;

  auto& BL = subsets[2];  // Bottom left
  BL.part_level = m_I.part_level;
  BL.start_x = 0;
  BL.start_y = approx_len_y;
  BL.length_x = approx_len_x;
  BL.length_y = detail_len_y;

  // Also update m_I
  m_I.part_level--;
  m_I.start_x += detail_len_x;
  m_I.start_y += detail_len_y;

  return subsets;
}

template <typename T>
void sperr::SPECK2D_INT<T>::m_initialize_lists()
{
#if 0
  // prepare m_LIS
  const auto num_of_parts =
      std::max(sperr::num_of_partitions(m_dims[0]), sperr::num_of_partitions(m_dims[1]));
  if (m_LIS.size() < num_of_parts + 1)
    m_LIS.resize(num_of_parts + 1);
  for (auto& list : m_LIS)
    list.clear();
  m_LIP.clear();
  m_LIP.reserve(m_coeff_buf.size() / 4);

  // prepare the root, S
  auto S = m_produce_root();
  m_LIS[S.part_level].emplace_back(S);

  // prepare m_I
  m_I.part_level = sperr::num_of_xforms(std::min(m_dims[0], m_dims[1]));
  m_I.start_x = S.length_x;
  m_I.start_y = S.length_y;
  m_I.length_x = m_dims[0];
  m_I.length_y = m_dims[1];

  // clear lists and reserve space.
  m_LSP_new.clear();
  m_LSP_new.reserve(m_coeff_buf.size() / 8);
  m_LSP_mask.reserve(m_coeff_buf.size());
  m_bit_buffer.reserve(m_coeff_buf.size());
#endif
}

template class sperr::SPECK2D_INT<uint8_t>;
template class sperr::SPECK2D_INT<uint16_t>;
template class sperr::SPECK2D_INT<uint32_t>;
template class sperr::SPECK2D_INT<uint64_t>;
