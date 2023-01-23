#include "SPERR3D_OMP_C.h"

#include <algorithm>  // std::all_of()
#include <cassert>
#include <cstring>
#include <numeric>  // std::accumulate()

#ifdef USE_OMP
#include <omp.h>
#endif

void SPERR3D_OMP_C::set_num_threads(size_t n)
{
#ifdef USE_OMP
  if (n == 0)
    m_num_threads = omp_get_max_threads();
  else
    m_num_threads = n;
#else
  m_num_threads = 1;
#endif
}

auto SPERR3D_OMP_C::get_outlier_stats() const -> std::pair<size_t, size_t>
{
  using pair = std::pair<size_t, size_t>;
  pair sum{0, 0};
  auto op = [](const pair& a, const pair& b) -> pair {
    return {a.first + b.first, a.second + b.second};
  };
  return std::accumulate(m_outlier_stats.begin(), m_outlier_stats.end(), sum, op);
}

auto SPERR3D_OMP_C::set_target_bpp(double bpp) -> RTNType
{
  if (bpp <= 0.0 || bpp > 64.0)
    return RTNType::InvalidParam;

  // If the volume and chunk dimension hasn't been set, return error.
  auto eq0 = [](auto v) { return v == 0; };
  if (std::any_of(m_dims.begin(), m_dims.end(), eq0) ||
      std::any_of(m_chunk_dims.begin(), m_chunk_dims.end(), eq0))
    return RTNType::SetBPPBeforeDims;

  const auto total_vals = static_cast<double>(m_dims[0] * m_dims[1] * m_dims[2]);
  m_bit_budget = static_cast<size_t>(bpp * total_vals);

  // Also set other termination criteria to be "never terminate."
  m_target_psnr = sperr::max_d;
  m_target_pwe = 0.0;

  return RTNType::Good;
}

void SPERR3D_OMP_C::set_target_psnr(double psnr)
{
  m_target_psnr = std::max(psnr, 0.0);
  m_bit_budget = sperr::max_size;
  m_target_pwe = 0.0;
}

void SPERR3D_OMP_C::set_target_pwe(double pwe)
{
  m_target_pwe = std::max(pwe, 0.0);
  m_bit_budget = sperr::max_size;
  m_target_psnr = sperr::max_d;
}

template <typename T>
auto SPERR3D_OMP_C::copy_data(const T* vol,
                              size_t len,
                              sperr::dims_type vol_dims,
                              sperr::dims_type chunk_dims) -> RTNType
{
  static_assert(std::is_floating_point<T>::value, "!! Only floating point values are supported !!");

  if constexpr (std::is_same<T, float>::value)
    m_orig_is_float = true;
  else
    m_orig_is_float = false;

  if (len != vol_dims[0] * vol_dims[1] * vol_dims[2])
    return RTNType::WrongDims;
  else
    m_dims = vol_dims;

  // The preferred chunk size has to be between 1 and m_dims.
  for (size_t i = 0; i < m_chunk_dims.size(); i++)
    m_chunk_dims[i] = std::min(std::max(size_t{1}, chunk_dims[i]), vol_dims[i]);

  // Block the volume into smaller chunks
  const auto chunks = sperr::chunk_volume(m_dims, m_chunk_dims);
  const auto num_chunks = chunks.size();
  m_chunk_buffers.resize(num_chunks);

#pragma omp parallel for num_threads(m_num_threads)
  for (size_t i = 0; i < num_chunks; i++) {
    m_chunk_buffers[i] = sperr::gather_chunk<T, double>(vol, m_dims, chunks[i]);
  }

  return RTNType::Good;
}
template auto SPERR3D_OMP_C::copy_data(const float*, size_t, sperr::dims_type, sperr::dims_type)
    -> RTNType;
template auto SPERR3D_OMP_C::copy_data(const double*, size_t, sperr::dims_type, sperr::dims_type)
    -> RTNType;

auto SPERR3D_OMP_C::compress() -> RTNType
{
  // Need to make sure that the chunks are ready!
  auto chunks = sperr::chunk_volume(m_dims, m_chunk_dims);
  const auto num_chunks = chunks.size();
  assert(num_chunks != 0);
  if (m_chunk_buffers.size() != num_chunks)
    return RTNType::Error;
  if (std::any_of(m_chunk_buffers.begin(), m_chunk_buffers.end(),
                  [](auto& v) { return v.empty(); }))
    return RTNType::Error;

  // Sanity check: what compression mode to use?
  const auto mode = sperr::compression_mode(m_bit_budget, m_target_psnr, m_target_pwe);
  assert(mode != sperr::CompMode::Unknown);

  // Let's prepare some data structures for compression!
  assert(m_num_threads > 0);
  auto compressors = std::vector<sperr::SPERR3D_Compressor>(m_num_threads);
  auto chunk_rtn = std::vector<RTNType>(num_chunks, RTNType::Good);
  m_encoded_streams.resize(num_chunks);
  std::for_each(m_encoded_streams.begin(), m_encoded_streams.end(), [](auto& v) { v.clear(); });
  m_outlier_stats.assign(num_chunks, {0, 0});

#pragma omp parallel for num_threads(m_num_threads)
  for (size_t i = 0; i < num_chunks; i++) {
#ifdef USE_OMP
    auto& compressor = compressors[omp_get_thread_num()];
#else
    auto& compressor = compressors[0];
#endif

    // Prepare for compression
    compressor.take_data(std::move(m_chunk_buffers[i]), {chunks[i][1], chunks[i][3], chunks[i][5]});

    // Figure out the bit budget for this chunk
    auto my_budget = size_t{0};
    if (m_bit_budget == sperr::max_size)
      my_budget = sperr::max_size;
    else {
      const auto total_vals = m_dims[0] * m_dims[1] * m_dims[2];
      const auto chunk_vals = chunks[i][1] * chunks[i][3] * chunks[i][5];
      auto avail_bits = size_t{0};
      if (num_chunks > 1)
        avail_bits = m_bit_budget - (m_header_magic_nchunks + num_chunks * 4) * 8;
      else
        avail_bits = m_bit_budget - (m_header_magic_1chunk + num_chunks * 4) * 8;
      my_budget = static_cast<size_t>((static_cast<double>(chunk_vals) / total_vals) * avail_bits);
      while (my_budget % 8 != 0)
        my_budget--;
    }

    chunk_rtn[i] = compressor.set_comp_params(my_budget, m_target_psnr, m_target_pwe);

    if (chunk_rtn[i] == RTNType::Good) {
      chunk_rtn[i] = compressor.compress();
    }

    m_encoded_streams[i] = compressor.view_encoded_bitstream();

    m_outlier_stats[i] = compressor.get_outlier_stats();
  }

  auto fail =
      std::find_if(chunk_rtn.begin(), chunk_rtn.end(), [](auto r) { return r != RTNType::Good; });
  if (fail != chunk_rtn.end())
    return (*fail);

  if (std::any_of(m_encoded_streams.begin(), m_encoded_streams.end(),
                  [](auto& s) { return s.empty(); }))
    return RTNType::EmptyStream;

  return RTNType::Good;
}

