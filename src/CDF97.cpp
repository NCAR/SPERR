#include "CDF97.h"

#include <algorithm>
#include <cassert>
#include <numeric>  // std::accumulate()
#include <type_traits>

template <typename T>
auto sperr::CDF97::copy_data(const T* data, size_t len, dims_type dims) -> RTNType
{
  static_assert(std::is_floating_point<T>::value, "!! Only floating point values are supported !!");
  if (len != dims[0] * dims[1] * dims[2])
    return RTNType::WrongDims;

  m_data_buf.resize(len);
  std::copy(data, data + len, m_data_buf.begin());

  m_dims = dims;

  auto max_col = std::max(std::max(dims[0], dims[1]), dims[2]);
  if (max_col * 2 > m_qcc_buf.size())
    m_qcc_buf.resize(std::max(m_qcc_buf.size(), max_col) * 2);

  auto max_slice = std::max(std::max(dims[0] * dims[1], dims[0] * dims[2]), dims[1] * dims[2]);
  if (max_slice > m_slice_buf.size())
    m_slice_buf.resize(std::max(m_slice_buf.size() * 2, max_slice));

  return RTNType::Good;
}
template auto sperr::CDF97::copy_data(const float*, size_t, dims_type) -> RTNType;
template auto sperr::CDF97::copy_data(const double*, size_t, dims_type) -> RTNType;

auto sperr::CDF97::take_data(vecd_type&& buf, dims_type dims) -> RTNType
{
  if (buf.size() != dims[0] * dims[1] * dims[2])
    return RTNType::WrongDims;

  m_data_buf = std::move(buf);
  m_dims = dims;

  auto max_col = std::max(std::max(dims[0], dims[1]), dims[2]);
  if (max_col * 2 > m_qcc_buf.size())
    m_qcc_buf.resize(std::max(m_qcc_buf.size(), max_col) * 2);

  auto max_slice = std::max(std::max(dims[0] * dims[1], dims[0] * dims[2]), dims[1] * dims[2]);
  if (max_slice > m_slice_buf.size())
    m_slice_buf.resize(std::max(m_slice_buf.size() * 2, max_slice));

  return RTNType::Good;
}

auto sperr::CDF97::view_data() const -> const vecd_type&
{
  return m_data_buf;
}

auto sperr::CDF97::release_data() -> vecd_type&&
{
  m_dims = {0, 0, 0};
  return std::move(m_data_buf);
}

auto sperr::CDF97::get_dims() const -> std::array<size_t, 3>
{
  return m_dims;
}

void sperr::CDF97::dwt1d()
{
  size_t num_xforms = sperr::num_of_xforms(m_dims[0]);
  m_dwt1d(m_data_buf.begin(), m_data_buf.size(), num_xforms);
}

void sperr::CDF97::idwt1d()
{
  size_t num_xforms = sperr::num_of_xforms(m_dims[0]);
  m_idwt1d(m_data_buf.begin(), m_data_buf.size(), num_xforms);
}

void sperr::CDF97::dwt2d()
{
  size_t num_xforms_xy = sperr::num_of_xforms(std::min(m_dims[0], m_dims[1]));
  m_dwt2d(m_data_buf.begin(), {m_dims[0], m_dims[1]}, num_xforms_xy);
}

void sperr::CDF97::idwt2d()
{
  size_t num_xforms_xy = sperr::num_of_xforms(std::min(m_dims[0], m_dims[1]));
  m_idwt2d(m_data_buf.begin(), {m_dims[0], m_dims[1]}, num_xforms_xy);
}

void sperr::CDF97::dwt3d()
{
  // Strategy to choose "dyadic" or "wavelet packet" 3D transform:
  // 1) IF all 3 axes have 5 or more levels of transforms, THEN use dyadic;
  // 2) ELSE IF XY and Z have different levels, THEN use wavelet packets;
  // 3) ELSE use dyadic.
  //
  // clang-format off
  const auto num_xforms = std::array<size_t, 3>{sperr::num_of_xforms(m_dims[0]),
                                                sperr::num_of_xforms(m_dims[1]),
                                                sperr::num_of_xforms(m_dims[2])};
  // clang-format on

  if (num_xforms[0] >= 5 && num_xforms[1] >= 5 && num_xforms[2] >= 5) {
    m_dwt3d_dyadic();
  }
  else if (std::min(num_xforms[0], num_xforms[1]) != num_xforms[2]) {
    m_dwt3d_wavelet_packet();
  }
  else {
    m_dwt3d_dyadic();
  }
}

