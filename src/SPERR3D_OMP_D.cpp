#include "SPERR3D_OMP_D.h"

#include <algorithm>
#include <cassert>
#include <cstring>
#include <numeric>

#ifdef USE_OMP
#include <omp.h>
#endif

void sperr::SPERR3D_OMP_D::set_num_threads(size_t n)
{
#ifdef USE_OMP
  if (n == 0)
    m_num_threads = omp_get_max_threads();
  else
    m_num_threads = n;
#endif
}

auto sperr::SPERR3D_OMP_D::setup_decomp(const void* p, size_t total_len) -> RTNType
{
  // This method parses the header of a bitstream and puts volume dimension and
  // chunk size information in respective member variables.
  // It also stores the offset number to reach all chunks.
  // It does NOT, however, read the actual bitstream. The actual bitstream
  // will be provided when the decompress() method is called.
  // The header definition is in SPERR3D_OMP_D.cpp::m_generate_header().
  //

  const uint8_t* const u8p = static_cast<const uint8_t*>(p);

  // Parse Step 1: Major version number need to match
  uint8_t ver = *u8p;
  if (ver != static_cast<uint8_t>(SPERR_VERSION_MAJOR))
    return RTNType::VersionMismatch;
  size_t pos = 1;

  // Parse Step 2: 3D/2D recording need to be consistent.
  const auto b8 = sperr::unpack_8_booleans(u8p[pos]);
  pos++;
  if (b8[1] == false)
    return RTNType::SliceVolumeMismatch;

  const auto multi_chunk = b8[3];

  // Parse Step 3: Extract volume and chunk dimensions
  uint32_t vdim[3] = {0, 0, 0};
  std::memcpy(vdim, u8p + pos, sizeof(vdim));
  pos += sizeof(vdim);
  m_dims[0] = vdim[0];
  m_dims[1] = vdim[1];
  m_dims[2] = vdim[2];
  m_chunk_dims = m_dims;

  if (multi_chunk) {
    uint16_t vcdim[3] = {0, 0, 0};
    std::memcpy(vcdim, u8p + pos, sizeof(vcdim));
    pos += sizeof(vcdim);
    m_chunk_dims[0] = vcdim[0];
    m_chunk_dims[1] = vcdim[1];
    m_chunk_dims[2] = vcdim[2];
  }

  // Figure out how many chunks and their length.
  auto chunks = sperr::chunk_volume(m_dims, m_chunk_dims);
  const auto num_chunks = chunks.size();
  assert((multi_chunk && num_chunks > 1) || (!multi_chunk && num_chunks == 1));
  auto chunk_sizes = std::vector<size_t>(num_chunks, 0);
  for (size_t i = 0; i < num_chunks; i++) {
    uint32_t len;
    std::memcpy(&len, u8p + pos, sizeof(len));
    pos += sizeof(len);
    chunk_sizes[i] = len;
  }

  // Sanity check: if the buffer size matches what the header claims
  size_t header_len = num_chunks * 4;
  if (multi_chunk)
    header_len += m_header_magic_nchunks;
  else
    header_len += m_header_magic_1chunk;

  const auto suppose_size = std::accumulate(chunk_sizes.cbegin(), chunk_sizes.cend(), header_len);
  if (suppose_size != total_len)
    return RTNType::BitstreamWrongLen;

  // We also calculate the offset value to address each bitstream chunk.
  m_offsets.assign(num_chunks + 1, 0);
  m_offsets[0] = header_len;
  for (size_t i = 0; i < num_chunks; i++)
    m_offsets[i + 1] = m_offsets[i] + chunk_sizes[i];

  // Finally, we keep a copy of the bitstream pointer
  m_bitstream_ptr = u8p;

  return RTNType::Good;
}

