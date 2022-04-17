#include "SPERR_C_API.h"

#include "SPECK2D_Compressor.h"
#include "SPECK2D_Decompressor.h"

int sperr_qzcomp_2d(const void* src,
                    int32_t is_float,
                    size_t dimx,
                    size_t dimy,
                    int32_t qlev,
                    double tol,
                    void** dst,
                    size_t* dst_len)
{
  // Examine if `dst` is pointing to a NULL pointer
  if (*dst != NULL)
    return 1;

  // Setup the compressor
  const auto total_vals = dimx * dimy;
  auto compressor = SPECK2D_Compressor();
  auto rtn = sperr::RTNType::Good;
  switch (is_float) {
    case 0:  // double
      rtn = compressor.copy_data(static_cast<const double*>(src), total_vals, {dimx, dimy, 1});
      break;
    case 1:  // float
      rtn = compressor.copy_data(static_cast<const float*>(src), total_vals, {dimx, dimy, 1});
      break;
    default:
      return 3;
  }
  if (rtn != RTNType::Good)
    return -1;
  compressor.set_qz_level(qlev);
  compressor.set_tolerance(tol);

  // Do the actual compression work!
  rtn = compressor.compress();
  switch (rtn) {
    case RTNType::QzLevelTooBig:
      return 2;
    case RTNType::Good:
      break;
    default:
      return -1;
  }

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

int sperr_qzdecomp_2d(const void* src,
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
  auto decompressor = SPECK2D_Decompressor();
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
