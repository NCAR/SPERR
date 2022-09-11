#include "SPERR.h"

#include <algorithm>  // std::lower_bound()
#include <cassert>
#include <cmath>
#include <cstring>

//
// Class Outlier
//
sperr::Outlier::Outlier(size_t loc, double e) : location(loc), error(e) {}

//
// Class SPERR
//
void sperr::SPERR::add_outlier(size_t pos, double e)
{
  m_LOS.emplace_back(pos, e);
}

void sperr::SPERR::copy_outlier_list(const std::vector<Outlier>& list)
{
  // Not using the "pass by value and move" idiom because we want to
  // reuse the storage of `m_LOS` here.
  m_LOS = list;
}

void sperr::SPERR::take_outlier_list(std::vector<Outlier>&& list)
{
  m_LOS = std::move(list);
}

void sperr::SPERR::set_length(uint64_t len)
{
  m_total_len = len;
}

void sperr::SPERR::set_tolerance(double t)
{
  m_tolerance = t;
}

auto sperr::SPERR::release_outliers() -> std::vector<Outlier>&&
{
  return std::move(m_LOS);
}
auto sperr::SPERR::view_outliers() -> const std::vector<Outlier>&
{
  return m_LOS;
}

auto sperr::SPERR::m_part_set(const SPECKSet1D& set) const -> std::array<SPECKSet1D, 2>
{
  std::array<SPECKSet1D, 2> subsets;
  // Prepare the 1st set
  auto& set1 = subsets[0];
  set1.start = set.start;
  set1.length = set.length - set.length / 2;
  set1.part_level = set.part_level + 1;
  // Prepare the 2nd set
  auto& set2 = subsets[1];
  set2.start = set.start + set1.length;
  set2.length = set.length / 2;
  set2.part_level = set.part_level + 1;

  return subsets;
}

void sperr::SPERR::m_initialize_LIS()
{
  // Note that `m_LIS` is a 2D array. In order to avoid unnecessary memory
  // allocation, we don't clear `m_LIS` itself, but clear every secondary array
  // it holds. This is OK as long as the extra secondary arrays are cleared.
  auto num_of_parts = sperr::num_of_partitions(m_total_len);
  auto num_of_lists = num_of_parts + 1;
  if (m_LIS.size() < num_of_lists)
    m_LIS.resize(num_of_lists);
  for (auto& list : m_LIS)
    list.clear();

  // Put in two sets, each representing a half of the long array.
  SPECKSet1D set;
  set.length = m_total_len;  // Set represents the whole 1D array.
  auto sets = m_part_set(set);
  m_LIS[sets[0].part_level].emplace_back(sets[0]);
  m_LIS[sets[1].part_level].emplace_back(sets[1]);
}

void sperr::SPERR::m_clean_LIS()
{
  for (auto& list : m_LIS) {
    auto itr = std::remove_if(list.begin(), list.end(),
                              [](auto& s) { return s.type == SetType::Garbage; });
    list.erase(itr, list.end());
  }
}

auto sperr::SPERR::m_ready_to_encode() const -> bool
{
  if (m_total_len == 0)
    return false;
  if (m_tolerance <= 0.0)
    return false;
  if (m_LOS.empty())
    return false;

  // Make sure each outlier to process has an error greater or equal to the tolerance.
  if (!std::all_of(m_LOS.begin(), m_LOS.end(),
                   [tol = m_tolerance](auto& out) { return std::abs(out.error) >= tol; }))
    return false;

  // Make sure there are no duplicate locations in the outlier list,
  // and the largest location is still within range.
  // Note that the list of outliers is already sorted at the beginning of encoding.
  if (m_LOS.back().location >= m_total_len)
    return false;
  auto adj = std::adjacent_find(m_LOS.begin(), m_LOS.end(),
                                [](auto& a, auto& b) { return a.location == b.location; });
  return (adj == m_LOS.end());
}

auto sperr::SPERR::m_ready_to_decode() const -> bool
{
  if (m_total_len == 0)
    return false;
  if (m_bit_buffer.empty())
    return false;

  return true;
}

