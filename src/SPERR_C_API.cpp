#include <cassert>

#include "SPERR_C_API.h"

#include "SPECK2D_FLT.h"
#include "SPERR3D_OMP_C.h"
#include "SPERR3D_OMP_D.h"

#include "SPERR3D_Stream_Tools.h"

int C_API::sperr_comp_2d(const void* src,
                         int is_float,
                         size_t dimx,
                         size_t dimy,
                         int mode,
                         double quality,
                         void** dst,
                         size_t* dst_len)
{
  // Examine if `dst` is pointing to a NULL pointer
  if (*dst != NULL)
    return 1;
  if (quality <= 0.0)
    return 2;

  // The actual encoding steps are just the same as in `utilities/sperr2d.cpp`.
  auto encoder = std::make_unique<sperr::SPECK2D_FLT>();
  encoder->set_dims({dimx, dimy, 1});
  switch (is_float) {
    case 0:  // double
      encoder->copy_data(static_cast<const double*>(src), dimx * dimy);
      break;
    case 1:  // float
      encoder->copy_data(static_cast<const float*>(src), dimx * dimy);
      break;
    default:
      return 3;
  }
  switch (mode) {
    case 1:  // fixed bitrate
      encoder->set_bitrate(quality);
      break;
    case 2:  // fixed PSNR
      encoder->set_psnr(quality);
      break;
    case 3:  // fixed PWE
      encoder->set_tolerance(quality);
      break;
    default:
      return 2;
  }
  auto rtn = encoder->compress();
  if (rtn != sperr::RTNType::Good)
    return -1;

  // Assemble the output bitstream.
  auto stream = sperr::vec8_type();
  encoder->append_encoded_bitstream(stream);
  encoder.reset();  // Free up some memory.

  // Note: prepend two uint32_t values to record slice dimensions.
  auto dims = std::array{static_cast<uint32_t>(dimx), static_cast<uint32_t>(dimy)};
  *dst_len = stream.size() + sizeof(dims);
  auto* buf = (uint8_t*)std::malloc(*dst_len);
  std::memcpy(buf, &dims, sizeof(dims));
  std::copy(stream.cbegin(), stream.cend(), buf + sizeof(dims));
  *dst = buf;

  return 0;
}

int C_API::sperr_decomp_2d(const void* src,
                           size_t src_len,
                           int output_float,
                           size_t* dimx,
                           size_t* dimy,
                           void** dst)
{
  // Examine if `dst` is pointing to a NULL pointer
  if (*dst != NULL)
    return 1;

  // Extract the slice dimensions.
  auto dims = std::array<uint32_t, 2>();
  std::memcpy(dims.data(), src, sizeof(dims));
  *dimx = dims[0];
  *dimy = dims[1];

  // Use a decoder, similar steps as in `utilities/sperr2d.cpp`.
  auto decoder = std::make_unique<sperr::SPECK2D_FLT>();
  decoder->set_dims({*dimx, *dimy, 1});
  const auto* srcp = static_cast<const uint8_t*>(src);
  decoder->use_bitstream(srcp + sizeof(dims), src_len - sizeof(dims));
  auto rtn = decoder->decompress();
  if (rtn != sperr::RTNType::Good)
    return -1;
  auto outputd = decoder->release_decoded_data();
  assert(outputd.size() == size_t{dims[0]} * dims[1]);
  decoder.reset();

  // Provide decompressed data to `dst`.
  switch (output_float) {
    case 0: {  // double
      auto* buf = (double*)std::malloc(outputd.size() * sizeof(double));
      std::copy(outputd.cbegin(), outputd.cend(), buf);
      *dst = buf;
      break;
    }
    case 1: {  // float
      auto* buf = (float*)std::malloc(outputd.size() * sizeof(float));
      std::copy(outputd.cbegin(), outputd.cend(), buf);
      *dst = buf;
      break;
    }
    default:
      return 2;
  }

  return 0;
}

