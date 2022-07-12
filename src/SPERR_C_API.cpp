#include "SPERR_C_API.h"

#include "SPERR2D_Compressor.h"
#include "SPERR2D_Decompressor.h"

#include "SPERR3D_OMP_C.h"
#include "SPERR3D_OMP_D.h"


int C_API::sperr_comp_2d(const void* src,
                        int32_t is_float,
                        size_t dimx,
                        size_t dimy,
                        int32_t mode,
                        double quality,
                        void** dst,
                        size_t* dst_len)
{
  // Examine if `dst` is pointing to a NULL pointer
  if (*dst != NULL)
    return 1;

  // Examine if `mode` and `quality` are valid
  if (mode < 1 || mode > 3 || quality <= 0.0)
    return 2;
    
  // Setup the compressor
  const auto total_vals = dimx * dimy;
  auto compressor = SPERR2D_Compressor();
  auto rtn = sperr::RTNType::Good;
  switch (is_float) {
    case 0:  // double
      rtn = compressor.copy_data(static_cast<const double*>(src), total_vals, {dimx, dimy, 1});
      break;
    case 1:  // float
      rtn = compressor.copy_data(static_cast<const float*>(src), total_vals, {dimx, dimy, 1});
      break;
    default:
      rtn = RTNType::Error;
  }
  if (rtn != RTNType::Good)
    return -1;

  // Specify a particular compression mode.
  switch (mode) {
    case 1:
      compressor.set_target_bpp(quality);
      break;
    case 2:
      compressor.set_target_psnr(quality);
      break;
    case 3:
      compressor.set_target_pwe(quality);
      break;
    default:
      return 2;
  }

  // Do the actual compression work!
  rtn = compressor.compress();
  if (rtn != RTNType::Good)
    return -1;

  // Output the compressed bitstream
  const auto& stream = compressor.view_encoded_bitstream();
  if (stream.empty())
    return -1;
  *dst_len = stream.size();
  uint8_t* buf = (uint8_t*)std::malloc(stream.size());
  std::copy(stream.cbegin(), stream.cend(), buf);
  *dst = buf;

  return 0;
}


int C_API::sperr_comp_3d(const void* src,
                         int32_t is_float,
                         size_t dimx,
                         size_t dimy,
                         size_t dimz,
                         int32_t mode,
                         double quality,
                         size_t nthreads,
                         void** dst,
                         size_t* dst_len)
{
  // Examine if `dst` is pointing to a NULL pointer
  if (*dst != NULL)
    return 1;

  // Examine if `mode` and `quality` are valid
  if (mode < 1 || mode > 3 || quality <= 0.0)
    return 2;

  // Hard code the preferred chunk size as 256^3. Can change in the future.
  auto chunk_dims = sperr::dims_type{256, 256, 256};

  // Setup the compressor
  const auto total_vals = dimx * dimy * dimz;
  auto compressor = SPERR3D_OMP_C();
  compressor.set_num_threads(nthreads);
  auto rtn = sperr::RTNType::Good;
  switch (is_float) {
    case 0:  // double
      rtn = compressor.copy_data(static_cast<const double*>(src), total_vals, {dimx, dimy, dimz},
                                 chunk_dims);
      break;
    case 1:  // float
      rtn = compressor.copy_data(static_cast<const float*>(src), total_vals, {dimx, dimy, dimz},
                                 chunk_dims);
      break;
    default:
      rtn = RTNType::Error;
  }
  if (rtn != RTNType::Good)
    return -1;

  // Specify a particular compression mode.
  switch (mode) {
    case 1:
      compressor.set_target_bpp(quality);
      break;
    case 2:
      compressor.set_target_psnr(quality);
      break;
    case 3:
      compressor.set_target_pwe(quality);
      break;
    default:
      return 2;
  }

  // Do the actual compression work
  rtn = compressor.compress();
  if (rtn != RTNType::Good)
    return -1;

  // Output the compressed bitstream
  const auto stream = compressor.get_encoded_bitstream();
  if (stream.empty())
    return -1;
  *dst_len = stream.size();
  uint8_t* buf = (uint8_t*)std::malloc(stream.size());
  std::copy(stream.cbegin(), stream.cend(), buf);
  *dst = buf;

  return 0;
}