auto sperr::SPERR::encode() -> RTNType
{
  // Let's sort the list of outliers so it'll be easier to locate particular individuals.
  // Actually, if the list of outliers is produced by this package (SPECK3D_Compressor),
  // it's already in the desired order so this step has no cost!
  // Outliers added individually will potentially trigger actual sorting operations though.
  std::sort(m_LOS.begin(), m_LOS.end(), [](auto& a, auto& b) { return a.location < b.location; });
  if (!m_ready_to_encode())
    return RTNType::InvalidParam;
  m_bit_buffer.clear();
  m_encode_mode = true;

  m_initialize_LIS();

  // `m_q` is initialized to have the absolute value of all error.
  m_q.resize(m_LOS.size());
  std::transform(m_LOS.cbegin(), m_LOS.cend(), m_q.begin(),
                 [](auto out) { return std::abs(out.error); });

  m_LSP_new.clear();
  m_LSP_old.clear();
  m_LSP_old.reserve(m_LOS.size());

  auto max_q = *(std::max_element(m_q.cbegin(), m_q.cend()));
  auto max_t = m_tolerance * 0.99;  // make the terminal threshold just a tiny bit smaller.
  m_num_itrs = 1;
  while (max_t * 2.0 < max_q) {
    max_t *= 2.0;
    m_num_itrs++;
  }
  m_max_threshold_f = static_cast<float>(max_t);
  m_threshold = static_cast<double>(m_max_threshold_f);

  // Start the iterations!
  for (size_t bitplane = 0; bitplane < m_num_itrs; bitplane++) {
    // Reset the significance map
    m_sig_map.assign(m_total_len, false);
    for (size_t i = 0; i < m_LOS.size(); i++) {
      if (m_q[i] >= m_threshold)
        m_sig_map[m_LOS[i].location] = true;
    }

    m_sorting_pass();
    m_refinement_pass_encoding();

    m_threshold *= 0.5;
    m_clean_LIS();
  }

  return RTNType::Good;
}

auto sperr::SPERR::decode() -> RTNType
{
  if (!m_ready_to_decode())
    return RTNType::InvalidParam;
  m_encode_mode = false;

  // Clear/initialize data structures
  m_LOS.clear();
  m_recovered_signs.clear();
  m_initialize_LIS();
  m_bit_idx = 0;
  m_LOS_size = 0;

  m_threshold = static_cast<double>(m_max_threshold_f);

  for (size_t bitplane = 0; bitplane < 64; bitplane++) {
    m_sorting_pass();
    m_refinement_pass_decoding();

    m_threshold *= 0.5;
    m_clean_LIS();

    if (m_bit_idx >= m_bit_buffer.size()) {
      assert(m_bit_idx == m_bit_buffer.size());
      break;
    }
  }

  // Put restored values in m_LOS with proper signs
  assert(m_LOS.size() == m_recovered_signs.size());
  for (size_t idx = 0; idx < m_LOS.size(); idx++) {
    if (m_recovered_signs[idx] == false) {
      m_LOS[idx].error = -m_LOS[idx].error;
    }
  }

  return RTNType::Good;
}

auto sperr::SPERR::m_decide_significance(const SPECKSet1D& set) const -> std::pair<bool, size_t>
{
  // This function is only used during encoding.

  // Step 1: use the significance map to decide if this set is significant
  std::pair<bool, size_t> sig{false, 0};
  auto begin = m_sig_map.cbegin() + set.start;
  auto end = begin + set.length;
  auto itr1 = std::find(begin, end, true);

  // Step 2: if this set is significant, then find the index of the outlier in
  //         `m_LSO` that caused it being significant.
  if (itr1 != end) {
    sig.first = true;

    // Note that `m_LSO` is sorted at the beginning of encoding.
    size_t idx_to_find = std::distance(m_sig_map.cbegin(), itr1);
    auto itr2 = std::lower_bound(m_LOS.cbegin(), m_LOS.cend(), idx_to_find,
                                 [](const auto& otl, auto idx) { return otl.location < idx; });
    assert(itr2 != m_LOS.cend());
    assert((*itr2).location == idx_to_find);  // Must find exactly this index
    sig.second = std::distance(m_LOS.cbegin(), itr2);
  }

  return sig;
}

