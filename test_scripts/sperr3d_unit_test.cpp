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
  auto inputf = sperr::read_whole_file<float>("../test_data/wmag12.float");
  const auto dims = sperr::dims_type{12, 12, 12};
  const auto total_vals = inputf.size();
  auto inputd = sperr::vecd_type(total_vals);
  std::copy(inputf.cbegin(), inputf.cend(), inputd.begin());
  double pwe = 1e-3;
  
  // Encode
  auto encoder = sperr::SPERR3D();
  encoder.set_dims(dims);
  encoder.set_q(pwe);
  encoder.copy_data(inputd.data(), total_vals);
  encoder.compress();
  auto bitstream = encoder.get_encoded_bitstream();

  // Decode
  auto decoder = sperr::SPERR3D();
  decoder.set_dims(dims);
  decoder.set_q(pwe);
  decoder.use_bitstream(bitstream.data(), bitstream.size());
  decoder.decompress();
  auto outputd = decoder.release_decoded_data();

  auto stats = sperr::calc_stats(inputd.data(), outputd.data(), total_vals);
  auto bpp = 8.0 * bitstream.size() / total_vals;

  // TODO: add more rigourous tests.
  EXPECT_EQ(inputd.size(), outputd.size());
}


}  // namespace
