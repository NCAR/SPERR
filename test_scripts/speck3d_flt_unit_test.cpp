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
  const auto total_vals = dims[0] * dims[1] * dims[2];
  auto input = std::vector<double>(total_vals, 4.332);
  auto tmp = input;

  auto encoder = sperr::SPECK3D_FLT();
  encoder.set_dims(dims);
  encoder.set_tolerance(1.2e-2);
  encoder.take_data(std::move(tmp));
  auto rtn = encoder.compress();
  ASSERT_EQ(rtn, sperr::RTNType::Good);
  auto bitstream = sperr::vec8_type();
  encoder.append_encoded_bitstream(bitstream);
  EXPECT_EQ(bitstream.size(), 17);  // Constant storage to record a constant field.

  auto decoder = sperr::SPECK3D_FLT();
  decoder.set_dims(dims);
  rtn = decoder.use_bitstream(bitstream.data(), bitstream.size());
  ASSERT_EQ(rtn, sperr::RTNType::Good);
  rtn = decoder.decompress();
  ASSERT_EQ(rtn, sperr::RTNType::Good);
  auto output = decoder.release_decoded_data();
  EXPECT_EQ(input, output);

  // Test specifying PSNR
  encoder.set_dims(dims);
  encoder.set_psnr(89.0);
  tmp = input;
  encoder.take_data(std::move(tmp));
  rtn = encoder.compress();
  ASSERT_EQ(rtn, sperr::RTNType::Good);
  bitstream.clear(); 
  encoder.append_encoded_bitstream(bitstream);
  EXPECT_EQ(bitstream.size(), 17);  // Constant storage to record a constant field.

  decoder.set_dims(dims);
  rtn = decoder.use_bitstream(bitstream.data(), bitstream.size());
  ASSERT_EQ(rtn, sperr::RTNType::Good);
  rtn = decoder.decompress();
  ASSERT_EQ(rtn, sperr::RTNType::Good);
  output = decoder.release_decoded_data();
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
  double psnr = 40.0;
  
  // Encode
  auto encoder = sperr::SPECK3D_FLT();
  encoder.set_dims(dims);
  encoder.set_psnr(psnr);
  encoder.copy_data(inputd.data(), total_vals);
  auto rtn = encoder.compress();
  ASSERT_EQ(rtn, sperr::RTNType::Good);
  auto bitstream = sperr::vec8_type();
  encoder.append_encoded_bitstream(bitstream);

  // Decode
  auto decoder = sperr::SPECK3D_FLT();
  decoder.set_dims(dims);
  rtn = decoder.use_bitstream(bitstream.data(), bitstream.size());
  ASSERT_EQ(rtn, sperr::RTNType::Good);
  rtn = decoder.decompress();
  ASSERT_EQ(rtn, sperr::RTNType::Good);
  auto outputd = decoder.release_decoded_data();
#ifdef PRINT
  auto stats = sperr::calc_stats(inputd.data(), outputd.data(), total_vals);
  std::printf("bpp = %.2f, PSNR = %.2f\n", 8.0 * bitstream.size() / total_vals, stats[2]);
#endif
  EXPECT_EQ(encoder.integer_len(), 1);
  EXPECT_EQ(decoder.integer_len(), 1);

  // Test a new PSNR target.
  psnr = 50.0;
  encoder.set_psnr(psnr);
  encoder.copy_data(inputd.data(), total_vals);
  encoder.compress();
  bitstream.clear();
  encoder.append_encoded_bitstream(bitstream);
  decoder.use_bitstream(bitstream.data(), bitstream.size());
  decoder.decompress();
  outputd = decoder.release_decoded_data();
#ifdef PRINT
  stats = sperr::calc_stats(inputd.data(), outputd.data(), total_vals);
  std::printf("bpp = %.2f, PSNR = %.2f\n", 8.0 * bitstream.size() / total_vals, stats[2]);