void sperr::CDF97::idwt3d()
{
  // clang-format off
  const auto num_xforms = std::array<size_t, 3>{sperr::num_of_xforms(m_dims[0]),
                                                sperr::num_of_xforms(m_dims[1]),
                                                sperr::num_of_xforms(m_dims[2])};
  // clang-format on

  if (num_xforms[0] >= 5 && num_xforms[1] >= 5 && num_xforms[2] >= 5) {
    m_idwt3d_dyadic();
  }
  else if (std::min(num_xforms[0], num_xforms[1]) != num_xforms[2]) {
    m_idwt3d_wavelet_packet();
  }
  else {
    m_idwt3d_dyadic();
  }
}

void sperr::CDF97::m_dwt3d_wavelet_packet()
{
  /*
   *             Z
   *            /
   *           /
   *          /________
   *         /       /|
   *        /       / |
   *     0 |-------/-------> X
   *       |       |  |
   *       |       |  /
   *       |       | /
   *       |_______|/
   *       |
   *       |
   *       Y
   */

  const size_t plane_size_xy = m_dims[0] * m_dims[1];

  // First transform along the Z dimension
  //
  const auto num_xforms_z = sperr::num_of_xforms(m_dims[2]);

  for (size_t y = 0; y < m_dims[1]; y++) {
    const auto y_offset = y * m_dims[0];

    // Re-arrange values of one XZ slice so that they form many z_columns
    for (size_t z = 0; z < m_dims[2]; z++) {
      const auto cube_start_idx = z * plane_size_xy + y_offset;
      for (size_t x = 0; x < m_dims[0]; x++)
        m_slice_buf[z + x * m_dims[2]] = m_data_buf[cube_start_idx + x];
    }

    // DWT1D on every z_column
    for (size_t x = 0; x < m_dims[0]; x++)
      m_dwt1d(m_slice_buf.begin() + x * m_dims[2], m_dims[2], num_xforms_z);

    // Put back values of the z_columns to the cube
    for (size_t z = 0; z < m_dims[2]; z++) {
      const auto cube_start_idx = z * plane_size_xy + y_offset;
      for (size_t x = 0; x < m_dims[0]; x++)
        m_data_buf[cube_start_idx + x] = m_slice_buf[z + x * m_dims[2]];
    }
  }

  // Second transform each plane
  //
  const auto num_xforms_xy = sperr::num_of_xforms(std::min(m_dims[0], m_dims[1]));

  for (size_t z = 0; z < m_dims[2]; z++) {
    const size_t offset = plane_size_xy * z;
    m_dwt2d(m_data_buf.begin() + offset, {m_dims[0], m_dims[1]}, num_xforms_xy);
  }
}

