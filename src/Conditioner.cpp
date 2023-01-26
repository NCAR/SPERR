#include <algorithm>  // std::all_of()
#include <cassert>
#include <cmath>    // std::sqrt()
#include <cstring>  // std::memcpy()
#include <numeric>  // std::accumulate()
#include <type_traits>

#include "Conditioner.h"

auto sperr::Conditioner::condition(vecd_type& buf, dims_type dims) -> vec8_type
{
  // The order of performing condition operations:
  // 1. Test constant. If it's a constant field, return immediately.
  // 2. Apply filter;
  // 3. Subtract mean;

  assert(!buf.empty());
  m_reset_meta();

  // Operation 1
  //
  if (std::all_of(buf.cbegin(), buf.cend(), [v0 = buf[0]](auto v) { return v == v0; })) {
    m_meta[m_constant_field_idx] = true;
    const double val = buf[0];
    const uint64_t nval = buf.size();
    // Assemble a header in the following order:
    // m_meta   nval  val
    //
    auto header = vec8_type(m_constant_field_header_size);
    header[0] = sperr::pack_8_booleans(m_meta);
    size_t pos = 1;
    std::memcpy(header.data() + pos, &nval, sizeof(nval));
    pos += sizeof(nval);
    std::memcpy(header.data() + pos, &val, sizeof(val));
    return header;
  }

  // Operation 2
  //
  m_meta[m_custom_filter_idx] = !std::is_same<Base_Filter, decltype(m_filter)>::value;
  auto filter_header = m_filter.apply_filter(buf, dims);

  // Operation 3
  //
  m_adjust_strides(buf.size());
  const auto mean = m_calc_mean(buf);
  std::for_each(buf.begin(), buf.end(), [mean](auto& v) { v -= mean; });

  // Assemble a header in the following order:
  // m_meta   mean  filter_header
  //
  const auto header_size = 1 + sizeof(mean) + filter_header.size();
  auto header = vec8_type(header_size);
  header[0] = sperr::pack_8_booleans(m_meta);
  size_t pos = 1;
  std::memcpy(header.data() + pos, &mean, sizeof(mean));
  pos += sizeof(mean);
  if (filter_header.size() > 0)
    std::copy(filter_header.cbegin(), filter_header.cend(), header.begin() + pos);

  return header;
}

auto sperr::Conditioner::inverse_condition(vecd_type& buf, dims_type dims, const vec8_type& header)
    -> RTNType
{
  if (header.size() < m_min_header_size)
    return RTNType::BitstreamWrongLen;

  // unpack header
  m_meta = sperr::unpack_8_booleans(header[0]);
  size_t pos = 1;

  // Operation 1: if this is a constant field?
  //
  if (m_meta[m_constant_field_idx]) {
    if (header.size() != m_constant_field_header_size)
      return RTNType::BitstreamWrongLen;

    uint64_t nval = 0;
    double val = 0.0;
    std::memcpy(&nval, header.data() + pos, sizeof(nval));
    pos += sizeof(nval);
    std::memcpy(&val, header.data() + pos, sizeof(val));

    buf.resize(nval);
    std::fill(buf.begin(), buf.end(), val);

    return RTNType::Good;
  }

  // Operation 2: add back the mean
  //
  double mean = 0.0;
  std::memcpy(&mean, header.data() + pos, sizeof(mean));
  std::for_each(buf.begin(), buf.end(), [mean](auto& v) { v += mean; });

  // Operation 3: if there's custom filter, apply the inverse of that filter
  //
  if (m_meta[m_custom_filter_idx]) {
    // Sanity check: the custom filter is compiled
    if (std::is_same<Base_Filter, decltype(m_filter)>::value)
      return RTNType::CustomFilterMissing;
    // Sanity check: the filter header size is correct
    const auto* filter = header.data() + m_min_header_size;
    const auto filter_len = header.size() - m_min_header_size;
    if (m_filter.header_size(filter) != filter_len)
      return RTNType::BitstreamWrongLen;

    if (!m_filter.inverse_filter(buf, dims, filter, filter_len))
      return RTNType::CustomFilterError;
  }

  return RTNType::Good;
}

auto sperr::Conditioner::is_constant(uint8_t byte) const -> bool
{
  auto b8 = sperr::unpack_8_booleans(byte);
  return b8[m_constant_field_idx];
}

auto sperr::Conditioner::has_custom_filter(uint8_t byte) const -> bool
{
  auto b8 = sperr::unpack_8_booleans(byte);
  return b8[m_custom_filter_idx];
}

auto sperr::Conditioner::header_size(const void* header) const -> size_t
{
  const uint8_t* ptr = static_cast<const uint8_t*>(header);
  auto b8 = sperr::unpack_8_booleans(ptr[0]);
  if (b8[m_constant_field_idx])
    return m_constant_field_header_size;

  if (b8[m_custom_filter_idx]) {
    auto filter_size = m_filter.header_size(ptr + m_min_header_size);
    return filter_size + m_min_header_size;
  }

  return m_min_header_size;
}

auto sperr::Conditioner::m_calc_mean(const vecd_type& buf) -> double
{
  assert(buf.size() % m_num_strides == 0);

  m_stride_buf.resize(m_num_strides);
  const size_t stride_size = buf.size() / m_num_strides;

  for (size_t s = 0; s < m_num_strides; s++) {
    auto begin = buf.begin() + stride_size * s;
    auto end = begin + stride_size;
    m_stride_buf[s] = std::accumulate(begin, end, double{0.0}) / static_cast<double>(stride_size);
  }

  double sum = std::accumulate(m_stride_buf.begin(), m_stride_buf.end(), double{0.0});

  return (sum / static_cast<double>(m_stride_buf.size()));
}

void sperr::Conditioner::m_adjust_strides(size_t len)
{
  m_num_strides = m_default_num_strides;
  if (len % m_num_strides == 0)
    return;

  size_t num = 0;

  // First, try to increase till 2^15 = 32,768
  for (num = m_num_strides; num <= 32'768; num++) {
    if (len % num == 0)
      break;
  }

  if (len % num == 0) {
    m_num_strides = num;
    return;
  }

  // Second, try to decrease till 1, at which point it must work.
  for (num = m_num_strides; num > 0; num--) {
    if (len % num == 0)
      break;
  }

  m_num_strides = num;
}

void sperr::Conditioner::m_reset_meta()
{
  m_meta = {true, false, false, false, false, false, false, false};
}