#endif
  EXPECT_EQ(encoder.integer_len(), 2);
  EXPECT_EQ(decoder.integer_len(), 2);

  // Test a new PSNR target.
  psnr = 190.0;
  encoder.set_psnr(psnr);
  encoder.copy_data(inputd.data(), total_vals);
  encoder.compress();
  bitstream.clear();
  encoder.append_encoded_bitstream(bitstream);
  decoder.use_bitstream(bitstream.data(), bitstream.size());
  decoder.decompress();
  outputd = decoder.release_decoded_data();
#ifdef PRINT
  stats = sperr::calc_stats(inputd.data(), outputd.data(), total_vals);
  std::printf("bpp = %.2f, PSNR = %.2f\n", 8.0 * bitstream.size() / total_vals, stats[2]);
#endif
  EXPECT_EQ(encoder.integer_len(), 4);
  EXPECT_EQ(decoder.integer_len(), 4);

  // Test a new PSNR target.
  psnr = 210.0;
  encoder.set_psnr(psnr);
  encoder.copy_data(inputd.data(), total_vals);
  encoder.compress();
  bitstream.clear();
  encoder.append_encoded_bitstream(bitstream);
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
TEST(SPECK3D_FLT, OutlierCorrection)
{
  auto inputf = sperr::read_whole_file<float>("../test_data/vorticity.128_128_41");
  const auto dims = sperr::dims_type{128, 128, 41};
  const auto total_vals = inputf.size();
  auto inputd = sperr::vecd_type(total_vals);
  std::copy(inputf.cbegin(), inputf.cend(), inputd.begin());
  double tol = 1.0e-5;
  
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
  for (size_t i = 0; i < inputd.size(); i++)
    EXPECT_NEAR(inputd[i], outputd[i], tol);

  //
  // Test a smaller-than-32-bit-epsilon tolerance
  //
  tol = 2.9e-9;
  encoder.set_tolerance(tol);
  encoder.copy_data(inputd.data(), total_vals);
  rtn = encoder.compress();
  ASSERT_EQ(rtn, sperr::RTNType::Good);
  bitstream.clear();
  encoder.append_encoded_bitstream(bitstream);

  decoder.set_tolerance(tol);
  rtn = decoder.use_bitstream(bitstream.data(), bitstream.size());
  ASSERT_EQ(rtn, sperr::RTNType::Good);
  rtn = decoder.decompress();
  ASSERT_EQ(rtn, sperr::RTNType::Good);
  outputd = decoder.release_decoded_data();
#ifdef PRINT
  stats = sperr::calc_stats(inputd.data(), outputd.data(), total_vals);
  std::printf("bpp = %.2f, PSNR = %.2f\n", 8.0 * bitstream.size() / total_vals, stats[2]);
#endif
  for (size_t i = 0; i < inputd.size(); i++)
    EXPECT_NEAR(inputd[i], outputd[i], tol);

  //
  // Test a big tolerance which essentially produces all zero integer arrays. 
  // The compression should still carry on, and tolerance being honored.
  //
  tol = 1e-2;
  encoder.set_tolerance(tol);
  encoder.copy_data(inputd.data(), total_vals);
  rtn = encoder.compress();
  ASSERT_EQ(rtn, sperr::RTNType::Good);
  bitstream.clear();
  encoder.append_encoded_bitstream(bitstream);

  decoder.set_tolerance(tol);
  rtn = decoder.use_bitstream(bitstream.data(), bitstream.size());
  ASSERT_EQ(rtn, sperr::RTNType::Good);
  rtn = decoder.decompress();
  ASSERT_EQ(rtn, sperr::RTNType::Good);
  outputd = decoder.release_decoded_data();
#ifdef PRINT
  stats = sperr::calc_stats(inputd.data(), outputd.data(), total_vals);
  std::printf("bpp = %.2f, PSNR = %.2f\n", 8.0 * bitstream.size() / total_vals, stats[2]);
  std::printf("  -- bitstream size = %lu bytes\n", bitstream.size());
#endif
  for (size_t i = 0; i < inputd.size(); i++)
    EXPECT_NEAR(inputd[i], outputd[i], tol);
}

}  // namespace
