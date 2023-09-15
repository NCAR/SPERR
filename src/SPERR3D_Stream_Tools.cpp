#include "SPERR3D_Stream_Tools.h"
#include "Conditioner.h"
#include "SPECK_INT.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstring>
#include <numeric>

auto sperr::SPERR3D_Stream_Tools::get_header_len(std::array<uint8_t, 20> magic) const -> size_t
{
  // Step 1: Decode the 8 booleans, and decide if there are multiple chunks.
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
  auto chunks = sperr::chunk_volume(vdim, cdim);
  const auto num_chunks = chunks.size();
  assert((multi_chunk && num_chunks > 1) || (!multi_chunk && num_chunks == 1));
  size_t header_len = num_chunks * 4;
  if (multi_chunk)
    header_len += m_header_magic_nchunks;
  else
    header_len += m_header_magic_1chunk;

  return header_len;
}

void sperr::SPERR3D_Stream_Tools::populate_stream_info(const void* p)
{
  const auto* const u8p = static_cast<const uint8_t*>(p);

  // Step 1: major version number
  major_version = u8p[0];
  size_t pos = 1;

  // Step 2: unpack 8 booleans.
  const auto b8 = sperr::unpack_8_booleans(u8p[pos++]);
  is_portion = b8[0];
  is_3D = b8[1];
  is_float = b8[2];
  multi_chunk = b8[3];

  // Step 3: volume and chunk dimensions
  uint32_t int3[3] = {0, 0, 0};
  std::memcpy(int3, u8p + pos, sizeof(int3));
  pos += sizeof(int3);
  vol_dims[0] = int3[0];
  vol_dims[1] = int3[1];
  vol_dims[2] = int3[2];
  if (multi_chunk) {
    uint16_t short3[3] = {0, 0, 0};
    std::memcpy(short3, u8p + pos, sizeof(short3));
    pos += sizeof(short3);
    chunk_dims[0] = short3[0];
    chunk_dims[1] = short3[1];
    chunk_dims[2] = short3[2];
  }
  else
    chunk_dims = vol_dims;

  auto chunks = sperr::chunk_volume(vol_dims, chunk_dims);
  const auto num_chunks = chunks.size();
  assert((multi_chunk && num_chunks > 1) || (!multi_chunk && num_chunks == 1));

  // Step 4: derived info!
  if (multi_chunk)
    header_len = m_header_magic_nchunks + num_chunks * 4;
  else
    header_len = m_header_magic_1chunk + num_chunks * 4;

  const auto* chunk_len = reinterpret_cast<const uint32_t*>(u8p + pos);
  stream_len = std::accumulate(chunk_len, chunk_len + num_chunks, header_len);

  chunk_offsets.resize(num_chunks * 2);
  chunk_offsets[0] = header_len;
  chunk_offsets[1] = chunk_len[0];
  for (size_t i = 1; i < num_chunks; i++) {
    chunk_offsets[i * 2] = chunk_offsets[i * 2 - 2] + chunk_offsets[i * 2 - 1];
    chunk_offsets[i * 2 + 1] = chunk_len[i];
  }
}

auto sperr::SPERR3D_Stream_Tools::progressive_read(std::string filename, uint32_t pct) -> vec8_type
{
  // Gather info of the current bitstream.
  auto vec20 = sperr::read_n_bytes(filename, 20);
  auto arr20 = std::array<uint8_t, 20>();
  std::copy(vec20.begin(), vec20.end(), arr20.begin());
  const auto header_len = this->get_header_len(arr20);
  const auto header = sperr::read_n_bytes(filename, header_len);
  this->populate_stream_info(header.data());

  // If the complete bitstream is less than what's requested, return the complete bitstream!
  if (pct == 0 || pct >= 100) {
    auto complete_stream = sperr::read_n_bytes(filename, this->stream_len);
    return complete_stream;
  }

  // Calculate how many bytes to allocate to each chunk, with `m_progressive_min_chunk_bytes`
  //    being the minimal length. The only exception is that when the chunk itself has less
  //    bytes, e.g., when it's a constant chunk.
  assert(chunk_offsets.size() % 2 == 0);
  auto nchunks = chunk_offsets.size() / 2;
  auto chunk_offsets_new = chunk_offsets;

  for (size_t i = 0; i < nchunks; i++) {
    auto orig_len = chunk_offsets[i * 2 + 1];
    // Only touch the value stored at `chunk_offsets_new[i * 2 + 1]` if it's bigger than
    // the minimal number of bytes to keep.
    if (orig_len > m_progressive_min_chunk_bytes) {
      auto request_len = static_cast<size_t>(double(pct) / 100.0 * double(orig_len));
      request_len = std::max(m_progressive_min_chunk_bytes, request_len);
      chunk_offsets_new[i * 2 + 1] = request_len;
    }
  }

  // Calculate the total length of the new bitstream, and read it from disk!
  auto total_len_new = header_len;
  for (size_t i = 0; i < nchunks; i++)
    total_len_new += chunk_offsets_new[i * 2 + 1];
  auto stream_new = vec8_type();
  stream_new.reserve(total_len_new);
  stream_new.resize(header_len);  // Occupy the space for a new header.
  auto rtn = sperr::read_sections(filename, chunk_offsets_new, stream_new);
  if (rtn != RTNType::Good) {
    stream_new.clear();
    return stream_new;
  }

  // Finally, create a proper new header.
  //
  stream_new[0] = static_cast<uint8_t>(SPERR_VERSION_MAJOR);
  size_t pos = 1;
  auto b8 = sperr::unpack_8_booleans(header[pos]);
  b8[0] = true;  // Record that this is a portion of another complete bitstream.
  stream_new[pos++] = sperr::pack_8_booleans(b8);
  // Copy over the volume and chunk dimensions.
  if (multi_chunk) {
    std::copy(header.begin() + pos, header.begin() + m_header_magic_nchunks,
              stream_new.begin() + pos);
    pos = m_header_magic_nchunks;
  }
  else {
    std::copy(header.begin() + pos, header.begin() + m_header_magic_1chunk,
              stream_new.begin() + pos);
    pos = m_header_magic_1chunk;
  }
  // Record the length of bitstreams for each chunk.
  for (size_t i = 0; i < nchunks; i++) {
    uint32_t len = chunk_offsets_new[i * 2 + 1];
    std::memcpy(&stream_new[pos], &len, sizeof(len));
    pos += sizeof(len);
  }
  assert(pos == header_len);

  return stream_new;
}