void sperr::CDF97::m_idwt3d_wavelet_packet()
{
  const size_t plane_size_xy = m_dims[0] * m_dims[1];

  // First, inverse transform each plane
  //
  auto num_xforms_xy = sperr::num_of_xforms(std::min(m_dims[0], m_dims[1]));

  for (size_t i = 0; i < m_dims[2]; i++) {
    const size_t offset = plane_size_xy * i;
    m_idwt2d(m_data_buf.begin() + offset, {m_dims[0], m_dims[1]}, num_xforms_xy);
  }

  /*
   * Second, inverse transform along the Z dimension
   *
   *             Z
   *            /
   *           /
   *          /________
   *         /       /|
   *        /       / |
   *     0 |-------/-------> X
   *       |       |  |
   *       |       |  /
   *       |       | /
   *       |_______|/
   *       |
   *       |
   *       Y
   */

  // Process one XZ slice at a time
  //
  const auto num_xforms_z = sperr::num_of_xforms(m_dims[2]);

  for (size_t y = 0; y < m_dims[1]; y++) {
    const auto y_offset = y * m_dims[0];

    // Re-arrange values on one slice so that they form many z_columns
    for (size_t z = 0; z < m_dims[2]; z++) {
      const auto cube_start_idx = z * plane_size_xy + y_offset;
      for (size_t x = 0; x < m_dims[0]; x++)
        m_slice_buf[z + x * m_dims[2]] = m_data_buf[cube_start_idx + x];
    }

    // IDWT1D on every z_column
    for (size_t x = 0; x < m_dims[0]; x++)
      m_idwt1d(m_slice_buf.begin() + x * m_dims[2], m_dims[2], num_xforms_z);

    // Put back values from the z_columns to the cube
    for (size_t z = 0; z < m_dims[2]; z++) {
      const auto cube_start_idx = z * plane_size_xy + y_offset;
      for (size_t x = 0; x < m_dims[0]; x++)
        m_data_buf[cube_start_idx + x] = m_slice_buf[z + x * m_dims[2]];
    }
  }
}

void sperr::CDF97::m_dwt3d_dyadic()
{
  // clang-format off
  const auto xforms = std::array<size_t, 3>{sperr::num_of_xforms(m_dims[0]),
                                            sperr::num_of_xforms(m_dims[1]),
                                            sperr::num_of_xforms(m_dims[2])};
  const auto num_xforms = *std::min_element(xforms.cbegin(), xforms.cend());
  // clang-format on

  for (size_t lev = 0; lev < num_xforms; lev++) {
    auto app_x = sperr::calc_approx_detail_len(m_dims[0], lev);
    auto app_y = sperr::calc_approx_detail_len(m_dims[1], lev);
    auto app_z = sperr::calc_approx_detail_len(m_dims[2], lev);
    m_dwt3d_one_level(m_data_buf.begin(), {app_x[0], app_y[0], app_z[0]});
  }
}

void sperr::CDF97::m_idwt3d_dyadic()
{
  // clang-format off
  const auto xforms = std::array<size_t, 3>{sperr::num_of_xforms(m_dims[0]),
                                            sperr::num_of_xforms(m_dims[1]),
                                            sperr::num_of_xforms(m_dims[2])};
  const auto num_xforms = *std::min_element(xforms.cbegin(), xforms.cend());
  // clang-format on

  for (size_t lev = num_xforms; lev > 0; lev--) {
    auto app_x = sperr::calc_approx_detail_len(m_dims[0], lev - 1);
    auto app_y = sperr::calc_approx_detail_len(m_dims[1], lev - 1);
    auto app_z = sperr::calc_approx_detail_len(m_dims[2], lev - 1);
    m_idwt3d_one_level(m_data_buf.begin(), {app_x[0], app_y[0], app_z[0]});
  }
}

//
// Private Methods
//

void sperr::CDF97::m_dwt1d(itd_type array, size_t array_len, size_t num_of_lev)
{
  for (size_t lev = 0; lev < num_of_lev; lev++) {
    auto [apx, nnm] = sperr::calc_approx_detail_len(array_len, lev);
    m_dwt1d_one_level(array, apx);
  }
}

void sperr::CDF97::m_idwt1d(itd_type array, size_t array_len, size_t num_of_lev)
{
  for (size_t lev = num_of_lev; lev > 0; lev--) {
    auto [apx, nnm] = sperr::calc_approx_detail_len(array_len, lev - 1);
    m_idwt1d_one_level(array, apx);
  }
}

void sperr::CDF97::m_dwt2d(itd_type plane, std::array<size_t, 2> len_xy, size_t num_of_lev)
{
  for (size_t lev = 0; lev < num_of_lev; lev++) {
    auto approx_x = sperr::calc_approx_detail_len(len_xy[0], lev);
    auto approx_y = sperr::calc_approx_detail_len(len_xy[1], lev);
    m_dwt2d_one_level(plane, {approx_x[0], approx_y[0]});
  }
}

