#include "CDF97_PL.h"

#include <algorithm>
#include <cassert>
#include <numeric>  // std::accumulate()
#include <type_traits>

template <typename T>
auto sperr::CDF97_PL::copy_data(const T* data, size_t len, dims_type dims) -> RTNType
{
  static_assert(std::is_floating_point<T>::value, "!! Only floating point values are supported !!");
  if (len != dims[0] * dims[1] * dims[2])
    return RTNType::WrongLength;

  m_data_buf.resize(len);
  std::copy(data, data + len, m_data_buf.begin());

  m_dims = dims;

  auto max_col = std::max(std::max(dims[0], dims[1]), dims[2]);
  if (max_col * 2 > m_col_buf.size())
    m_col_buf.resize(std::max(m_col_buf.size(), max_col) * 2);

  return RTNType::Good;
}
template auto sperr::CDF97_PL::copy_data(const float*, size_t, dims_type) -> RTNType;
template auto sperr::CDF97_PL::copy_data(const double*, size_t, dims_type) -> RTNType;
template auto sperr::CDF97_PL::copy_data(const long double*, size_t, dims_type) -> RTNType;
template auto sperr::CDF97_PL::copy_data(const _Float128*, size_t, dims_type) -> RTNType;

auto sperr::CDF97_PL::view_data() const -> const std::vector<flt_type>&
{
  return m_data_buf;
}

auto sperr::CDF97_PL::get_dims() const -> std::array<size_t, 3>
{
  return m_dims;
}

//
// Start Peter Lindstrom's wavelet scheme.
//
void sperr::CDF97_PL::dwt1d_pl(flt_type* start_p, ptrdiff_t stride)
{
  // If not specified, use the beginning of `m_data_buf`.
  if (start_p == nullptr) {
    assert(!m_data_buf.empty());
    start_p = m_data_buf.data();
  }
  auto n = m_dims[0];
  auto levels = sperr::num_of_xforms_pl(n);
  if (levels == 0)
    return;

  while (levels--)
    n = m_fwd_pl(start_p, stride, start_p, stride, n);
}

void sperr::CDF97_PL::idwt1d_pl(flt_type* start_p, ptrdiff_t stride)
{
  // If not specified, use the beginning of `m_data_buf`.
  if (start_p == nullptr) {
    assert(!m_data_buf.empty());
    start_p = m_data_buf.data();
  }
  auto levels = sperr::num_of_xforms_pl(m_dims[0]);
  if (levels == 0)
    return;

  // Compute number of scaling coefficients on each level.
  assert(levels <= 6);
  auto count = std::array<size_t, 7>{m_dims[0], 0, 0, 0, 0, 0, 0};
  for (size_t i = 1; i <= levels; i++)
    count[i] = (count[i - 1] + 6) / 2;

  while (levels--) {
    auto rtn = m_inv_pl(start_p, stride, start_p, stride, count[levels]);
    assert(rtn);
  }
}

void sperr::CDF97_PL::dwt2d_xy_pl(flt_type* start_p)
{
  // If not specified, use the beginning of `m_data_buf`.
  if (start_p == nullptr) {
    assert(!m_data_buf.empty());
    start_p = m_data_buf.data();
  }
  auto levels = sperr::num_of_xforms_pl(std::min(m_dims[0], m_dims[1]));
  auto [nx, ny] = std::array{m_dims[0], m_dims[1]};
  if (levels == 0)
    return;

  while (levels--) {
    auto [nx2, ny2] = std::array{nx, ny};
    // Transform every row.
    for (size_t i = 0; i < ny; i++) {
      auto* p = start_p + i * m_dims[0];
      nx2 = m_fwd_pl(p, 1, p, 1, nx);
    }
    // Transform every column.
    for (size_t i = 0; i < nx; i++) {
      auto* p = start_p + i;
      ny2 = m_fwd_pl(p, m_dims[0], p, m_dims[0], ny);
    }
    std::tie(nx, ny) = {nx2, ny2};
  }
}

void sperr::CDF97_PL::idwt2d_xy_pl(flt_type* start_p)
{
  // If not specified, use the beginning of `m_data_buf`.
  if (start_p == nullptr) {
    assert(!m_data_buf.empty());
    start_p = m_data_buf.data();
  }
  auto levels = sperr::num_of_xforms_pl(std::min(m_dims[0], m_dims[1]));
  if (levels == 0)
    return;

  // Compute number of scaling coefficients on each level.
  assert(levels <= 6);
  auto count_x = std::array<size_t, 7>{m_dims[0], 0, 0, 0, 0, 0, 0};
  auto count_y = std::array<size_t, 7>{m_dims[1], 0, 0, 0, 0, 0, 0};
  for (size_t i = 1; i <= levels; i++) {
    count_x[i] = (count_x[i - 1] + 6) / 2;
    count_y[i] = (count_y[i - 1] + 6) / 2;
  }

  while (levels--) {
    const auto nx = count_x[levels];
    const auto ny = count_y[levels];
    // Transform every column.
    for (size_t i = 0; i < nx; i++) {
      auto* p = start_p + i;
      auto rtn = m_inv_pl(p, m_dims[0], p, m_dims[0], ny);
      assert(rtn);
    }
    // Transform every row.
    for (size_t i = 0; i < ny; i++) {
      auto* p = start_p + i * m_dims[0];
      auto rtn = m_inv_pl(p, 1, p, 1, nx);
      assert(rtn);
    }
  }
}