void sperr::SPERR::m_process_S_encoding(size_t idx1, size_t idx2, size_t& counter, bool output)
{
  auto& set = m_LIS[idx1][idx2];
  auto [is_sig, sig_idx] = m_decide_significance(set);

  if (output)
    m_bit_buffer.push_back(is_sig);

  // Sanity check: when `output` is false, `is_sig` must be true.
  assert(output || is_sig);

  if (is_sig) {
    counter++;
    if (set.length == 1) {                                  // Is a pixel
      m_bit_buffer.push_back(m_LOS[sig_idx].error >= 0.0);  // Record its sign
      m_LSP_new.push_back(sig_idx);
      m_q[sig_idx] -= m_threshold;
    }
    else {  // Not a pixel
      m_code_S(idx1, idx2);
    }

    set.type = SetType::Garbage;
  }
}

void sperr::SPERR::m_code_S(size_t idx1, size_t idx2)
{
  auto sets = m_part_set(m_LIS[idx1][idx2]);
  size_t counter = 0;

  // Process the 1st set
  const auto& s1 = sets[0];
  if (s1.length > 0) {
    auto newi1 = s1.part_level;
    m_LIS[newi1].emplace_back(s1);
    auto newi2 = m_LIS[newi1].size() - 1;
    if (m_encode_mode)
      m_process_S_encoding(newi1, newi2, counter, true);
    else
      m_process_S_decoding(newi1, newi2, counter, true);
  }

  // Process the 2nd set
  const auto& s2 = sets[1];
  if (s2.length > 0) {
    auto newi1 = s2.part_level;
    m_LIS[newi1].emplace_back(s2);
    auto newi2 = m_LIS[newi1].size() - 1;

    // If the 1st set wasn't significant, then the 2nd set must be significant,
    // thus don't need to output/input this information.
    bool IO = (counter == 0 ? false : true);
    if (m_encode_mode)
      m_process_S_encoding(newi1, newi2, counter, IO);
    else
      m_process_S_decoding(newi1, newi2, counter, IO);
  }
}

void sperr::SPERR::m_sorting_pass()
{
  size_t dummy = 0;
  for (size_t tmp = 1; tmp <= m_LIS.size(); tmp++) {
    size_t idx1 = m_LIS.size() - tmp;
    for (size_t idx2 = 0; idx2 < m_LIS[idx1].size(); idx2++) {
      if (m_encode_mode)
        m_process_S_encoding(idx1, idx2, dummy, true);
      else
        m_process_S_decoding(idx1, idx2, dummy, true);
    }
  }
}

void sperr::SPERR::m_refinement_pass_encoding()
{
  for (auto idx : m_LSP_old) {
    auto need_refine = m_q[idx] >= m_threshold;
    m_bit_buffer.push_back(need_refine);
    if (need_refine)
      m_q[idx] -= m_threshold;
  }

  // Now attach content from `m_LSP_new` to the end of `m_LSP_old`.
  m_LSP_old.insert(m_LSP_old.end(), m_LSP_new.begin(), m_LSP_new.end());
  m_LSP_new.clear();
}

void sperr::SPERR::m_process_S_decoding(size_t idx1, size_t idx2, size_t& counter, bool input)
{
  bool is_sig = true;
  if (input) {
    is_sig = m_bit_buffer[m_bit_idx++];
    assert(m_bit_idx <= m_bit_buffer.size());
  }

  if (is_sig) {
    counter++;
    auto& set = m_LIS[idx1][idx2];
    if (set.length == 1) {  // This is a pixel
      // We recovered the location of another outlier!
      m_LOS.emplace_back(set.start, m_threshold * 1.5);
      m_recovered_signs.push_back(m_bit_buffer[m_bit_idx++]);

      assert(m_bit_idx <= m_bit_buffer.size());
    }
    else {
      m_code_S(idx1, idx2);
    }

    set.type = SetType::Garbage;
  }
}