void sperr::CDF97::m_idwt2d(itd_type plane, std::array<size_t, 2> len_xy, size_t num_of_lev)
{
  for (size_t lev = num_of_lev; lev > 0; lev--) {
    auto approx_x = sperr::calc_approx_detail_len(len_xy[0], lev - 1);
    auto approx_y = sperr::calc_approx_detail_len(len_xy[1], lev - 1);
    m_idwt2d_one_level(plane, {approx_x[0], approx_y[0]});
  }
}

void sperr::CDF97::m_dwt1d_one_level(itd_type array, size_t array_len)
{
  std::copy(array, array + array_len, m_qcc_buf.begin());
#if __cplusplus >= 202002L
  if (array_len % 2 == 0) [[likely]]  // Even length
#else
  if (array_len % 2 == 0)   // Even length
#endif
  {
    this->QccWAVCDF97AnalysisSymmetricEvenEven(m_qcc_buf.data(), array_len);
    m_gather_even(m_qcc_buf.begin(), m_qcc_buf.begin() + array_len, array);
  }
  else {
    this->QccWAVCDF97AnalysisSymmetricOddEven(m_qcc_buf.data(), array_len);
    m_gather_odd(m_qcc_buf.begin(), m_qcc_buf.begin() + array_len, array);
  }
}

void sperr::CDF97::m_idwt1d_one_level(itd_type array, size_t array_len)
{
#if __cplusplus >= 202002L
  if (array_len % 2 == 0) [[likely]]  // Even length
#else
  if (array_len % 2 == 0)   // Even length
#endif
  {
    m_scatter_even(array, array + array_len, m_qcc_buf.begin());
    this->QccWAVCDF97SynthesisSymmetricEvenEven(m_qcc_buf.data(), array_len);
  }
  else {
    m_scatter_odd(array, array + array_len, m_qcc_buf.begin());
    this->QccWAVCDF97SynthesisSymmetricOddEven(m_qcc_buf.data(), array_len);
  }
  std::copy(m_qcc_buf.begin(), m_qcc_buf.begin() + array_len, array);
}

void sperr::CDF97::m_dwt2d_one_level(itd_type plane, std::array<size_t, 2> len_xy)
{
  // Note: here we call low-level functions (Qcc*()) instead of
  // m_dwt1d_one_level() because we want to have only one even/odd test outside of
  // the loop.

  const size_t max_len = std::max(len_xy[0], len_xy[1]);
  const auto beg = m_qcc_buf.begin();
  const auto beg2 = beg + max_len;

  // First, perform DWT along X for every row
#if __cplusplus >= 202002L
  if (len_xy[0] % 2 == 0) [[likely]]  // Even Length
#else
  if (len_xy[0] % 2 == 0)   // Even Length
#endif
  {
    for (size_t i = 0; i < len_xy[1]; i++) {
      auto pos = plane + i * m_dims[0];
      std::copy(pos, pos + len_xy[0], beg);
      this->QccWAVCDF97AnalysisSymmetricEvenEven(m_qcc_buf.data(), len_xy[0]);
      m_gather_even(beg, beg + len_xy[0], pos);
    }
  }
  else  // Odd length
  {
    for (size_t i = 0; i < len_xy[1]; i++) {
      auto pos = plane + i * m_dims[0];
      std::copy(pos, pos + len_xy[0], beg);
      this->QccWAVCDF97AnalysisSymmetricOddEven(m_qcc_buf.data(), len_xy[0]);
      m_gather_odd(beg, beg + len_xy[0], pos);
    }
  }

  // Second, perform DWT along Y for every column
  // Note, I've tested that up to 1024^2 planes it is actually slightly slower
  // to transpose the plane and then perform the transforms. This was consistent
  // on both a MacBook and a RaspberryPi 3. Note2, I've tested transpose again
  // on an X86 linux machine using gcc, clang, and pgi. Again the difference is
  // either indistinguishable, or the current implementation has a slight edge.

#if __cplusplus >= 202002L
  if (len_xy[1] % 2 == 0) [[likely]]  // Even length
#else
  if (len_xy[1] % 2 == 0)   // Even length
#endif
  {
    for (size_t x = 0; x < len_xy[0]; x++) {
      for (size_t y = 0; y < len_xy[1]; y++)
        m_qcc_buf[y] = *(plane + y * m_dims[0] + x);
      this->QccWAVCDF97AnalysisSymmetricEvenEven(m_qcc_buf.data(), len_xy[1]);
      m_gather_even(beg, beg + len_xy[1], beg2);
      for (size_t y = 0; y < len_xy[1]; y++)
        *(plane + y * m_dims[0] + x) = *(beg2 + y);
    }
  }
  else  // Odd length
  {
    for (size_t x = 0; x < len_xy[0]; x++) {
      for (size_t y = 0; y < len_xy[1]; y++)
        m_qcc_buf[y] = *(plane + y * m_dims[0] + x);
      this->QccWAVCDF97AnalysisSymmetricOddEven(m_qcc_buf.data(), len_xy[1]);
      m_gather_odd(beg, beg + len_xy[1], beg2);
      for (size_t y = 0; y < len_xy[1]; y++)
        *(plane + y * m_dims[0] + x) = *(beg2 + y);
    }
  }
}