int C_API::sperr_comp_3d(const void* src,
                         int is_float,
                         size_t dimx,
                         size_t dimy,
                         size_t dimz,
                         size_t chunk_x,
                         size_t chunk_y,
                         size_t chunk_z,
                         int mode,
                         double quality,
                         size_t nthreads,
                         void** dst,
                         size_t* dst_len)
{
  // Examine if `dst` is pointing to a NULL pointer
  if (*dst != NULL)
    return 1;
  if (quality <= 0.0)
    return 2;

  const auto dims = sperr::dims_type{dimx, dimy, dimz};
  const auto chunks = sperr::dims_type{chunk_x, chunk_y, chunk_z};

  // Setup the compressor. Very similar steps as in `utilities/sperr3d.cpp`.
  const auto total_vals = dimx * dimy * dimz;
  auto encoder = std::make_unique<sperr::SPERR3D_OMP_C>();
  encoder->set_dims_and_chunks(dims, chunks);
  encoder->set_num_threads(nthreads);
  switch (mode) {
    case 1:  // fixed bitrate
      encoder->set_bitrate(quality);
      break;
    case 2:  // fixed PSNR
      encoder->set_psnr(quality);
      break;
    case 3:  // fixed PWE
      encoder->set_tolerance(quality);
      break;
    default:
      return 2;
  }
  auto rtn = sperr::RTNType::Good;
  switch (is_float) {
    case 0:  // double
      rtn = encoder->compress(static_cast<const double*>(src), total_vals);
      break;
    case 1:  // float
      rtn = encoder->compress(static_cast<const float*>(src), total_vals);
      break;
    default:
      rtn = sperr::RTNType::Error;
  }
  if (rtn != sperr::RTNType::Good)
    return -1;

  // Prepare the compressed bitstream.
  auto stream = encoder->get_encoded_bitstream();
  if (stream.empty())
    return -1;
  encoder.reset();
  *dst_len = stream.size();
  auto* buf = (uint8_t*)std::malloc(stream.size());
  std::copy(stream.cbegin(), stream.cend(), buf);
  *dst = buf;

  return 0;
}

int C_API::sperr_decomp_3d(const void* src,
                           size_t src_len,
                           int output_float,
                           size_t nthreads,
                           size_t* dimx,
                           size_t* dimy,
                           size_t* dimz,
                           void** dst)
{
  // Examine if `dst` is pointing to a NULL pointer.
  if (*dst != NULL)
    return 1;

  // Use a decompressor to decompress this bitstream
  auto decoder = std::make_unique<sperr::SPERR3D_OMP_D>();
  decoder->set_num_threads(nthreads);
  decoder->setup_decomp(src, src_len);
  auto rtn = decoder->decompress(src);
  if (rtn != sperr::RTNType::Good)
    return -1;
  auto dims = decoder->get_dims();
  auto outputd = decoder->release_decoded_data();
  decoder.reset();

  // Provide the decompressed volume.
  *dimx = dims[0];
  *dimy = dims[1];
  *dimz = dims[2];
  switch (output_float) {
    case 0: {  // double
      auto* buf = (double*)std::malloc(outputd.size() * sizeof(double));
      std::copy(outputd.cbegin(), outputd.cend(), buf);
      *dst = buf;
      break;
    }
    case 1: {  // float
      auto* buf = (float*)std::malloc(outputd.size() * sizeof(float));
      std::copy(outputd.cbegin(), outputd.cend(), buf);
      *dst = buf;
      break;
    }
    default:
      return 2;
  }

  return 0;
}

int C_API::sperr_trunc_3d(const void* src,
                          size_t src_len,
                          unsigned pct,
                          void** dst,
                          size_t* dst_len)
{
  if (*dst != NULL)
    return 1;

  auto tools = sperr::SPERR3D_Stream_Tools();
  auto trunc = tools.progressive_truncate(src, src_len, pct);
  if (trunc.empty())
    return -1;
  else {
    auto* buf = (uint8_t*)std::malloc(trunc.size());
    std::copy(trunc.cbegin(), trunc.cend(), buf);
    *dst = buf;
    *dst_len = trunc.size();
    return 0;
  }
}
