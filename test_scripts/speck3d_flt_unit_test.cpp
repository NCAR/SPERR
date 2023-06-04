#include "SPECK3D_FLT.h"

#include "gtest/gtest.h"

#include <algorithm>
#include <cstdio>
#include <iostream>

namespace {

//
// Test a constant data field
//
TEST(SPECK3D_FLT, ConstantField)
{
  const auto dims = sperr::dims_type{12, 13, 15};
  const auto total_vals = 12 * 13 * 15;
  auto tmp = std::vector<double>(total_vals, 4.332);
  auto input = tmp;

  auto encoder = sperr::SPECK3D_FLT();
  encoder.set_dims(dims);
  encoder.set_q(1.2e-2);
  encoder.take_data(std::move(tmp));
  auto rtn = encoder.compress();
  ASSERT_EQ(rtn, sperr::RTNType::Good);
  auto bitstream = sperr::vec8_type();
  encoder.append_encoded_bitstream(bitstream);
  EXPECT_EQ(bitstream.size(), 17);  // Constant storage to record a constant field.

  auto decoder = sperr::SPECK3D_FLT();
  decoder.set_dims(dims);
  decoder.set_q(1.2e-2);
  rtn = decoder.use_bitstream(bitstream.data(), bitstream.size());
  ASSERT_EQ(rtn, sperr::RTNType::Good);
  rtn = decoder.decompress();
  ASSERT_EQ(rtn, sperr::RTNType::Good);
  auto output = decoder.release_decoded_data();
  EXPECT_EQ(input, output);
}

//
// Test a minimal data set
//
TEST(SPECK3D_FLT, IntegerLen)
{
  auto inputf = sperr::read_whole_file<float>("../test_data/wmag17.float");
  const auto dims = sperr::dims_type{17, 17, 17};
  const auto total_vals = inputf.size();
  auto inputd = sperr::vecd_type(total_vals);
  std::copy(inputf.cbegin(), inputf.cend(), inputd.begin());
  double q = 1.5e-1;
  
  // Encode
  auto encoder = sperr::SPECK3D_FLT();
  encoder.set_dims(dims);
  encoder.set_q(q);
  encoder.copy_data(inputd.data(), total_vals);
  auto rtn = encoder.compress();
  ASSERT_EQ(rtn, sperr::RTNType::Good);
  auto bitstream = sperr::vec8_type();
  encoder.append_encoded_bitstream(bitstream);

  // Decode
  auto decoder = sperr::SPECK3D_FLT();
  decoder.set_dims(dims);
  decoder.set_q(q);
  rtn = decoder.use_bitstream(bitstream.data(), bitstream.size());
  ASSERT_EQ(rtn, sperr::RTNType::Good);
  rtn = decoder.decompress();
  ASSERT_EQ(rtn, sperr::RTNType::Good);
  auto outputd = decoder.release_decoded_data();
#ifdef PRINT
  auto stats = sperr::calc_stats(inputd.data(), outputd.data(), total_vals);
  std::printf("bpp = %.2f, PSNR = %.2f\n", 8.0 * bitstream.size() / total_vals, stats[2]);
#endif
  EXPECT_EQ(encoder.integer_len(), 2);
  EXPECT_EQ(decoder.integer_len(), 2);

  // Test a new q value
  q = 3.0;
  encoder.set_q(q);
  encoder.copy_data(inputd.data(), total_vals);
  encoder.compress();
  bitstream.clear();
  encoder.append_encoded_bitstream(bitstream);
  decoder.set_q(q);
  decoder.use_bitstream(bitstream.data(), bitstream.size());
  decoder.decompress();
  outputd = decoder.release_decoded_data();
#ifdef PRINT
  stats = sperr::calc_stats(inputd.data(), outputd.data(), total_vals);
  std::printf("bpp = %.2f, PSNR = %.2f\n", 8.0 * bitstream.size() / total_vals, stats[2]);
#endif
  EXPECT_EQ(encoder.integer_len(), 1);
  EXPECT_EQ(decoder.integer_len(), 1);

  // Test a new q value
  q = 3e-3;
  encoder.set_q(q);
  encoder.copy_data(inputd.data(), total_vals);
  encoder.compress();
  bitstream.clear();
  encoder.append_encoded_bitstream(bitstream);
  decoder.set_q(q);
  decoder.use_bitstream(bitstream.data(), bitstream.size());
  decoder.decompress();
  outputd = decoder.release_decoded_data();
#ifdef PRINT
  stats = sperr::calc_stats(inputd.data(), outputd.data(), total_vals);
  std::printf("bpp = %.2f, PSNR = %.2f\n", 8.0 * bitstream.size() / total_vals, stats[2]);
#endif
  EXPECT_EQ(encoder.integer_len(), 4);
  EXPECT_EQ(decoder.integer_len(), 4);

  // Test a new q value
  q = 1e-8;
  encoder.set_q(q);
  encoder.copy_data(inputd.data(), total_vals);
  encoder.compress();
  bitstream.clear();
  encoder.append_encoded_bitstream(bitstream);
  decoder.set_q(q);
  decoder.use_bitstream(bitstream.data(), bitstream.size());
  decoder.decompress();
  outputd = decoder.release_decoded_data();
#ifdef PRINT
  stats = sperr::calc_stats(inputd.data(), outputd.data(), total_vals);
  std::printf("bpp = %.2f, PSNR = %.2f\n", 8.0 * bitstream.size() / total_vals, stats[2]);
#endif
  EXPECT_EQ(encoder.integer_len(), 8);
  EXPECT_EQ(decoder.integer_len(), 8);
}

//
// Test outlier correction
//
#if 0
TEST(SPECK3D_FLT, OutlierCorrection)
{
  auto inputf = sperr::read_whole_file<float>("../test_data/vorticity.128_128_41");
  const auto dims = sperr::dims_type{128, 128, 41};
  const auto total_vals = inputf.size();
  auto inputd = sperr::vecd_type(total_vals);
  std::copy(inputf.cbegin(), inputf.cend(), inputd.begin());
  double tol = 1.5e-1;
  
  // Encode
  auto encoder = sperr::SPECK3D_FLT();
  encoder.set_dims(dims);
  encoder.set_tolerance(tol);
  encoder.copy_data(inputd.data(), total_vals);
  auto rtn = encoder.compress();
  ASSERT_EQ(rtn, sperr::RTNType::Good);
  auto bitstream = sperr::vec8_type();
  encoder.append_encoded_bitstream(bitstream);

  // Decode
  auto decoder = sperr::SPECK3D_FLT();
  decoder.set_dims(dims);
  decoder.set_tolerance(tol);
  rtn = decoder.use_bitstream(bitstream.data(), bitstream.size());
  ASSERT_EQ(rtn, sperr::RTNType::Good);
  rtn = decoder.decompress();
  ASSERT_EQ(rtn, sperr::RTNType::Good);
  auto outputd = decoder.release_decoded_data();
#ifdef PRINT
  auto stats = sperr::calc_stats(inputd.data(), outputd.data(), total_vals);
  std::printf("bpp = %.2f, PSNR = %.2f\n", 8.0 * bitstream.size() / total_vals, stats[2]);
#endif
  EXPECT_EQ(encoder.integer_len(), 2);
  EXPECT_EQ(decoder.integer_len(), 2);
}
#endif

}  // namespace
