#include "SPERR_C_API.h"

#include "SPECK2D_Compressor.h"
#include "SPECK2D_Decompressor.h"

int sperr_qzcomp_2d_double(const void* src,
                           int32_t is_float,
                           size_t dimx,
                           size_t dimy,
                           int32_t qlev,
                           double tol,
                           void* dst,
                           size_t dst_len,
                           size_t* useful_dst_len)
{
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
  if (stream.size() > dst_len)
    return 1;

  *useful_dst_len = stream.size();
  std::copy(stream.cbegin(), stream.cend(), static_cast<uint8_t*>(dst));
  return 0;
}
