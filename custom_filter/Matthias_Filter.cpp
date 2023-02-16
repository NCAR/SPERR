#include "Matthias_Filter.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstring>
#include <numeric>

auto sperr::Matthias_Filter::apply_filter(vecd_type& buf, dims_type dims) -> vec8_type
{
  assert(buf.size() == dims[0] * dims[1] * dims[2]);
  assert(!buf.empty());

  auto meta = std::vector<double>(dims[0] * 2);

  for (size_t x = 0; x < dims[0]; x++) {
    m_extract_YZ_slice(buf, dims, x);

    // Operation 1: subtract mean
    auto mean = std::accumulate(m_slice_buf.cbegin(), m_slice_buf.cend(), 0.0);
    mean /= double(m_slice_buf.size());
    std::for_each(m_slice_buf.begin(), m_slice_buf.end(), [mean](auto& v) { v -= mean; });

    // Operation 2: divide by RMS
    auto rms = m_calc_RMS();
    if (rms == 0.0)
      rms = 1.0;
    std::for_each(m_slice_buf.begin(), m_slice_buf.end(), [rms](auto& v) { v /= rms; });

    m_restore_YZ_slice(buf, dims, x);

    // Save meta data
    meta[x * 2] = mean;
    meta[x * 2 + 1] = rms;
  }

  // Filter header definition:
  // Length (uint32_t) +  pairs of mean and rms (double + double)
  //
  const uint32_t len = sizeof(uint32_t) + sizeof(double) * 2 * dims[0];
  auto header = vec8_type(len);
  std::memcpy(header.data(), &len, sizeof(len));
  std::memcpy(header.data() + sizeof(len), meta.data(), sizeof(double) * meta.size());

  return header;
}

auto sperr::Matthias_Filter::inverse_filter(vecd_type& buf,
                                            dims_type dims,
                                            const void* header,
                                            size_t header_len) -> bool
{
  // Sanity check on the length
  const auto len = this->header_size(header);
  if (len != header_len)
    return false;
  if (len != sizeof(uint32_t) + sizeof(double) * 2 * dims[0])
    return false;

  const double* const meta_p =
      reinterpret_cast<const double*>(static_cast<const uint8_t*>(header) + 4);

  for (size_t x = 0; x < dims[0]; x++) {
    const auto mean = meta_p[x * 2];
    const auto rms = meta_p[x * 2 + 1];
    m_extract_YZ_slice(buf, dims, x);

    // Operation 1: multiply by RMS
    std::for_each(m_slice_buf.begin(), m_slice_buf.end(), [rms](auto& v) { return v *= rms; });

    // Operation 2: add mean
    std::for_each(m_slice_buf.begin(), m_slice_buf.end(), [mean](auto& v) { return v += mean; });

    m_restore_YZ_slice(buf, dims, x);
  }

  return true;
}

auto sperr::Matthias_Filter::header_size(const void* header) const -> size_t
{
  // Directly read the first 4 bytes
  uint32_t len = 0;
  std::memcpy(&len, header, sizeof(len));
  return len;
}

void sperr::Matthias_Filter::m_extract_YZ_slice(vecd_type& buf, dims_type dims, size_t x)
{
  assert(x < dims[0]);

  const auto YZ_slice = dims[1] * dims[2];
  m_slice_buf.resize(YZ_slice);

  size_t idx = 0;
  const auto XY_plane = dims[0] * dims[1];
  for (size_t z = 0; z < dims[2]; z++)
    for (size_t y = 0; y < dims[1]; y++)
      m_slice_buf[idx++] = buf[z * XY_plane + y * dims[0] + x];
}

void sperr::Matthias_Filter::m_restore_YZ_slice(vecd_type& buf, dims_type dims, size_t x)
{
  assert(x < dims[0]);
  const auto YZ_slice = dims[1] * dims[2];
  assert(YZ_slice == m_slice_buf.size());

  size_t idx = 0;
  const auto XY_plane = dims[0] * dims[1];
  for (size_t z = 0; z < dims[2]; z++)
    for (size_t y = 0; y < dims[1]; y++)
      buf[z * XY_plane + y * dims[0] + x] = m_slice_buf[idx++];
}

auto sperr::Matthias_Filter::m_calc_RMS() const -> double
{
  assert(!m_slice_buf.empty());
  auto sum = std::accumulate(m_slice_buf.cbegin(), m_slice_buf.cend(), 0.0,
                             [](auto a, auto b) { return a + b * b; });
  sum /= double(m_slice_buf.size());
  sum = std::sqrt(sum);
  return sum;
}