void sperr::CDF97_PL::dwt3d_dyadic_pl()
{
  auto levels = sperr::num_of_xforms_pl(*std::min_element(m_dims.begin(), m_dims.end()));
  auto [nx, ny, nz] = m_dims;
  if (levels == 0)
    return;

  while (levels--) {
    auto [nx2, ny2, nz2] = std::array{nx, ny, nz};
    // Transform every row.
    for (size_t z = 0; z < nz; z++) {
      const auto offset_z = z * m_dims[0] * m_dims[1];
      for (size_t y = 0; y < ny; y++) {
        auto* p = m_data_buf.data() + offset_z + y * m_dims[0];
        nx2 = m_fwd_pl(p, 1, p, 1, nx);
      }
    }
    // Transform every column.
    for (size_t z = 0; z < nz; z++) {
      const auto offset_z = z * m_dims[0] * m_dims[1];
      for (size_t x = 0; x < nx; x++) {
        auto* p = m_data_buf.data() + offset_z + x;
        ny2 = m_fwd_pl(p, m_dims[0], p, m_dims[0], ny);
      }
    }
    // Transform every 1D array along the Z axis.
    for (size_t y = 0; y < ny; y++) {
      const auto offset_y = y * m_dims[0];
      const auto stride = m_dims[0] * m_dims[1];
      for (size_t x = 0; x < nx; x++) {
        auto* p = m_data_buf.data() + offset_y + x;
        nz2 = m_fwd_pl(p, stride, p, stride, nz);
      }
    }
    std::tie(nx, ny, nz) = {nx2, ny2, nz2};
  }
}

void sperr::CDF97_PL::idwt3d_dyadic_pl()
{
  auto levels = sperr::num_of_xforms_pl(*std::min_element(m_dims.begin(), m_dims.end()));
  if (levels == 0)
    return;

  // Compute number of scaling coefficients on each level.
  assert(levels <= 6);
  auto count_x = std::array<size_t, 7>{m_dims[0], 0, 0, 0, 0, 0, 0};
  auto count_y = std::array<size_t, 7>{m_dims[1], 0, 0, 0, 0, 0, 0};
  auto count_z = std::array<size_t, 7>{m_dims[2], 0, 0, 0, 0, 0, 0};
  for (size_t i = 1; i <= levels; i++) {
    count_x[i] = (count_x[i - 1] + 6) / 2;
    count_y[i] = (count_y[i - 1] + 6) / 2;
    count_z[i] = (count_z[i - 1] + 6) / 2;
  }

  while (levels--) {
    const auto nx = count_x[levels];
    const auto ny = count_y[levels];
    const auto nz = count_z[levels];
    // Transform every 1D array along the Z axis.
    for (size_t y = 0; y < ny; y++) {
      const auto offset_y = y * m_dims[0];
      const auto stride = m_dims[0] * m_dims[1];
      for (size_t x = 0; x < nx; x++) {
        auto* p = m_data_buf.data() + offset_y + x;
        auto rtn = m_inv_pl(p, stride, p, stride, nz);
        assert(rtn);
      }
    }
    // Transform every column.
    for (size_t z = 0; z < nz; z++) {
      const auto offset_z = z * m_dims[0] * m_dims[1];
      for (size_t x = 0; x < nx; x++) {
        auto* p = m_data_buf.data() + offset_z + x;
        auto rtn = m_inv_pl(p, m_dims[0], p, m_dims[0], ny);
        assert(rtn);
      }
    }
    // Transform every row.
    for (size_t z = 0; z < nz; z++) {
      const auto offset_z = z * m_dims[0] * m_dims[1];
      for (size_t y = 0; y < ny; y++) {
        auto* p = m_data_buf.data() + offset_z + y * m_dims[0];
        auto rtn = m_inv_pl(p, 1, p, 1, nx);
        assert(rtn);
      }
    }
  }
}

