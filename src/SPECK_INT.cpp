#include "SPECK_INT.h"

#include <algorithm>
#include <cassert>
#include <cstring>
#include <numeric>

//
// Free-standing helper function
//
auto sperr::speck_int_get_num_bitplanes(const void* buf) -> uint8_t
{
  // Given the header definition, directly retrieve the value stored in the first byte.
  const auto* const ptr = static_cast<const uint8_t*>(buf);
  return ptr[0];
}

template <typename T>
sperr::SPECK_INT<T>::SPECK_INT()
{
  static_assert(std::is_integral_v<T>);
  static_assert(std::is_unsigned_v<T>);
}

template <typename T>
auto sperr::SPECK_INT<T>::integer_len() const -> size_t
{
  if constexpr (std::is_same_v<uint64_t, T>)
    return sizeof(uint64_t);
  else if constexpr (std::is_same_v<uint32_t, T>)
    return sizeof(uint32_t);
  else if constexpr (std::is_same_v<uint16_t, T>)
    return sizeof(uint16_t);
  else
    return sizeof(uint8_t);
}

template <typename T>
void sperr::SPECK_INT<T>::set_dims(dims_type dims)
{
  m_dims = dims;
}

template <typename T>
void sperr::SPECK_INT<T>::reset()
{
  m_dims = {0, 0, 0};
  m_threshold = 0;
  m_coeff_buf.clear();
  m_sign_array.clear();
  m_bit_buffer.rewind();
  m_LSP_mask.reset();
  m_LIP_mask.reset();
  m_LSP_new.clear();
  m_total_bits = 0;
  m_num_bitplanes = 0;

  m_clean_LIS();
}

template <typename T>
auto sperr::SPECK_INT<T>::get_speck_bits(const void* buf) const -> uint64_t
{
  // Given the header definition, directly retrieve the value stored in bytes 1--9.
  const auto* const ptr = static_cast<const uint8_t*>(buf);
  uint64_t num_bits = 0;
  std::memcpy(&num_bits, ptr + 1, sizeof(num_bits));
  return num_bits;
}

template <typename T>
auto sperr::SPECK_INT<T>::get_stream_full_len(const void* buf) const -> uint64_t
{
  auto num_bits = get_speck_bits(buf);
  while (num_bits % 8 != 0)
    ++num_bits;
  return (header_size + num_bits / 8);
}

template <typename T>
void sperr::SPECK_INT<T>::use_bitstream(const void* p, size_t len)
{
  // Header definition: 9 bytes in total:
  // num_bitplanes (uint8_t), num_useful_bits (uint64_t)

  // Step 1: extract num_bitplanes and num_useful_bits
  assert(len >= header_size);
  const auto* const p8 = static_cast<const uint8_t*>(p);
  std::memcpy(&m_num_bitplanes, p8, sizeof(m_num_bitplanes));
  std::memcpy(&m_total_bits, p8 + sizeof(m_num_bitplanes), sizeof(m_total_bits));

  // Step 2: unpack bits.
  //    Note that the bitstream passed in might not be of its original length as a result of
  //    progressive access. In that case, we parse available bits, and pad 0's to make the
  //    bitstream still have `m_total_bits`.
  m_avail_bits = (len - header_size) * 8;
  if (m_avail_bits < m_total_bits) {
    m_bit_buffer.reserve(m_total_bits);
    m_bit_buffer.reset();  // Set buffer to contain all 0's.
    m_bit_buffer.parse_bitstream(p8 + header_size, m_avail_bits);
  }
  else {
    assert(m_avail_bits - m_total_bits < 64);
    m_avail_bits = m_total_bits;
    m_bit_buffer.parse_bitstream(p8 + header_size, m_total_bits);
  }

  // After parsing an incoming bitstream, m_avail_bits <= m_total_bits.
}