void sperr::CDF97::m_idwt2d_one_level(itd_type plane, std::array<size_t, 2> len_xy)
{
  const size_t max_len = std::max(len_xy[0], len_xy[1]);
  const auto beg = m_qcc_buf.begin();  // First half of the buffer
  const auto beg2 = beg + max_len;     // Second half of the buffer

  // First, perform IDWT along Y for every column
#if __cplusplus >= 202002L
  if (len_xy[1] % 2 == 0) [[likely]]  // Even length
#else
  if (len_xy[1] % 2 == 0)   // Even length
#endif
  {
    for (size_t x = 0; x < len_xy[0]; x++) {
      for (size_t y = 0; y < len_xy[1]; y++)
        m_qcc_buf[y] = *(plane + y * m_dims[0] + x);
      m_scatter_even(beg, beg + len_xy[1], beg2);
      this->QccWAVCDF97SynthesisSymmetricEvenEven(m_qcc_buf.data() + max_len, len_xy[1]);
      for (size_t y = 0; y < len_xy[1]; y++)
        *(plane + y * m_dims[0] + x) = *(beg2 + y);
    }
  }
  else  // Odd length
  {
    for (size_t x = 0; x < len_xy[0]; x++) {
      for (size_t y = 0; y < len_xy[1]; y++)
        m_qcc_buf[y] = *(plane + y * m_dims[0] + x);
      m_scatter_odd(beg, beg + len_xy[1], beg2);
      this->QccWAVCDF97SynthesisSymmetricOddEven(m_qcc_buf.data() + max_len, len_xy[1]);
      for (size_t y = 0; y < len_xy[1]; y++)
        *(plane + y * m_dims[0] + x) = *(beg2 + y);
    }
  }

  // Second, perform IDWT along X for every row
#if __cplusplus >= 202002L
  if (len_xy[0] % 2 == 0) [[likely]]  // Even length
#else
  if (len_xy[0] % 2 == 0)   // Even length
#endif
  {
    for (size_t i = 0; i < len_xy[1]; i++) {
      auto pos = plane + i * m_dims[0];
      m_scatter_even(pos, pos + len_xy[0], beg);
      this->QccWAVCDF97SynthesisSymmetricEvenEven(m_qcc_buf.data(), len_xy[0]);
      std::copy(beg, beg + len_xy[0], pos);
    }
  }
  else  // Odd length
  {
    for (size_t i = 0; i < len_xy[1]; i++) {
      auto pos = plane + i * m_dims[0];
      m_scatter_odd(pos, pos + len_xy[0], beg);
      this->QccWAVCDF97SynthesisSymmetricOddEven(m_qcc_buf.data(), len_xy[0]);
      std::copy(beg, beg + len_xy[0], pos);
    }
  }
}