auto SPERR3D_OMP_C::get_encoded_bitstream() const -> std::vector<uint8_t>
{
  auto buf = std::vector<uint8_t>();
  auto header = m_generate_header();
  if (header.empty())
    return buf;

  auto total_size =
      std::accumulate(m_encoded_streams.begin(), m_encoded_streams.end(), header.size(),
                      [](size_t a, const auto& b) { return a + b.size(); });
  buf.resize(total_size, 0);

  std::copy(header.begin(), header.end(), buf.begin());
  auto itr = buf.begin() + header.size();
  for (const auto& s : m_encoded_streams) {
    std::copy(s.begin(), s.end(), itr);
    itr += s.size();
  }

  return buf;
}

auto SPERR3D_OMP_C::m_generate_header() const -> sperr::vec8_type
{
  // The header would contain the following information
  //  -- a version number                     (1 byte)
  //  -- 8 booleans                           (1 byte)
  //  -- volume and/or chunk dimensions       (4 x 6 = 24 or 4 x 3 = 12 bytes)
  //  -- length of bitstream for each chunk   (4 x num_chunks)
  //
  auto chunks = sperr::chunk_volume(m_dims, m_chunk_dims);
  const auto num_chunks = chunks.size();
  assert(num_chunks != 0);
  if (num_chunks != m_encoded_streams.size())
    return std::vector<uint8_t>();
  auto header_size = size_t{0};
  if (num_chunks > 1)
    header_size = m_header_magic_nchunks + num_chunks * 4;
  else
    header_size = m_header_magic_1chunk + num_chunks * 4;

  auto header = std::vector<uint8_t>(header_size);

  // Version number
  header[0] = static_cast<uint8_t>(SPERR_VERSION_MAJOR);
  size_t loc = 1;

  // 8 booleans:
  // bool[0]  : if ZSTD is used
  // bool[1]  : if this bitstream is for 3D (true) or 2D (false) data.
  // bool[2]  : if the original data is float (true) or double (false).
  // bool[3]  : if there are multiple chunks (true) or a single chunk (false).
  // bool[4-7]: undefined
  //
  const auto b8 = std::array<bool, 8>{
#ifdef USE_ZSTD
      true,  // using ZSTD
#else
      false,  // NOT using ZSTD
#endif
      true,  // 3D
      m_orig_is_float,
      (num_chunks > 1),
      false,   // undefined
      false,   // undefined
      false,   // undefined
      false};  // undefined

  header[loc] = sperr::pack_8_booleans(b8);
  loc += 1;

  // Volume and chunk dimensions
  if (num_chunks > 1) {
    const uint32_t vcdim[6] = {
        static_cast<uint32_t>(m_dims[0]),       static_cast<uint32_t>(m_dims[1]),
        static_cast<uint32_t>(m_dims[2]),       static_cast<uint32_t>(m_chunk_dims[0]),
        static_cast<uint32_t>(m_chunk_dims[1]), static_cast<uint32_t>(m_chunk_dims[2])};
    std::memcpy(&header[loc], vcdim, sizeof(vcdim));
    loc += sizeof(vcdim);
  }
  else {
    const uint32_t vdim[3] = {static_cast<uint32_t>(m_dims[0]), static_cast<uint32_t>(m_dims[1]),
                              static_cast<uint32_t>(m_dims[2])};
    std::memcpy(&header[loc], vdim, sizeof(vdim));
    loc += sizeof(vdim);
  }

  // Length of bitstream for each chunk
  // Note that we use uint32_t to keep the length, and we need to make sure
  // that no chunk size is bigger than that.
  for (const auto& stream : m_encoded_streams) {
    assert(stream.size() <= uint64_t{std::numeric_limits<uint32_t>::max()});
    uint32_t len = stream.size();
    std::memcpy(&header[loc], &len, sizeof(len));
    loc += sizeof(len);
  }
  assert(loc == header_size);

  return header;
}