template <typename T>
void sperr::SPECK_INT<T>::encode()
{
  m_bit_buffer.reserve(m_coeff_buf.size());  // A good starting point
  m_bit_buffer.rewind();
  m_total_bits = 0;
  m_initialize_lists();

  // Mark every coefficient as insignificant
  m_LSP_mask.resize(m_coeff_buf.size());
  m_LSP_mask.reset();
  m_LSP_new.clear();
  m_LSP_new.reserve(m_coeff_buf.size() / 16);

  // Treat it as a special case when all coeffs (m_coeff_buf) are zero.
  //    In such a case, we mark `m_num_bitplanes` as zero.
  //    Of course, `m_total_bits` is also zero.
  if (std::all_of(m_coeff_buf.cbegin(), m_coeff_buf.cend(), [](auto v) { return v == 0; })) {
    m_num_bitplanes = 0;
    return;
  }

  // Decide the starting threshold.
  const auto max_coeff = *std::max_element(m_coeff_buf.cbegin(), m_coeff_buf.cend());
  m_num_bitplanes = 1;
  m_threshold = 1;
  // !! Careful loop condition so no integer overflow !!
  while (max_coeff - m_threshold >= m_threshold) {
    m_threshold *= uint_type{2};
    m_num_bitplanes++;
  }

  // Marching over bitplanes.
  for (uint8_t bitplane = 0; bitplane < m_num_bitplanes; bitplane++) {
    m_sorting_pass();
    m_refinement_pass_encode();

    m_threshold /= uint_type{2};
    m_clean_LIS();
  }

  // Record the total number of bits produced, and flush the stream.
  m_total_bits = m_bit_buffer.wtell();
  m_bit_buffer.flush();
}

template <typename T>
void sperr::SPECK_INT<T>::decode()
{
  m_initialize_lists();
  m_bit_buffer.rewind();

  // initialize coefficients to be zero, and sign array to be all positive
  const auto coeff_len = m_dims[0] * m_dims[1] * m_dims[2];
  m_coeff_buf.assign(coeff_len, uint_type{0});
  m_sign_array.assign(coeff_len, true);

  // Mark every coefficient as insignificant.
  m_LSP_mask.resize(coeff_len);
  m_LSP_mask.reset();
  m_LSP_new.clear();
  m_LSP_new.reserve(m_coeff_buf.size() / 16);

  // Handle the special case of all coeffs (m_coeff_buf) are zero by return now!
  // This case is indicated by both `m_num_bitplanes` and `m_total_bits` equal zero.
  if (m_num_bitplanes == 0) {
    assert(m_total_bits == 0);
    return;
  }

  // Restore the biggest `m_threshold`.
  m_threshold = 1;
  for (uint8_t i = 1; i < m_num_bitplanes; i++)
    m_threshold *= uint_type{2};

  // Marching over bitplanes.
  for (uint8_t bitplane = 0; bitplane < m_num_bitplanes; bitplane++) {
    m_sorting_pass();

    if (m_avail_bits != m_total_bits) {  // `m_bit_buffer` has only partial bitstream.
      if (m_bit_buffer.rtell() >= m_avail_bits)
        break;

      auto rtn = m_refinement_pass_decode_partial();
      assert(m_bit_buffer.rtell() <= m_avail_bits);
      if (rtn == RTNType::BitBudgetMet)
        break;
    }
    else {  // `m_bit_buffer` has the complete bitstream.
      m_refinement_pass_decode_complete();
    }

    m_threshold /= uint_type{2};
    m_clean_LIS();
  }

  if (m_avail_bits == m_total_bits)
    assert(m_bit_buffer.rtell() == m_total_bits);
  else {
    assert(m_bit_buffer.rtell() >= m_avail_bits);
    assert(m_bit_buffer.rtell() < m_total_bits);
  }
}

template <typename T>
auto sperr::SPECK_INT<T>::use_coeffs(vecui_type coeffs, vecb_type signs) -> RTNType
{
  if (coeffs.size() != signs.size())
    return RTNType::Error;
  m_coeff_buf = std::move(coeffs);
  m_sign_array = std::move(signs);
  return RTNType::Good;
}

template <typename T>
auto sperr::SPECK_INT<T>::release_coeffs() -> vecui_type&&
{
  return std::move(m_coeff_buf);
}

template <typename T>
auto sperr::SPECK_INT<T>::release_signs() -> vecb_type&&
{
  return std::move(m_sign_array);
}

template <typename T>
auto sperr::SPECK_INT<T>::view_coeffs() const -> const vecui_type&
{
  return m_coeff_buf;
}

template <typename T>
auto sperr::SPECK_INT<T>::view_signs() const -> const vecb_type&
{
  return m_sign_array;
}

template <typename T>
void sperr::SPECK_INT<T>::append_encoded_bitstream(vec8_type& buffer) const
{
  // Header definition: 9 bytes in total:
  // num_bitplanes (uint8_t), num_useful_bits (uint64_t)

  // Step 1: calculate size and allocate space for the encoded bitstream
  uint64_t bit_in_byte = m_total_bits / 8;
  if (m_total_bits % 8 != 0)
    ++bit_in_byte;
  const auto app_size = header_size + bit_in_byte;

  const auto orig_size = buffer.size();
  buffer.resize(orig_size + app_size);
  auto* const ptr = buffer.data() + orig_size;

  // Step 2: fill header
  size_t pos = 0;
  std::memcpy(ptr + pos, &m_num_bitplanes, sizeof(m_num_bitplanes));
  pos += sizeof(m_num_bitplanes);
  std::memcpy(ptr + pos, &m_total_bits, sizeof(m_total_bits));
  pos += sizeof(m_total_bits);

  // Step 3: assemble `m_bit_buffer` into bytes
  m_bit_buffer.write_bitstream(ptr + header_size, m_total_bits);
}