void sperr::CDF97::m_dwt3d_one_level(itd_type vol, std::array<size_t, 3> len_xyz)
{
  // First, do one level of transform on all XY planes.
  const size_t plane_size_xy = m_dims[0] * m_dims[1];
  for (size_t z = 0; z < len_xyz[2]; z++) {
    const size_t offset = plane_size_xy * z;
    m_dwt2d_one_level(vol + offset, {len_xyz[0], len_xyz[1]});
  }

  const auto beg = m_qcc_buf.begin();  // First half of the buffer
  const auto beg2 = beg + len_xyz[2];  // Second half of the buffer

  // Second, do one level of transform on all Z columns.  Strategy:
  // 1) extract a Z column to buffer space `m_qcc_buf`
  // 2) use appropriate even/odd Qcc*** function to transform it
  // 3) gather coefficients from `m_qcc_buf` to the second half of `m_qcc_buf`
  // 4) put the Z column back to their locations as a Z column.

#if __cplusplus >= 202002L
  if (len_xyz[2] % 2 == 0) [[likely]]  // Even length
#else
  if (len_xyz[2] % 2 == 0)  // Even length
#endif
  {
    for (size_t y = 0; y < len_xyz[1]; y++) {
      for (size_t x = 0; x < len_xyz[0]; x++) {
        const size_t xy_offset = y * m_dims[0] + x;
        // Step 1
        for (size_t z = 0; z < len_xyz[2]; z++)
          m_qcc_buf[z] = m_data_buf[z * plane_size_xy + xy_offset];
        // Step 2
        this->QccWAVCDF97AnalysisSymmetricEvenEven(m_qcc_buf.data(), len_xyz[2]);
        // Step 3
        m_gather_even(beg, beg2, beg2);
        // Step 4
        for (size_t z = 0; z < len_xyz[2]; z++)
          m_data_buf[z * plane_size_xy + xy_offset] = *(beg2 + z);
      }
    }
  }
  else  // Odd length
  {
    for (size_t y = 0; y < len_xyz[1]; y++) {
      for (size_t x = 0; x < len_xyz[0]; x++) {
        const size_t xy_offset = y * m_dims[0] + x;
        // Step 1
        for (size_t z = 0; z < len_xyz[2]; z++)
          m_qcc_buf[z] = m_data_buf[z * plane_size_xy + xy_offset];
        // Step 2
        this->QccWAVCDF97AnalysisSymmetricOddEven(m_qcc_buf.data(), len_xyz[2]);
        // Step 3
        m_gather_odd(beg, beg2, beg2);
        // Step 4
        for (size_t z = 0; z < len_xyz[2]; z++)
          m_data_buf[z * plane_size_xy + xy_offset] = *(beg2 + z);
      }
    }
  }
}

void sperr::CDF97::m_idwt3d_one_level(itd_type vol, std::array<size_t, 3> len_xyz)
{
  const size_t plane_size_xy = m_dims[0] * m_dims[1];
  const auto beg = m_qcc_buf.begin();  // First half of the buffer
  const auto beg2 = beg + len_xyz[2];  // Second half of the buffer

  // First, do one level of inverse transform on all Z columns.  Strategy:
  // 1) extract a Z column to buffer space `m_qcc_buf`
  // 2) scatter coefficients from `m_qcc_buf` to the second half of `m_qcc_buf`
  // 3) use appropriate even/odd Qcc*** function to transform it
  // 4) put the Z column back to their locations as a Z column.

#if __cplusplus >= 202002L
  if (len_xyz[2] % 2 == 0) [[likely]]
#else
  if (len_xyz[2] % 2 == 0)
#endif
  {
    for (size_t y = 0; y < len_xyz[1]; y++) {
      for (size_t x = 0; x < len_xyz[0]; x++) {
        const size_t xy_offset = y * m_dims[0] + x;
        // Step 1
        for (size_t z = 0; z < len_xyz[2]; z++)
          m_qcc_buf[z] = m_data_buf[z * plane_size_xy + xy_offset];
        // Step 2
        m_scatter_even(beg, beg2, beg2);
        // Step 3
        this->QccWAVCDF97SynthesisSymmetricEvenEven(m_qcc_buf.data() + len_xyz[2], len_xyz[2]);
        // Step 4
        for (size_t z = 0; z < len_xyz[2]; z++)
          m_data_buf[z * plane_size_xy + xy_offset] = *(beg2 + z);
      }
    }
  }
  else  // Odd length
  {
    for (size_t y = 0; y < len_xyz[1]; y++) {
      for (size_t x = 0; x < len_xyz[0]; x++) {
        const size_t xy_offset = y * m_dims[0] + x;
        // Step 1
        for (size_t z = 0; z < len_xyz[2]; z++)
          m_qcc_buf[z] = m_data_buf[z * plane_size_xy + xy_offset];
        // Step 2
        m_scatter_odd(beg, beg2, beg2);
        // Step 3
        this->QccWAVCDF97SynthesisSymmetricOddEven(m_qcc_buf.data() + len_xyz[2], len_xyz[2]);
        // Step 4
        for (size_t z = 0; z < len_xyz[2]; z++)
          m_data_buf[z * plane_size_xy + xy_offset] = *(beg2 + z);
      }
    }
  }

  // Second, do one level of inverse transform on all XY planes.
  for (size_t z = 0; z < len_xyz[2]; z++) {
    const size_t offset = plane_size_xy * z;
    m_idwt2d_one_level(vol + offset, {len_xyz[0], len_xyz[1]});
  }
}