void sperr::SPERR::m_refinement_pass_decoding()
{
  // Refine significant pixels from previous iterations only,
  //   because pixels added from this iteration are already refined.
  assert(m_bit_idx + m_LOS_size <= m_bit_buffer.size());
  for (size_t idx = 0; idx < m_LOS_size; idx++) {
    if (m_bit_buffer[m_bit_idx++])
      m_LOS[idx].error += m_threshold * 0.5;
    else
      m_LOS[idx].error -= m_threshold * 0.5;
  }

  // Record the size of `m_LOS` after this iteration.
  m_LOS_size = m_LOS.size();
}

auto sperr::SPERR::get_encoded_bitstream() -> std::vector<uint8_t>
{
  // Header definition:
  // total_len  max_threshold_f   num_of_bits
  // uint64_t   float             uint64_t

  // Record the current (useful) number of bits before it's padded.
  const uint64_t num_bits = m_bit_buffer.size();
  while (m_bit_buffer.size() % 8 != 0)
    m_bit_buffer.push_back(false);

  const size_t buf_len = m_header_size + m_bit_buffer.size() / 8;
  auto buf = std::vector<uint8_t>(buf_len);

  // Fill header
  size_t pos = 0;

  std::memcpy(&buf[0], &m_total_len, sizeof(m_total_len));
  pos += sizeof(m_total_len);

  std::memcpy(&buf[pos], &m_max_threshold_f, sizeof(m_max_threshold_f));
  pos += sizeof(m_max_threshold_f);

  std::memcpy(&buf[pos], &num_bits, sizeof(num_bits));
  pos += sizeof(num_bits);

  assert(pos == m_header_size);

  // Assemble the bitstream into bytes
  auto rtn = sperr::pack_booleans(buf, m_bit_buffer, pos);
  if (rtn != RTNType::Good)
    buf.clear();

  // Restore `m_bit_buffer` to its original size.
  m_bit_buffer.resize(num_bits);

  return buf;
}

auto sperr::SPERR::parse_encoded_bitstream(const void* buf, size_t len) -> RTNType
{
  // The buffer passed in is supposed to consist a header and then a compacted
  // bitstream, just like what was returned by `get_encoded_bitstream()`. Note:
  // header definition is documented in get_encoded_bitstream().

  const uint8_t* const ptr = static_cast<const uint8_t*>(buf);

  // Parse header
  uint64_t num_bits, pos = 0;

  std::memcpy(&m_total_len, ptr, sizeof(m_total_len));
  pos += sizeof(m_total_len);

  std::memcpy(&m_max_threshold_f, ptr + pos, sizeof(m_max_threshold_f));
  pos += sizeof(m_max_threshold_f);

  std::memcpy(&num_bits, ptr + pos, sizeof(num_bits));
  pos += sizeof(num_bits);

  assert(pos == m_header_size);

  // Unpack bits
  const auto num_of_bools = (len - pos) * 8;
  m_bit_buffer.resize(num_of_bools, false);  // allocate enough space before passing it around
  auto rtn = sperr::unpack_booleans(m_bit_buffer, buf, len, pos);
  if (rtn != RTNType::Good)
    return rtn;

  // Restore the correct number of bits
  m_bit_buffer.resize(num_bits);

  return RTNType::Good;
}

auto sperr::SPERR::get_sperr_stream_size(const void* buf) const -> uint64_t
{
  // Given the header definition in `get_encoded_bitstream()`, directly
  // go retrieve the value stored in byte 12-20.
  const uint8_t* const ptr = static_cast<const uint8_t*>(buf);
  uint64_t num_bits;
  std::memcpy(&num_bits, ptr + 12, sizeof(num_bits));
  while (num_bits % 8 != 0)
    num_bits++;

  return (m_header_size + num_bits / 8);
}
