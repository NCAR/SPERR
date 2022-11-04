#include "SPERR3D_INT_Driver.h"

#include "gtest/gtest.h"

#include <algorithm>
#include <cstdio>

namespace {


//
// Test a minimal test case
//
TEST(SPERR3DInt, minimal)
{
  auto inputf = sperr::read_whole_file<float>("../test_data/wmag128.float");
  const auto dims = sperr::dims_type{128, 128, 128};
  const auto total_vals = inputf.size();
  auto inputd = sperr::vecd_type(total_vals);
  std::copy(inputf.cbegin(), inputf.cend(), inputd.begin());
  double pwe = 1e-2;
  
  // Encode
  auto encoder = sperr::SPERR3D_INT_Driver();
  encoder.set_dims(dims);
  encoder.set_pwe(pwe);
  encoder.copy_data(inputd.data(), total_vals);
  encoder.compress();
  auto bitstream = encoder.release_encoded_bitstream();

  // Decode
  auto decoder = sperr::SPERR3D_INT_Driver();
  decoder.set_dims(dims);
  decoder.set_pwe(pwe);
  decoder.use_bitstream(bitstream.data(), bitstream.size());
  decoder.decompress();
  auto outputd = decoder.release_decoded_data();

  auto stats = sperr::calc_stats(inputd.data(), outputd.data(), total_vals);
  auto bpp = 8.0 * bitstream.size() / total_vals;
  std::printf("bpp = %.2f, PSNR = %.2f\n", bpp, stats[2]);

  EXPECT_EQ(inputd.size(), outputd.size());
}



}  // namespace