void sperr::CDF97::m_gather_even(citd_type begin, citd_type end, itd_type dest) const
{
  auto len = end - begin;
  assert(len % 2 == 0);  // This function specifically for even length input
  size_t low_count = len / 2, high_count = len / 2;
  for (size_t i = 0; i < low_count; i++) {
    *dest = *(begin + i * 2);
    ++dest;
  }
  for (size_t i = 0; i < high_count; i++) {
    *dest = *(begin + i * 2 + 1);
    ++dest;
  }
}

void sperr::CDF97::m_gather_odd(citd_type begin, citd_type end, itd_type dest) const
{
  auto len = end - begin;
  assert(len % 2 == 1);  // This function specifically for odd length input
  size_t low_count = len / 2 + 1, high_count = len / 2;
  for (size_t i = 0; i < low_count; i++) {
    *dest = *(begin + i * 2);
    ++dest;
  }
  for (size_t i = 0; i < high_count; i++) {
    *dest = *(begin + i * 2 + 1);
    ++dest;
  }
}

void sperr::CDF97::m_scatter_even(citd_type begin, citd_type end, itd_type dest) const
{
  auto len = end - begin;
  assert(len % 2 == 0);  // This function specifically for even length input
  size_t low_count = len / 2, high_count = len / 2;
  for (size_t i = 0; i < low_count; i++) {
    *(dest + i * 2) = *begin;
    ++begin;
  }
  for (size_t i = 0; i < high_count; i++) {
    *(dest + i * 2 + 1) = *begin;
    ++begin;
  }
}

void sperr::CDF97::m_scatter_odd(citd_type begin, citd_type end, itd_type dest) const
{
  auto len = end - begin;
  assert(len % 2 == 1);  // This function specifically for odd length input
  size_t low_count = len / 2 + 1, high_count = len / 2;
  for (size_t i = 0; i < low_count; i++) {
    *(dest + i * 2) = *begin;
    ++begin;
  }
  for (size_t i = 0; i < high_count; i++) {
    *(dest + i * 2 + 1) = *begin;
    ++begin;
  }
}

//
// Methods from QccPack
//
void sperr::CDF97::QccWAVCDF97AnalysisSymmetricEvenEven(double* signal, size_t signal_length)
{
  for (size_t index = 1; index < signal_length - 2; index += 2)
    signal[index] += ALPHA * (signal[index - 1] + signal[index + 1]);
  signal[signal_length - 1] += 2.0 * ALPHA * signal[signal_length - 2];

  signal[0] += 2.0 * BETA * signal[1];
  for (size_t index = 2; index < signal_length; index += 2)
    signal[index] += BETA * (signal[index + 1] + signal[index - 1]);

  for (size_t index = 1; index < signal_length - 2; index += 2)
    signal[index] += GAMMA * (signal[index - 1] + signal[index + 1]);
  signal[signal_length - 1] += 2.0 * GAMMA * signal[signal_length - 2];

  signal[0] = EPSILON * (signal[0] + 2.0 * DELTA * signal[1]);
  for (size_t index = 2; index < signal_length; index += 2)
    signal[index] = EPSILON * (signal[index] + DELTA * (signal[index + 1] + signal[index - 1]));

  for (size_t index = 1; index < signal_length; index += 2)
    signal[index] *= -INV_EPSILON;
}