int C_API::sperr_decomp_2d(const void* src,
                           size_t src_len,
                           int32_t output_float,
                           size_t* dimx,
                           size_t* dimy,
                           void** dst)
{
  // Examine if `dst` is pointing to a NULL pointer
  if (*dst != NULL)
    return 1;

  // Use a decompressor to decompress this bitstream
  auto decompressor = SPERR2D_Decompressor();
  auto rtn = decompressor.use_bitstream(src, src_len);
  if (rtn != RTNType::Good)
    return -1;
  rtn = decompressor.decompress();
  if (rtn != RTNType::Good)
    return -1;

  // Double check that the slice dimension is correct
  const auto& slice = decompressor.view_data();
  const auto dims = decompressor.get_dims();
  if (slice.size() != dims[0] * dims[1])
    return -1;
  *dimx = dims[0];
  *dimy = dims[1];

  // write out the 2D slice in double or float format
  switch (output_float) {
    case 0: {  // double
      double* buf = (double*)std::malloc(slice.size() * sizeof(double));
      std::copy(slice.cbegin(), slice.cend(), buf);
      *dst = buf;
      break;
    }
    case 1: {  // float
      float* buf = (float*)std::malloc(slice.size() * sizeof(float));
      std::copy(slice.cbegin(), slice.cend(), buf);
      *dst = buf;
      break;
    }
    default:
      return 2;
  }

  return 0;
}

int C_API::sperr_decomp_3d(const void* src,
                           size_t src_len,
                           int32_t output_float,
                           int32_t nthreads,
                           size_t* dimx,
                           size_t* dimy,
                           size_t* dimz,
                           void** dst)
{
  // Examine if `dst` is pointing to a NULL pointer
  if (*dst != NULL)
    return 1;

  if (nthreads <= 0)
    return -1;

  // Use a decompressor to decompress this bitstream
  auto decompressor = SPERR3D_OMP_D();
  decompressor.set_num_threads(nthreads);
  auto rtn = decompressor.use_bitstream(src, src_len);
  if (rtn != RTNType::Good)
    return -1;
  rtn = decompressor.decompress(src);
  if (rtn != RTNType::Good)
    return -1;

  // Double check that the volume dimension is correct
  const auto& vol = decompressor.view_data();
  const auto dims = decompressor.get_dims();
  if (vol.size() != dims[0] * dims[1] * dims[2])
    return -1;
  *dimx = dims[0];
  *dimy = dims[1];
  *dimz = dims[2];

  // write out the volume in double or float format
  switch (output_float) {
    case 0: {  // double
      double* buf = (double*)std::malloc(vol.size() * sizeof(double));
      std::copy(vol.cbegin(), vol.cend(), buf);
      *dst = buf;
      break;
    }
    case 1: {  // float
      float* buf = (float*)std::malloc(vol.size() * sizeof(float));
      std::copy(vol.cbegin(), vol.cend(), buf);
      *dst = buf;
      break;
    }
    default:
      return 2;
  }

  return 0;
}

void C_API::sperr_parse_header(const void* ptr,
                               int32_t* version_major,
                               int32_t* zstd_applied,
                               int32_t* is_qz_term,
                               int32_t* is_3d,
                               uint32_t* dim_x,
                               uint32_t* dim_y,
                               uint32_t* dim_z)
{
  auto header = sperr::parse_header(ptr);

  *version_major = header.version_major;
  *zstd_applied = header.zstd_applied;
  *is_qz_term = header.is_qz_term;
  *is_3d = header.is_3d;
  *dim_x = header.vol_dims[0];
  *dim_y = header.vol_dims[1];
  *dim_z = header.vol_dims[2];
}
