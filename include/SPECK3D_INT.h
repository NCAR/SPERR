#ifndef SPECK3D_INT_H
#define SPECK3D_INT_H

#include "SPECK_INT.h"

#include <cstring>  // std::memcpy
#include <tuple>

namespace sperr {

class Set3D {
 private:
  // The first 6 bytes of the morton offset in uint64_t. Because each set dimension is
  // stored using 16-bit integers, these 48 bits are big enough too!
  std::array<uint8_t, 6> m_morton = {0, 0, 0, 0, 0, 0};

 public:
  //
  // Publicly accessible public data members.
  //
  uint16_t start_x = 0;
  uint16_t start_y = 0;
  uint16_t start_z = 0;
  uint16_t length_x = 0;
  uint16_t length_y = 0;
  uint16_t length_z = 0;

 public:
  //
  // Member functions (intended to be inline)
  //
  auto get_morton() const -> uint64_t
  {
    auto tmp = uint64_t{0};
    std::memcpy(&tmp, m_morton.data(), sizeof(m_morton));
    return tmp;
  }
  void set_morton(uint64_t val) { std::memcpy(m_morton.data(), &val, sizeof(m_morton)); }
  auto is_empty() const -> bool { return (length_z == 0 || length_y == 0 || length_x == 0); }
  void make_empty() { length_z = 0; }
  auto num_elem() const -> size_t
  {
    return (size_t{length_x} * size_t{length_y} * size_t{length_z});
  }
};

//
// Main SPECK3D_INT class; intended to be the base class of both encoder and decoder.
//
template <typename T>
class SPECK3D_INT : public SPECK_INT<T> {
 protected:
  //
  // Bring members from the base class to this derived class.
  //
  using SPECK_INT<T>::m_LIP_mask;
  using SPECK_INT<T>::m_dims;
  using SPECK_INT<T>::m_LSP_new;
  using SPECK_INT<T>::m_coeff_buf;
  using SPECK_INT<T>::m_bit_buffer;

  void m_clean_LIS() override;
  void m_initialize_lists() override;

  // Divide a Set3D into 8, 4, or 2 smaller subsets.
  auto m_partition_S_XYZ(Set3D, uint16_t) const -> std::tuple<std::array<Set3D, 8>, uint16_t>;
  auto m_partition_S_XY(Set3D, uint16_t) const -> std::tuple<std::array<Set3D, 4>, uint16_t>;
  auto m_partition_S_Z(Set3D, uint16_t) const -> std::tuple<std::array<Set3D, 2>, uint16_t>;

  //
  // SPECK3D_INT specific data members
  //
  std::vector<std::vector<Set3D>> m_LIS;

  // Experiment with morton curves.
  bool m_morton_valid = false;
};

};  // namespace sperr

#endif
