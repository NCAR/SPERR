#include "SPERR3D.h"

#include "gtest/gtest.h"

#include <algorithm>
#include <cstdio>
#include <iostream>

namespace {


//
// Test a minimal data set
//
TEST(SPERR3D, minimal)
{
  auto inputf = sperr::read_whole_file<float>("../test_data/wmag128.float");
  const auto dims = sperr::dims_type{128, 128, 128};
  const auto total_vals = inputf.size();
  auto inputd = sperr::vecd_type(total_vals);
  std::copy(inputf.cbegin(), inputf.cend(), inputd.begin());
  double q = 1.5e-3;
  
  // Encode
  auto encoder = sperr::SPERR3D();
  encoder.set_dims(dims);
  encoder.set_q(q);
  encoder.copy_data(inputd.data(), total_vals);
  auto rtn = encoder.compress();
  ASSERT_EQ(rtn, sperr::RTNType::Good);
  auto bitstream = encoder.get_encoded_bitstream();

  // Decode
  auto decoder = sperr::SPERR3D();
  decoder.set_dims(dims);
  decoder.set_q(q);
  rtn = decoder.use_bitstream(bitstream.data(), bitstream.size());
  ASSERT_EQ(rtn, sperr::RTNType::Good);
  rtn = decoder.decompress();
  ASSERT_EQ(rtn, sperr::RTNType::Good);
  auto outputd = decoder.release_decoded_data();

  auto stats = sperr::calc_stats(inputd.data(), outputd.data(), total_vals);
  auto bpp = 8.0 * bitstream.size() / total_vals;
  std::printf("bpp = %.2f, PSNR = %.2f\n", bpp, stats[2]);

  // TODO: add more rigourous tests.
  EXPECT_EQ(inputd.size(), outputd.size());
}


}  // namespace