size_t sperr::CDF97_PL::m_fwd_pl(const flt_type* src,
                              ptrdiff_t sstride,
                              flt_type* dst,
                              ptrdiff_t dstride,
                              size_t n)
{
  if (n < 9)
    return 0;

  const int even = (n & 1u) ? 0 : 1;
  const size_t m = n - 1 - even;  // index of last scaling coefficient

  // copy input to scratch space
  const auto* p = src;
  auto* q = m_col_buf.data();
  for (size_t i = 0; i < n; i++) {
    q[i] = *p;
    p += sstride;
  }

  // first w-lift (predict) pass
  for (size_t i = 1; i < n - 1; i += 2)
    q[i] += m_lift[0] * (q[i - 1] + q[i + 1]);
  if (even)
    q[n - 1] += m_lift[0] * q[n - 2];

  // first s-lift (update) pass
  for (size_t i = 1; i < n - 1; i += 2) {
    auto w = m_lift[1] * q[i];
    q[i - 1] += w;
    q[i + 1] += w;
  }

  // second w-lift (predict) pass
  for (size_t i = 2; i < n - 2; i += 2) {
    auto s = m_lift[2] * q[i];
    q[i - 1] += s;
    q[i + 1] += s;
  }
  if (even)
    q[n - 1] += m_lift[2] * q[n - 2];

  // second s-lift (update) pass
  q[0] += m_blift[3] * q[1];
  for (size_t i = 3; i < n - 3; i += 2) {
    auto w = m_lift[3] * q[i];
    q[i - 1] += w;
    q[i + 1] += w;
  }
  q[m] += m_blift[3] * q[m - 1];

  // right boundary special case for even n
  if (even) {
    q[n - 1] += m_blift[6] * q[n - 3];
    q[n - 1] += m_blift[7] * q[n - 2];
  }

  // w-lift scale pass
  q[1] *= m_blift[4];
  for (size_t i = 3; i < n - 3; i += 2)
    q[i] *= m_lift[4];
  q[m - 1] *= m_blift[4];
  if (even)
    q[n - 1] *= m_blift[8];

  // s-lift scale pass
  q[0] *= m_blift[5];
  for (size_t i = 2; i < n - 2; i += 2)
    q[i] *= m_lift[5];
  q[m] *= m_blift[5];

  // copy scaling coefficients to destination
  p = q;
  q = dst;
  *q = p[0];
  q += dstride;
  *q = p[1];
  q += dstride;
  for (size_t i = 2; i < n - 2; i += 2) {
    *q = p[i];
    q += dstride;
  }
  if (even) {
    *q = p[n - 3];
    q += dstride;
  }
  *q = p[n - 2];
  q += dstride;
  *q = p[n - 1];
  q += dstride;

  // append wavelet coefficients to destination
  for (size_t i = 3; i < n - 3; i += 2) {
    *q = p[i];
    q += dstride;
  }

  return (n + 6) / 2;
}

bool sperr::CDF97_PL::m_inv_pl(const flt_type* src,
                            ptrdiff_t sstride,
                            flt_type* dst,
                            ptrdiff_t dstride,
                            size_t n)
{
  if (n < 9)
    return false;

  const int even = n & 1u ? 0 : 1;
  const size_t m = n - 1 - even;  // index of last scaling coefficient

  // copy scaling coefficients from source
  const auto* p = src;
  auto* q = m_col_buf.data();
  q[0] = *p;
  p += sstride;
  q[1] = *p;
  p += sstride;
  for (size_t i = 2; i < n - 2; i += 2) {
    q[i] = *p;
    p += sstride;
  }
  if (even) {
    q[n - 3] = *p;
    p += sstride;
  }
  q[n - 2] = *p;
  p += sstride;
  q[n - 1] = *p;
  p += sstride;

  // copy wavelet coefficients from source
  for (size_t i = 3; i < n - 3; i += 2) {
    q[i] = *p;
    p += sstride;
  }

  // s-lift scale pass
  q[0] /= m_blift[5];
  for (size_t i = 2; i < n - 2; i += 2)
    q[i] /= m_lift[5];
  q[m] /= m_blift[5];

  // w-lift scale pass
  q[1] /= m_blift[4];
  for (size_t i = 3; i < n - 3; i += 2)
    q[i] /= m_lift[4];
  q[m - 1] /= m_blift[4];
  if (even)
    q[n - 1] /= m_blift[8];

  // right boundary special case for even n
  if (even) {
    q[n - 1] -= m_blift[6] * q[n - 3];
    q[n - 1] -= m_blift[7] * q[n - 2];
  }

  // second s-lift (update) pass
  q[0] -= m_blift[3] * q[1];
  for (size_t i = 3; i < n - 3; i += 2) {
    auto w = m_lift[3] * q[i];
    q[i - 1] -= w;
    q[i + 1] -= w;
  }
  q[m] -= m_blift[3] * q[m - 1];

  // second w-lift (predict) pass
  for (size_t i = 2; i < n - 2; i += 2) {
    auto s = m_lift[2] * q[i];
    q[i - 1] -= s;
    q[i + 1] -= s;
  }
  if (even)
    q[n - 1] -= m_lift[2] * q[n - 2];

  // first s-lift (update) pass
  for (size_t i = 1; i < n - 1; i += 2) {
    auto w = m_lift[1] * q[i];
    q[i - 1] -= w;
    q[i + 1] -= w;
  }

  // first w-lift (predict) pass
  for (size_t i = 1; i < n - 1; i += 2)
    q[i] -= m_lift[0] * (q[i - 1] + q[i + 1]);
  if (even)
    q[n - 1] -= m_lift[0] * q[n - 2];

  // copy values to destination
  p = q;
  q = dst;
  for (size_t i = 0; i < n; i++) {
    *q = p[i];
    q += dstride;
  }

  return true;
}
