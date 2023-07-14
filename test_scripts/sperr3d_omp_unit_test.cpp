#include "SPERR3D_OMP_C.h"
#include "SPERR3D_OMP_D.h"

#include <cstring>
#include "gtest/gtest.h"

namespace {

using sperr::RTNType;

//
// Test constant fields.
//
TEST(sperr3d_constant, one_chunk)
{
  auto input = sperr::read_whole_file<float>("../test_data/const32x20x16.float");
  assert(!input.empty());
  const auto dims = sperr::dims_type{32, 20, 16};
  const auto total_len = dims[0] * dims[1] * dims[2];
  auto inputd = sperr::vecd_type(total_len);
  std::copy(input.cbegin(), input.cend(), inputd.begin());

  // Use an encoder
  auto encoder = sperr::SPERR3D_OMP_C();
  encoder.set_dims_and_chunks(dims, dims);
  encoder.set_psnr(99.0);
  auto rtn = encoder.compress(input.data(), input.size());
  EXPECT_EQ(rtn, RTNType::Good);
  auto stream1 = encoder.get_encoded_bitstream();

  // Use a decoder
  auto decoder = sperr::SPERR3D_OMP_D();
  decoder.setup_decomp(stream1.data(), stream1.size());
  decoder.decompress(stream1.data());
  auto& output = decoder.view_decoded_data();
  auto output_dims = decoder.get_dims();
  EXPECT_EQ(inputd, output);
  EXPECT_EQ(output_dims, dims);

  // Re-use the same objects, and test specifying PWE instead of PSNR.
  encoder.set_tolerance(1.0);
  encoder.compress(input.data(), input.size());
  auto stream2 = encoder.get_encoded_bitstream();

  decoder.setup_decomp(stream2.data(), stream2.size());
  decoder.decompress(stream2.data());
  auto& output2 = decoder.view_decoded_data();
  EXPECT_EQ(inputd, output2);
  EXPECT_EQ(stream2, stream1);
}

TEST(sperr3d_constant, omp_chunks)
{
  auto input = sperr::read_whole_file<float>("../test_data/const32x32x59.float");
  const auto dims = sperr::dims_type{32, 32, 59};
  const auto chunks = sperr::dims_type{32, 16, 32};
  const auto total_len = dims[0] * dims[1] * dims[2];
  auto inputd = sperr::vecd_type(total_len);
  std::copy(input.cbegin(), input.cend(), inputd.begin());

  // Use an encoder
  auto encoder = sperr::SPERR3D_OMP_C();
  encoder.set_dims_and_chunks(dims, chunks);
  encoder.set_psnr(99.0);
  encoder.set_num_threads(3);
  encoder.compress(inputd.data(), inputd.size());
  auto stream = encoder.get_encoded_bitstream();

  // Use a decoder
  auto decoder = sperr::SPERR3D_OMP_D();
  decoder.set_num_threads(4);
  decoder.setup_decomp(stream.data(), stream.size());
  decoder.decompress(stream.data());
  auto& output = decoder.view_decoded_data();
  auto output_dims = decoder.get_dims();
  EXPECT_EQ(inputd, output);
  EXPECT_EQ(output_dims, dims);
}

//
// Test target PWE
//
TEST(sperr3d_target_pwe, small_data_range)
{
  auto input = sperr::read_whole_file<float>("../test_data/vorticity.128_128_41");
  const auto dims = sperr::dims_type{128, 128, 41};
  const auto chunks = sperr::dims_type{64, 64, 41};
  const auto total_len = dims[0] * dims[1] * dims[2];

  // Use an encoder
  double tol = 1.5e-7;
  auto encoder = sperr::SPERR3D_OMP_C();
  encoder.set_dims_and_chunks(dims, chunks);
  encoder.set_tolerance(tol);
  encoder.set_num_threads(4);
  encoder.compress(input.data(), input.size());
  auto stream = encoder.get_encoded_bitstream();

  // Use a decoder
  auto decoder = sperr::SPERR3D_OMP_D();
  decoder.set_num_threads(4);
  decoder.setup_decomp(stream.data(), stream.size());
  decoder.decompress(stream.data());
  auto output_dims = decoder.get_dims();
  EXPECT_EQ(output_dims, dims);
  const auto& output = decoder.view_decoded_data();
  for (size_t i = 0; i < input.size(); i++)
    EXPECT_NEAR(input[i], output[i], tol);

  // Test a new tolerance
  tol = 6.7e-6;
  encoder.set_tolerance(tol);
  encoder.compress(input.data(), input.size());
  stream = encoder.get_encoded_bitstream();

  decoder.setup_decomp(stream.data(), stream.size());
  decoder.decompress(stream.data());
  const auto& output2 = decoder.view_decoded_data();
  for (size_t i = 0; i < input.size(); i++)
    EXPECT_NEAR(input[i], output2[i], tol);
}

TEST(sperr3d_target_pwe, big)
{
  auto input = sperr::read_whole_file<float>("../test_data/wmag128.float");
  const auto dims = sperr::dims_type{128, 128, 128};
  const auto chunks = sperr::dims_type{64, 70, 80};
  const auto total_len = dims[0] * dims[1] * dims[2];

  // Use an encoder
  double tol = 1.5e-2;
  auto encoder = sperr::SPERR3D_OMP_C();
  encoder.set_dims_and_chunks(dims, chunks);
  encoder.set_tolerance(tol);
  encoder.set_num_threads(4);
  encoder.compress(input.data(), input.size());
  auto stream = encoder.get_encoded_bitstream();

  // Use a decoder
  auto decoder = sperr::SPERR3D_OMP_D();
  decoder.set_num_threads(3);
  decoder.setup_decomp(stream.data(), stream.size());
  decoder.decompress(stream.data());
  auto output_dims = decoder.get_dims();
  EXPECT_EQ(output_dims, dims);
  const auto& output = decoder.view_decoded_data();
  for (size_t i = 0; i < input.size(); i++)
    EXPECT_NEAR(input[i], output[i], tol);

  // Test a new tolerance
  tol = 6.7e-1;
  encoder.set_tolerance(tol);
  encoder.compress(input.data(), input.size());
  stream = encoder.get_encoded_bitstream();

  decoder.setup_decomp(stream.data(), stream.size());
  decoder.decompress(stream.data());
  const auto& output2 = decoder.view_decoded_data();
  for (size_t i = 0; i < input.size(); i++)
    EXPECT_NEAR(input[i], output2[i], tol);
}

//
// Test target PSNR
//
TEST(sperr3d_target_psnr, big)
{
  auto input = sperr::read_whole_file<float>("../test_data/wmag128.float");
  const auto dims = sperr::dims_type{128, 128, 128};
  const auto chunks = sperr::dims_type{64, 70, 80};
  const auto total_len = dims[0] * dims[1] * dims[2];
  auto inputd = sperr::vecd_type(total_len);
  std::copy(input.begin(), input.end(), inputd.begin());

  // Use an encoder
  double psnr = 88.0;
  auto encoder = sperr::SPERR3D_OMP_C();
  encoder.set_dims_and_chunks(dims, chunks);
  encoder.set_psnr(psnr);
  encoder.set_num_threads(4);
  encoder.compress(inputd.data(), inputd.size());
  auto stream = encoder.get_encoded_bitstream();

  // Use a decoder
  auto decoder = sperr::SPERR3D_OMP_D();
  decoder.set_num_threads(3);
  decoder.setup_decomp(stream.data(), stream.size());
  decoder.decompress(stream.data());
  auto output_dims = decoder.get_dims();
  EXPECT_EQ(output_dims, dims);
  const auto& output = decoder.view_decoded_data();
  auto stats = sperr::calc_stats(inputd.data(), output.data(), total_len, 4);
  EXPECT_GT(stats[2], 92.1366);
  EXPECT_LT(stats[2], 92.1367);

  // Test a new psnr
  psnr = 130.0;
  encoder.set_psnr(psnr);
  encoder.compress(input.data(), input.size());
  stream = encoder.get_encoded_bitstream();

  decoder.setup_decomp(stream.data(), stream.size());
  decoder.decompress(stream.data());
  const auto& output2 = decoder.view_decoded_data();
  stats = sperr::calc_stats(inputd.data(), output2.data(), total_len, 4);
  EXPECT_GT(stats[2], 134.3939);
  EXPECT_LT(stats[2], 134.3940);
}

TEST(sperr3d_target_psnr, small_data_range)
{
  auto input = sperr::read_whole_file<float>("../test_data/vorticity.128_128_41");
  const auto dims = sperr::dims_type{128, 128, 41};
  const auto chunks = sperr::dims_type{64, 64, 64};
  const auto total_len = dims[0] * dims[1] * dims[2];
  auto inputd = sperr::vecd_type(total_len);
  std::copy(input.begin(), input.end(), inputd.begin());

  // Use an encoder
  double psnr = 88.0;
  auto encoder = sperr::SPERR3D_OMP_C();
  encoder.set_dims_and_chunks(dims, chunks);
  encoder.set_psnr(psnr);
  encoder.set_num_threads(3);
  encoder.compress(inputd.data(), inputd.size());
  auto stream = encoder.get_encoded_bitstream();

  // Use a decoder
  auto decoder = sperr::SPERR3D_OMP_D();
  decoder.set_num_threads(0);
  decoder.setup_decomp(stream.data(), stream.size());
  decoder.decompress(stream.data());
  auto output_dims = decoder.get_dims();
  EXPECT_EQ(output_dims, dims);
  const auto& output = decoder.view_decoded_data();
  auto stats = sperr::calc_stats(inputd.data(), output.data(), total_len, 4);
  EXPECT_GT(stats[2], 89.0560);
  EXPECT_LT(stats[2], 89.0561);

  // Test a new psnr
  psnr = 125.0;
  encoder.set_psnr(psnr);
  encoder.compress(input.data(), input.size());
  stream  = encoder.get_encoded_bitstream();

  decoder.setup_decomp(stream.data(), stream.size());
  decoder.decompress(stream.data());
  const auto& output2 = decoder.view_decoded_data();
  stats = sperr::calc_stats(inputd.data(), output2.data(), total_len, 4);
  EXPECT_GT(stats[2], 126.0384);
  EXPECT_LT(stats[2], 126.0385);
}

#if 0
//
// Test fixed-size mode
//
TEST(sperr3d_bit_rate, small)
{
  speck_tester_omp tester("../test_data/wmag17.float", {17, 17, 17}, 1);

  const auto tar_psnr = std::numeric_limits<double>::max();
  const auto pwe = 0.0;

  tester.execute(4.0, tar_psnr, pwe);
  auto psnr = tester.get_psnr();
  auto lmax = tester.get_lmax();
  EXPECT_FLOAT_EQ(psnr, 52.903);
  EXPECT_LT(lmax, 1.8526);

  tester.execute(2.0, tar_psnr, pwe);
  psnr = tester.get_psnr();
  lmax = tester.get_lmax();
  EXPECT_FLOAT_EQ(psnr, 41.281158);
  EXPECT_LT(lmax, 6.4132);

  tester.execute(1.0, tar_psnr, pwe);
  psnr = tester.get_psnr();
  lmax = tester.get_lmax();
  EXPECT_FLOAT_EQ(psnr, 34.645767);
  EXPECT_LT(lmax, 13.0171);
}

TEST(sperr3d_bit_rate, big)
{
  speck_tester_omp tester("../test_data/wmag128.float", {128, 128, 128}, 0);

  const auto tar_psnr = std::numeric_limits<double>::max();
  const auto pwe = 0.0;

  tester.execute(2.0, tar_psnr, pwe);
  auto psnr = tester.get_psnr();
  auto lmax = tester.get_lmax();
  EXPECT_GT(psnr, 53.8102);
  EXPECT_LT(psnr, 53.8103);
  EXPECT_LT(lmax, 9.6954);

  tester.execute(1.0, tar_psnr, pwe);
  psnr = tester.get_psnr();
  lmax = tester.get_lmax();
  EXPECT_GT(psnr, 47.2219);
  EXPECT_LT(psnr, 47.2220);
  EXPECT_LT(lmax, 16.3006);

  tester.execute(0.5, tar_psnr, pwe);
  psnr = tester.get_psnr();
  lmax = tester.get_lmax();
  EXPECT_GT(psnr, 42.6877);
  EXPECT_LT(psnr, 42.6878);
  EXPECT_LT(lmax, 27.5579);

  tester.execute(0.25, tar_psnr, pwe);
  psnr = tester.get_psnr();
  lmax = tester.get_lmax();
  EXPECT_GT(psnr, 39.2763);
  EXPECT_LT(psnr, 39.2764);
  EXPECT_LT(lmax, 48.8490);
}

TEST(sperr3d_bit_rate, narrow_data_range)
{
  speck_tester_omp tester("../test_data/vorticity.128_128_41", {128, 128, 41}, 2);

  const auto tar_psnr = std::numeric_limits<double>::max();
  const auto pwe = 0.0;

  tester.execute(4.0, tar_psnr, pwe);
  auto psnr = tester.get_psnr();
  auto lmax = tester.get_lmax();
  EXPECT_GT(psnr, 67.7939);
  EXPECT_LT(psnr, 67.7940);
  EXPECT_LT(lmax, 1.17879e-06);

  tester.execute(2.0, tar_psnr, pwe);
  psnr = tester.get_psnr();
  lmax = tester.get_lmax();
  EXPECT_GT(psnr, 55.7831);
  EXPECT_LT(psnr, 55.7832);
  EXPECT_LT(lmax, 4.71049e-06);

  tester.execute(0.8, tar_psnr, pwe);
  psnr = tester.get_psnr();
  lmax = tester.get_lmax();
  EXPECT_GT(psnr, 47.2464);
  EXPECT_LT(psnr, 47.2465);
  EXPECT_LT(lmax, 1.74502e-05);

  tester.execute(0.4, tar_psnr, pwe);
  psnr = tester.get_psnr();
  lmax = tester.get_lmax();
  EXPECT_GT(psnr, 43.1739);
  EXPECT_LT(psnr, 43.1740);
  EXPECT_LT(lmax, 3.34412e-05);
}
#endif

}  // namespace