void sperr::CDF97::QccWAVCDF97SynthesisSymmetricEvenEven(double* signal, size_t signal_length)
{
  for (size_t index = 1; index < signal_length; index += 2)
    signal[index] *= (-EPSILON);

  signal[0] = signal[0] * INV_EPSILON - 2.0 * DELTA * signal[1];

  for (size_t index = 2; index < signal_length; index += 2)
    signal[index] = signal[index] * INV_EPSILON - DELTA * (signal[index + 1] + signal[index - 1]);

  for (size_t index = 1; index < signal_length - 2; index += 2)
    signal[index] -= GAMMA * (signal[index - 1] + signal[index + 1]);
  signal[signal_length - 1] -= 2.0 * GAMMA * signal[signal_length - 2];

  signal[0] -= 2.0 * BETA * signal[1];
  for (size_t index = 2; index < signal_length; index += 2)
    signal[index] -= BETA * (signal[index + 1] + signal[index - 1]);

  for (size_t index = 1; index < signal_length - 2; index += 2)
    signal[index] -= ALPHA * (signal[index - 1] + signal[index + 1]);
  signal[signal_length - 1] -= 2.0 * ALPHA * signal[signal_length - 2];
}

void sperr::CDF97::QccWAVCDF97SynthesisSymmetricOddEven(double* signal, size_t signal_length)
{
  for (size_t index = 1; index < signal_length - 1; index += 2)
    signal[index] *= (-EPSILON);

  signal[0] = signal[0] * INV_EPSILON - 2.0 * DELTA * signal[1];

  for (size_t index = 2; index < signal_length - 2; index += 2)
    signal[index] = signal[index] * INV_EPSILON - DELTA * (signal[index + 1] + signal[index - 1]);

  signal[signal_length - 1] =
      signal[signal_length - 1] * INV_EPSILON - 2.0 * DELTA * signal[signal_length - 2];

  for (size_t index = 1; index < signal_length - 1; index += 2)
    signal[index] -= GAMMA * (signal[index - 1] + signal[index + 1]);

  signal[0] -= 2.0 * BETA * signal[1];

  for (size_t index = 2; index < signal_length - 2; index += 2)
    signal[index] -= BETA * (signal[index + 1] + signal[index - 1]);

  signal[signal_length - 1] -= 2.0 * BETA * signal[signal_length - 2];

  for (size_t index = 1; index < signal_length - 1; index += 2)
    signal[index] -= ALPHA * (signal[index - 1] + signal[index + 1]);
}

void sperr::CDF97::QccWAVCDF97AnalysisSymmetricOddEven(double* signal, size_t signal_length)
{
  for (size_t index = 1; index < signal_length - 1; index += 2)
    signal[index] += ALPHA * (signal[index - 1] + signal[index + 1]);

  signal[0] += 2.0 * BETA * signal[1];

  for (size_t index = 2; index < signal_length - 2; index += 2)
    signal[index] += BETA * (signal[index + 1] + signal[index - 1]);

  signal[signal_length - 1] += 2.0 * BETA * signal[signal_length - 2];

  for (size_t index = 1; index < signal_length - 1; index += 2)
    signal[index] += GAMMA * (signal[index - 1] + signal[index + 1]);

  signal[0] = EPSILON * (signal[0] + 2.0 * DELTA * signal[1]);

  for (size_t index = 2; index < signal_length - 2; index += 2)
    signal[index] = EPSILON * (signal[index] + DELTA * (signal[index + 1] + signal[index - 1]));

  signal[signal_length - 1] =
      EPSILON * (signal[signal_length - 1] + 2.0 * DELTA * signal[signal_length - 2]);

  for (size_t index = 1; index < signal_length - 1; index += 2)
    signal[index] *= (-INV_EPSILON);
}
