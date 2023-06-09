#include "SPERR3D_OMP_C.h"

#include <algorithm>  // std::all_of()
#include <cassert>
#include <cstring>
#include <numeric>  // std::accumulate()

#ifdef USE_OMP
#include <omp.h>
#endif

void sperr::SPERR3D_OMP_C::set_num_threads(size_t n)
{
#ifdef USE_OMP
  if (n == 0)
    m_num_threads = omp_get_max_threads();
  else
    m_num_threads = n;
#endif
}

template <typename T>
auto sperr::SPERR3D_OMP_C::copy_data(const T* vol,
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
    return RTNType::VectorWrongLen;
  else
    m_dims = vol_dims;

  // The preferred chunk size has to be between 1 and m_dims.
  for (size_t i = 0; i < m_chunk_dims.size(); i++)
    m_chunk_dims[i] = std::min(std::max(size_t{1}, chunk_dims[i]), vol_dims[i]);

  // Block the volume into smaller chunks
  const auto chunks = chunk_volume(m_dims, m_chunk_dims);
  const auto num_chunks = chunks.size();
  m_chunk_buffers.resize(num_chunks);

#pragma omp parallel for num_threads(m_num_threads)
  for (size_t i = 0; i < num_chunks; i++) {
    m_chunk_buffers[i] = gather_chunk<T, double>(vol, m_dims, chunks[i]);
  }

  return RTNType::Good;
}
template auto sperr::SPERR3D_OMP_C::copy_data(const float*, size_t, dims_type, dims_type)
    -> RTNType;
template auto sperr::SPERR3D_OMP_C::copy_data(const double*, size_t, dims_type, dims_type)
    -> RTNType;

void sperr::SPERR3D_OMP_C::set_psnr(double psnr)
{
  assert(psnr > 0.0);
  m_comp_mode = CompMode::FixedPSNR;
  m_quality = psnr;
}

void sperr::SPERR3D_OMP_C::set_tolerance(double pwe)
{
  assert(pwe > 0.0);
  m_comp_mode = CompMode::FixedPWE;
  m_quality = pwe;
}

auto sperr::SPERR3D_OMP_C::compress() -> RTNType
{
  // Need to make sure that the chunks are ready!
  auto chunks = sperr::chunk_volume(m_dims, m_chunk_dims);
  const auto num_chunks = chunks.size();
  if (m_chunk_buffers.size() != num_chunks)
    return RTNType::Error;
  assert(num_chunks != 0);
  assert(m_comp_mode != sperr::CompMode::Unknown);
  assert(std::none_of(m_chunk_buffers.cbegin(), m_chunk_buffers.cend(),
                      [](auto& v) { return v.empty(); }));

  // Let's prepare some data structures for compression!
  auto chunk_rtn = std::vector<RTNType>(num_chunks, RTNType::Good);
  m_encoded_streams.resize(num_chunks);

#ifdef USE_OMP
  m_compressors.resize(m_num_threads);
  std::for_each(m_compressors.begin(), m_compressors.end(), [](auto& p) {
    if (p == nullptr)
      p = std::make_unique<SPECK3D_FLT>();
  });
#else
  if (m_compressor == nullptr)
    m_compressor = std::make_unique<SPECK3D_FLT>();
#endif

#pragma omp parallel for num_threads(m_num_threads)
  for (size_t i = 0; i < num_chunks; i++) {
#ifdef USE_OMP
    auto& compressor = m_compressors[omp_get_thread_num()];
#else
    auto& compressor = m_compressor;
#endif

    // Setup compressor parameters, and compress!
    compressor->take_data(std::move(m_chunk_buffers[i]));
    compressor->set_dims({chunks[i][1], chunks[i][3], chunks[i][5]});
    if (m_comp_mode == CompMode::FixedPSNR)
      compressor->set_psnr(m_quality);
    else if (m_comp_mode == CompMode::FixedPWE)
      compressor->set_tolerance(m_quality);
    chunk_rtn[i] = compressor->compress();

    // Save bitstream for each chunk in `m_encoded_stream`.
    m_encoded_streams[i].clear();
    m_encoded_streams[i].reserve(128);
    compressor->append_encoded_bitstream(m_encoded_streams[i]);
  }

  auto fail = std::find_if_not(chunk_rtn.begin(), chunk_rtn.end(),
                               [](auto r) { return r == RTNType::Good; });
  if (fail != chunk_rtn.end())
    return (*fail);

  assert(std::none_of(m_encoded_streams.cbegin(), m_encoded_streams.cend(),
                      [](auto& s) { return s.empty(); }));

  return RTNType::Good;
}

void sperr::SPERR3D_OMP_C::append_encoded_bitstream(vec8_type& buf) const
{
  const auto orig_size = buf.size();
  auto header = m_generate_header();
  auto new_size =
      std::accumulate(m_encoded_streams.cbegin(), m_encoded_streams.cend(), header.size(),
                      [](size_t a, const auto& b) { return a + b.size(); });
  buf.resize(orig_size + new_size, 0);

  auto itr = buf.begin() + orig_size;
  std::copy(header.cbegin(), header.cend(), itr);
  itr += header.size();
  for (const auto& s : m_encoded_streams) {
    std::copy(s.cbegin(), s.cend(), itr);
    itr += s.size();
  }
}

auto sperr::SPERR3D_OMP_C::m_generate_header() const -> sperr::vec8_type
{
  auto header = sperr::vec8_type();

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
    return header;
  auto header_size = size_t{0};
  if (num_chunks > 1)
    header_size = m_header_magic_nchunks + num_chunks * 4;
  else
    header_size = m_header_magic_1chunk + num_chunks * 4;

  header.resize(header_size);

  // Version number
  header[0] = static_cast<uint8_t>(SPERR_VERSION_MAJOR);
  size_t loc = 1;

  // 8 booleans:
  // bool[0]  : unused
  // bool[1]  : if this bitstream is for 3D (true) or 2D (false) data.
  // bool[2]  : if the original data is float (true) or double (false).
  // bool[3]  : if there are multiple chunks (true) or a single chunk (false).
  // bool[4-7]: unused
  //
  const auto b8 = std::array<bool, 8>{false,  // unused
                                      true,   // 3D
                                      m_orig_is_float,
                                      (num_chunks > 1),
                                      false,   // unused
                                      false,   // unused
                                      false,   // unused
                                      false};  // unused

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

  // Length of bitstream for each chunk.
  for (const auto& stream : m_encoded_streams) {
    assert(stream.size() <= uint64_t{std::numeric_limits<uint32_t>::max()});
    uint32_t len = stream.size();
    std::memcpy(&header[loc], &len, sizeof(len));
    loc += sizeof(len);
  }
  assert(loc == header_size);

  return header;
}
