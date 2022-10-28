#include "SPERR3D_INT_ENC.h"
#include "SPERR3D_INT_DEC.h"

#include <cstring>
#include "gtest/gtest.h"

namespace {

using sperr::RTNType;


//
// Test constant fields.
//
TEST(speck3d_constant, one_chunk)
{
  speck_tester_omp tester("../test_data/const32x20x16.float", {32, 20, 16}, 2);

  const auto tar_psnr = std::numeric_limits<double>::max();
  const auto pwe = 0.0;

  auto rtn = tester.execute(1.0, tar_psnr, pwe);
  EXPECT_EQ(rtn, 0);
  auto psnr = tester.get_psnr();
  auto lmax = tester.get_lmax();
  auto infty = std::numeric_limits<double>::infinity();
  EXPECT_EQ(psnr, infty);
  EXPECT_EQ(lmax, 0.0f);
}

TEST(speck3d_constant, omp_chunks)
{
  speck_tester_omp tester("../test_data/const32x32x59.float", {32, 32, 59}, 8);
  tester.prefer_chunk_dims({32, 32, 32});

  const auto tar_psnr = std::numeric_limits<double>::max();
  const auto pwe = 0.0;

  auto rtn = tester.execute(1.0, tar_psnr, pwe);
  EXPECT_EQ(rtn, 0);
  auto psnr = tester.get_psnr();
  auto lmax = tester.get_lmax();
  auto infty = std::numeric_limits<double>::infinity();
  EXPECT_EQ(psnr, infty);
  EXPECT_EQ(lmax, 0.0f);
}

//
// Test target PWE
//
TEST(speck3d_target_pwe, small)
{
  speck_tester_omp tester("../test_data/wmag17.float", {17, 17, 17}, 1);

  const auto bpp = sperr::max_d;
  const auto target_psnr = sperr::max_d;

  auto pwe = double{0.741};
  tester.execute(bpp, target_psnr, pwe);
  auto lmax = tester.get_lmax();
  EXPECT_LE(lmax, pwe);

  pwe = 0.37;
  tester.execute(bpp, target_psnr, pwe);
  lmax = tester.get_lmax();
  EXPECT_LE(lmax, pwe);

  pwe = 0.07;
  tester.execute(bpp, target_psnr, pwe);
  lmax = tester.get_lmax();
  EXPECT_LE(lmax, pwe);

  pwe = 0.01;
  tester.execute(bpp, target_psnr, pwe);
  lmax = tester.get_lmax();
  EXPECT_LE(lmax, pwe);
}

TEST(speck3d_target_pwe, small_data_range)
{
  speck_tester_omp tester("../test_data/vorticity.128_128_41", {128, 128, 41}, 2);

  const auto bpp = sperr::max_d;
  const auto target_psnr = sperr::max_d;

  auto pwe = double{1.5e-7};
  tester.execute(bpp, target_psnr, pwe);
  auto lmax = tester.get_lmax();
  EXPECT_LE(lmax, pwe);

  pwe = 7.3e-7;
  tester.execute(bpp, target_psnr, pwe);
  lmax = tester.get_lmax();
  EXPECT_LE(lmax, pwe);

  pwe = 6.7e-6;
  tester.execute(bpp, target_psnr, pwe);
  lmax = tester.get_lmax();
  EXPECT_LE(lmax, pwe);
}

TEST(speck3d_target_pwe, big)
{
  speck_tester_omp tester("../test_data/wmag128.float", {128, 128, 128}, 0);

  const auto bpp = sperr::max_d;
  const auto target_psnr = sperr::max_d;

  double pwe = 0.92;
  tester.execute(bpp, target_psnr, pwe);
  auto lmax = tester.get_lmax();
  EXPECT_LE(lmax, pwe);

  pwe = 0.45;
  tester.execute(bpp, target_psnr, pwe);
  lmax = tester.get_lmax();
  EXPECT_LE(lmax, pwe);

  pwe = 0.08;
  tester.execute(bpp, target_psnr, pwe);
  lmax = tester.get_lmax();
  EXPECT_LE(lmax, pwe);

  pwe = 0.018;
  tester.execute(bpp, target_psnr, pwe);
  lmax = tester.get_lmax();
  EXPECT_LE(lmax, pwe);
}

//
// Test target PSNR
//
TEST(speck3d_target_psnr, small)
{
  speck_tester_omp tester("../test_data/wmag17.float", {17, 17, 17}, 1);

  const auto bpp = sperr::max_d;
  const auto pwe = 0.0;

  auto target_psnr = 90.0;
  tester.execute(bpp, target_psnr, pwe);
  auto psnr = tester.get_psnr();
  auto lmax = tester.get_lmax();
  EXPECT_GT(psnr, target_psnr);

  target_psnr = 120.0;
  tester.execute(bpp, target_psnr, pwe);
  psnr = tester.get_psnr();
  lmax = tester.get_lmax();
  EXPECT_GT(psnr, target_psnr - 0.1);  // An example of estimated error being too small.
}

TEST(speck3d_target_psnr, big)
{
  speck_tester_omp tester("../test_data/wmag128.float", {128, 128, 128}, 3);

  const auto bpp = sperr::max_d;
  const auto pwe = 0.0;

  auto target_psnr = 75.0;
  tester.execute(bpp, target_psnr, pwe);
  auto psnr = tester.get_psnr();
  auto lmax = tester.get_lmax();
  EXPECT_GT(psnr, target_psnr);

  target_psnr = 100.0;
  tester.execute(bpp, target_psnr, pwe);
  psnr = tester.get_psnr();
  lmax = tester.get_lmax();
  EXPECT_GT(psnr, target_psnr);
}

TEST(speck3d_target_psnr, small_data_range)
{
  speck_tester_omp tester("../test_data/vorticity.128_128_41", {128, 128, 41}, 0);

  const auto bpp = sperr::max_d;
  const auto pwe = 0.0;

  auto target_psnr = 85.0;
  tester.execute(bpp, target_psnr, pwe);
  auto psnr = tester.get_psnr();
  auto lmax = tester.get_lmax();
  EXPECT_GT(psnr, target_psnr);

  target_psnr = 110.0;
  tester.execute(bpp, target_psnr, pwe);
  psnr = tester.get_psnr();
  lmax = tester.get_lmax();
  EXPECT_GT(psnr, target_psnr);
}

//
// Test fixed-size mode
//
TEST(speck3d_bit_rate, small)
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

TEST(speck3d_bit_rate, big)
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

TEST(speck3d_bit_rate, narrow_data_range)
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

}  // namespace
