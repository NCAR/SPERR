
#include "SPERR3D_OMP_D.h"

#include <algorithm>
#include <cassert>
#include <cstring>
#include <numeric>

#ifdef USE_OMP
#include <omp.h>
#endif

void SPERR3D_OMP_D::set_num_threads(size_t n)
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

auto SPERR3D_OMP_D::use_bitstream(const void* p, size_t total_len) -> RTNType
{
  // This method parses the header of a bitstream and puts volume dimension and
  // chunk size information in respective member variables.
  // It also stores the offset number to reach all chunks.
  // It does not, however, read the actual bitstream. The actual bitstream
  // will be provided when the decompress() method is called.

  const uint8_t* const u8p = static_cast<const uint8_t*>(p);

  // Parse Step 1: Major version number need to match
  uint8_t ver = *u8p;
  if (ver != static_cast<uint8_t>(SPERR_VERSION_MAJOR))
    return RTNType::VersionMismatch;
  size_t loc = 1;

  // Parse Step 2: ZSTD application and 3D/2D recording need to be consistent.
  auto b8 = sperr::unpack_8_booleans(u8p[loc]);
  loc++;

#ifdef USE_ZSTD
  if (b8[0] == false)
    return RTNType::ZSTDMismatch;
#else
  if (b8[0] == true)
    return RTNType::ZSTDMismatch;
#endif

  if (b8[1] == false)
    return RTNType::SliceVolumeMismatch;

  // Parse Step 3: Extract volume and chunk dimensions
  uint32_t vcdim[6];
  std::memcpy(vcdim, u8p + loc, sizeof(vcdim));
  loc += sizeof(vcdim);
  m_dims[0] = vcdim[0];
  m_dims[1] = vcdim[1];
  m_dims[2] = vcdim[2];
  m_chunk_dims[0] = vcdim[3];
  m_chunk_dims[1] = vcdim[4];
  m_chunk_dims[2] = vcdim[5];

  // Figure out how many chunks and their length
  auto chunks = sperr::chunk_volume(m_dims, m_chunk_dims);
  const auto num_chunks = chunks.size();
  auto chunk_sizes = std::vector<size_t>(num_chunks, 0);
  for (size_t i = 0; i < num_chunks; i++) {
    uint32_t len;
    std::memcpy(&len, u8p + loc, sizeof(len));
    loc += sizeof(len);
    chunk_sizes[i] = len;
  }

  // Sanity check: if the buffer size matches what the header claims
  const auto header_size = m_header_magic + num_chunks * 4;
  const auto suppose_size = std::accumulate(chunk_sizes.begin(), chunk_sizes.end(), header_size);
  if (suppose_size != total_len)
    return RTNType::BitstreamWrongLen;

  // We also calculate the offset value to address each bitstream chunk.
  m_offsets.assign(num_chunks + 1, 0);
  m_offsets[0] = header_size;
  for (size_t i = 0; i < num_chunks; i++)
    m_offsets[i + 1] = m_offsets[i] + chunk_sizes[i];

  // Finally, we keep a copy of the bitstream pointer
  m_bitstream_ptr = u8p;

  return RTNType::Good;
}

auto SPERR3D_OMP_D::decompress(const void* p) -> RTNType
{
  auto eq0 = [](auto v) { return v == 0; };
  if (std::any_of(m_dims.begin(), m_dims.end(), eq0) ||
      std::any_of(m_chunk_dims.begin(), m_chunk_dims.end(), eq0))
    return RTNType::Error;
  if (p == nullptr || m_bitstream_ptr == nullptr)
    return RTNType::Error;
  if (static_cast<const uint8_t*>(p) != m_bitstream_ptr)
    return RTNType::Error;

  // Let's figure out the chunk information
  const auto chunks = sperr::chunk_volume(m_dims, m_chunk_dims);
  const auto num_chunks = chunks.size();
  if (m_offsets.size() != num_chunks + 1)
    return RTNType::Error;
  const auto total_vals = m_dims[0] * m_dims[1] * m_dims[2];

  // Allocate a buffer to store the entire volume
  m_vol_buf.resize(total_vals);

  // Create number of decompressor instances equal to the number of threads
  auto decompressors = std::vector<sperr::SPERR3D_Decompressor>(m_num_threads);
  auto chunk_rtn = std::vector<RTNType>(num_chunks * 3, RTNType::Good);

#pragma omp parallel for num_threads(m_num_threads)
  for (size_t i = 0; i < num_chunks; i++) {
#ifdef USE_OMP
    auto& decompressor = decompressors[omp_get_thread_num()];
#else
    auto& decompressor = decompressors[0];
#endif

    decompressor.set_dims({chunks[i][1], chunks[i][3], chunks[i][5]});

    chunk_rtn[i * 3] =
        decompressor.use_bitstream(m_bitstream_ptr + m_offsets[i], m_offsets[i + 1] - m_offsets[i]);

    chunk_rtn[i * 3 + 1] = decompressor.decompress();
    const auto& small_vol = decompressor.view_data();
    if (small_vol.empty())
      chunk_rtn[i * 3 + 2] = RTNType::Error;
    else {
      chunk_rtn[i * 3 + 2] = RTNType::Good;
      sperr::scatter_chunk(m_vol_buf, m_dims, small_vol, chunks[i]);
    }
  }

  auto fail =
      std::find_if(chunk_rtn.begin(), chunk_rtn.end(), [](auto r) { return r != RTNType::Good; });
  if (fail != chunk_rtn.end())
    return *fail;
  else
    return RTNType::Good;
}

auto SPERR3D_OMP_D::release_data() -> sperr::vecd_type&&
{
  m_dims = {0, 0, 0};
  return std::move(m_vol_buf);
}

auto SPERR3D_OMP_D::view_data() const -> const std::vector<double>&
{
  return m_vol_buf;
}

auto SPERR3D_OMP_D::get_dims() const -> std::array<size_t, 3>
{
  return m_dims;
}

template <typename T>
auto SPERR3D_OMP_D::get_data() const -> std::vector<T>
{
  auto rtn_buf = std::vector<T>(m_vol_buf.size());
  std::copy(m_vol_buf.begin(), m_vol_buf.end(), rtn_buf.begin());
  return rtn_buf;
}
template auto SPERR3D_OMP_D::get_data() const -> std::vector<float>;
template auto SPERR3D_OMP_D::get_data() const -> std::vector<double>;