auto sperr::SPERR3D_OMP_D::get_header_len(std::array<uint8_t, 20> magic) const -> size_t
{
  // The header definition is in SPERR3D_OMP_D.cpp::m_generate_header().

  // Step 1: decode the 8 booleans, and decide if there are multiple chunks.
  const auto b8 = sperr::unpack_8_booleans(magic[1]);
  const auto multi_chunk = b8[3];

  // Step 2: Extract volume and chunk dimensions
  size_t pos = 2;
  uint32_t int3[3] = {0, 0, 0};
  std::memcpy(int3, magic.data() + pos, sizeof(int3));
  pos += sizeof(int3);
  dims_type vdim = {int3[0], int3[1], int3[2]};
  dims_type cdim = {int3[0], int3[1], int3[2]};
  if (multi_chunk) {
    uint16_t short3[3] = {0, 0, 0};
    std::memcpy(short3, magic.data() + pos, sizeof(short3));
    pos += sizeof(short3);
    cdim[0] = short3[0];
    cdim[1] = short3[1];
    cdim[2] = short3[2];
  }

  // Step 3: figure out how many chunks are there, and the header length.
  auto chunks = sperr::chunk_volume(m_dims, m_chunk_dims);
  const auto num_chunks = chunks.size();
  assert((multi_chunk && num_chunks > 1) || (!multi_chunk && num_chunks == 1));
  size_t header_len = num_chunks * 4;
  if (multi_chunk)
    header_len += m_header_magic_nchunks;
  else
    header_len += m_header_magic_1chunk;

  return header_len;
}

auto sperr::SPERR3D_OMP_D::decompress(const void* p) -> RTNType
{
  if (p == nullptr || m_bitstream_ptr == nullptr)
    return RTNType::Error;
  if (static_cast<const uint8_t*>(p) != m_bitstream_ptr)
    return RTNType::Error;
  auto eq0 = [](auto v) { return v == 0; };
  if (std::any_of(m_dims.cbegin(), m_dims.cend(), eq0) ||
      std::any_of(m_chunk_dims.cbegin(), m_chunk_dims.cend(), eq0))
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
  auto chunk_rtn = std::vector<RTNType>(num_chunks * 2, RTNType::Good);

#ifdef USE_OMP
  m_decompressors.resize(m_num_threads);
  std::for_each(m_decompressors.begin(), m_decompressors.end(), [](auto& p) {
    if (p == nullptr)
      p = std::make_unique<SPECK3D_FLT>();
  });
#else
  if (m_decompressor == nullptr)
    m_decompressor = std::make_unique<SPECK3D_FLT>();
#endif

#pragma omp parallel for num_threads(m_num_threads)
  for (size_t i = 0; i < num_chunks; i++) {
#ifdef USE_OMP
    auto& decompressor = m_decompressors[omp_get_thread_num()];
#else
    auto& decompressor = m_decompressor;
#endif

    // Setup decompressor parameters, and decompress!
    decompressor->set_dims({chunks[i][1], chunks[i][3], chunks[i][5]});
    chunk_rtn[i * 2] = decompressor->use_bitstream(m_bitstream_ptr + m_offsets[i],
                                                   m_offsets[i + 1] - m_offsets[i]);
    chunk_rtn[i * 2 + 1] = decompressor->decompress();
    const auto& small_vol = decompressor->view_decoded_data();
    m_scatter_chunk(m_vol_buf, m_dims, small_vol, chunks[i]);
  }

  auto fail = std::find_if_not(chunk_rtn.begin(), chunk_rtn.end(),
                               [](auto r) { return r == RTNType::Good; });
  if (fail != chunk_rtn.end())
    return *fail;
  else
    return RTNType::Good;
}

auto sperr::SPERR3D_OMP_D::release_decoded_data() -> sperr::vecd_type&&
{
  m_dims = {0, 0, 0};
  return std::move(m_vol_buf);
}

auto sperr::SPERR3D_OMP_D::view_decoded_data() const -> const sperr::vecd_type&
{
  return m_vol_buf;
}

auto sperr::SPERR3D_OMP_D::get_dims() const -> std::array<size_t, 3>
{
  return m_dims;
}

void sperr::SPERR3D_OMP_D::m_scatter_chunk(vecd_type& big_vol,
                                           dims_type vol_dim,
                                           const vecd_type& small_vol,
                                           std::array<size_t, 6> chunk_info)
{
  size_t idx = 0;
  const auto row_len = chunk_info[1];

  for (size_t z = chunk_info[4]; z < chunk_info[4] + chunk_info[5]; z++) {
    const size_t plane_offset = z * vol_dim[0] * vol_dim[1];
    for (size_t y = chunk_info[2]; y < chunk_info[2] + chunk_info[3]; y++) {
      const auto start_i = plane_offset + y * vol_dim[0] + chunk_info[0];
      std::copy(small_vol.begin() + idx, small_vol.begin() + idx + row_len,
                big_vol.begin() + start_i);
      idx += row_len;
    }
  }
}