template <typename T>
void sperr::SPECK_INT<T>::m_refinement_pass_encode()
{
  // First, process significant pixels previously found.
  //
  const auto tmp1 = std::array<uint_type, 2>{uint_type{0}, m_threshold};
  const auto bits_x64 = m_LSP_mask.size() - m_LSP_mask.size() % 64;

  for (size_t i = 0; i < bits_x64; i += 64) {
    const auto value = m_LSP_mask.read_long(i);
    if (value != 0) {
      for (size_t j = 0; j < 64; j++) {
        if ((value >> j) & uint64_t{1}) {
          const bool o1 = m_coeff_buf[i + j] >= m_threshold;
          m_coeff_buf[i + j] -= tmp1[o1];
          m_bit_buffer.wbit(o1);
        }
      }
    }
  }
  for (auto i = bits_x64; i < m_LSP_mask.size(); i++) {
    if (m_LSP_mask.read_bit(i)) {
      const bool o1 = m_coeff_buf[i] >= m_threshold;
      m_coeff_buf[i] -= tmp1[o1];
      m_bit_buffer.wbit(o1);
    }
  }

  // Second, mark newly found significant pixels in `m_LSP_mask`.
  //
  for (auto idx : m_LSP_new)
    m_LSP_mask.write_true(idx);
  m_LSP_new.clear();
}

template <typename T>
void sperr::SPECK_INT<T>::m_refinement_pass_decode_complete()
{
  // First, process significant pixels previously found.
  //
  const auto tmp = std::array<uint_type, 2>{uint_type{0}, m_threshold};
  const auto bits_x64 = m_LSP_mask.size() - m_LSP_mask.size() % 64;

  for (size_t i = 0; i < bits_x64; i += 64) {
    const auto value = m_LSP_mask.read_long(i);
    if (value != 0) {
      for (size_t j = 0; j < 64; j++) {
        if ((value >> j) & uint64_t{1})
          m_coeff_buf[i + j] += tmp[m_bit_buffer.rbit()];
      }
    }
  }
  for (auto i = bits_x64; i < m_LSP_mask.size(); i++) {
    if (m_LSP_mask.read_bit(i))
      m_coeff_buf[i] += tmp[m_bit_buffer.rbit()];
  }
  assert(m_bit_buffer.rtell() <= m_total_bits);

  // Second, mark newly found significant pixels in `m_LSP_mask`
  //
  for (auto idx : m_LSP_new)
    m_LSP_mask.write_true(idx);
  m_LSP_new.clear();
}

template <typename T>
auto sperr::SPECK_INT<T>::m_refinement_pass_decode_partial() -> RTNType
{
  // First, process significant pixels previously found.
  //
  const auto tmp = std::array<uint_type, 2>{uint_type{0}, m_threshold};
  const auto bits_x64 = m_LSP_mask.size() - m_LSP_mask.size() % 64;
  assert(m_bit_buffer.rtell() < m_avail_bits);

  for (size_t i = 0; i < bits_x64; i += 64) {
    const auto value = m_LSP_mask.read_long(i);
    if (value != 0) {
      for (size_t j = 0; j < 64; j++) {
        if ((value >> j) & uint64_t{1}) {
          m_coeff_buf[i + j] += tmp[m_bit_buffer.rbit()];
          if (m_bit_buffer.rtell() == m_avail_bits)
            return RTNType::BitBudgetMet;
        }
      }
    }
  }
  for (auto i = bits_x64; i < m_LSP_mask.size(); i++) {
    if (m_LSP_mask.read_bit(i)) {
      m_coeff_buf[i] += tmp[m_bit_buffer.rbit()];
      if (m_bit_buffer.rtell() == m_avail_bits)
        return RTNType::BitBudgetMet;
    }
  }

  // Second, mark newly found significant pixels in `m_LSP_mask`
  //
  for (auto idx : m_LSP_new)
    m_LSP_mask.write_true(idx);
  m_LSP_new.clear();

  return RTNType::Good;
}

template class sperr::SPECK_INT<uint64_t>;
template class sperr::SPECK_INT<uint32_t>;
template class sperr::SPECK_INT<uint16_t>;
template class sperr::SPECK_INT<uint8_t>;
