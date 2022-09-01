#include <cstdlib>
#include "SPERR2D_Compressor.h"
#include "SPERR2D_Decompressor.h"
#include "gtest/gtest.h"

using sperr::RTNType;

namespace {

// Create a class that executes the entire pipeline, and calculates the error metrics
class speck_tester {
 public:
  speck_tester(const char* in, size_t x, size_t y)
  {
    m_input_name = in;
    m_dims[0] = x;
    m_dims[1] = y;
  }

  float get_psnr() const { return m_psnr; }

  float get_lmax() const { return m_lmax; }

  //
  // Execute the compression/decompression pipeline. Return 0 on success
  //
  int execute(double bpp, double psnr, double pwe)
  {
    // Reset lmax and psnr
    m_psnr = 0.0;
    m_lmax = 10000.0;

    const size_t total_vals = m_dims[0] * m_dims[1];
    auto in_buf = sperr::read_whole_file<float>(m_input_name.c_str());
    if (in_buf.size() != total_vals)
      return 1;

    //
    // Use a compressor
    //
    SPERR2D_Compressor compressor;
    if (compressor.copy_data(in_buf.data(), total_vals, m_dims) != RTNType::Good)
      return 1;

    auto rtn = sperr::RTNType::Good;
    size_t total_bits = 0;
    if (bpp > 64.0)
      total_bits = sperr::max_size;
    else
      total_bits = static_cast<size_t>(bpp * total_vals);
    const auto mode = sperr::compression_mode(total_bits, psnr, pwe);
    switch (mode) {
      case sperr::CompMode::FixedSize:
        rtn = compressor.set_target_bpp(bpp);
        break;
      case sperr::CompMode::FixedPSNR:
        compressor.set_target_psnr(psnr);
        break;
      case sperr::CompMode::FixedPWE:
        compressor.set_target_pwe(pwe);
        break;
      default:
        return 1;
    }

    if (rtn != RTNType::Good)
      return 1;

    if (compressor.compress() != RTNType::Good)
      return 1;
    auto bitstream = compressor.release_encoded_bitstream();

    //
    // Then use a decompressor
    //
    SPERR2D_Decompressor decompressor;
    if (decompressor.use_bitstream(bitstream.data(), bitstream.size()) != RTNType::Good)
      return 1;
    if (decompressor.decompress() != RTNType::Good)
      return 1;
    auto slice = decompressor.get_data<float>();
    if (slice.size() != total_vals)
      return 1;

    //
    // Compare results
    //
    auto ret = sperr::calc_stats(in_buf.data(), slice.data(), total_vals, 8);
    m_psnr = ret[2];
    m_lmax = ret[1];

    return 0;
  }

 private:
  std::string m_input_name;
  sperr::dims_type m_dims = {0, 0, 1};
  std::string m_output_name = "output.tmp";
  float m_psnr, m_lmax;
};

//
// Test target PWE mode
//
TEST(speck2d, PWE_odd_dim_image)
{
  speck_tester tester("../test_data/90x90.float", 90, 90);

  const auto bpp = sperr::max_d;
  const auto target_psnr = sperr::max_d;

  double target_pwe = 1.0;
  tester.execute(bpp, target_psnr, target_pwe);
  auto lmax = tester.get_lmax();
  EXPECT_LE(lmax, target_pwe);

  target_pwe = 0.5;
  tester.execute(bpp, target_psnr, target_pwe);
  lmax = tester.get_lmax();
  EXPECT_LE(lmax, target_pwe);

  target_pwe = 0.1;
  tester.execute(bpp, target_psnr, target_pwe);
  lmax = tester.get_lmax();
  EXPECT_LE(lmax, target_pwe);

  target_pwe = 0.06;
  tester.execute(bpp, target_psnr, target_pwe);
  lmax = tester.get_lmax();
  EXPECT_LE(lmax, target_pwe);
}

TEST(speck2d, PWE_lena_image)
{
  speck_tester tester("../test_data/lena512.float", 512, 512);

  const auto bpp = sperr::max_d;
  const auto target_psnr = sperr::max_d;

  double target_pwe = 0.7;
  tester.execute(bpp, target_psnr, target_pwe);
  auto lmax = tester.get_lmax();
  EXPECT_LE(lmax, target_pwe);

  target_pwe = 0.35;
  tester.execute(bpp, target_psnr, target_pwe);
  lmax = tester.get_lmax();
  EXPECT_LE(lmax, target_pwe);

  target_pwe = 0.1;
  tester.execute(bpp, target_psnr, target_pwe);
  lmax = tester.get_lmax();
  EXPECT_LE(lmax, target_pwe);

  target_pwe = 0.06;
  tester.execute(bpp, target_psnr, target_pwe);
  lmax = tester.get_lmax();
  EXPECT_LE(lmax, target_pwe);
}

TEST(speck2d, PWE_small_data_range)
{
  speck_tester tester("../test_data/vorticity.512_512", 512, 512);

  const auto bpp = sperr::max_d;
  const auto target_psnr = sperr::max_d;

  double target_pwe = 3.2e-6;
  tester.execute(bpp, target_psnr, target_pwe);
  auto lmax = tester.get_lmax();
  EXPECT_LE(lmax, target_pwe);

  target_pwe = 1e-6;
  tester.execute(bpp, target_psnr, target_pwe);
  lmax = tester.get_lmax();
  EXPECT_LE(lmax, target_pwe);

  target_pwe = 5.5e-7;
  tester.execute(bpp, target_psnr, target_pwe);
  lmax = tester.get_lmax();
  EXPECT_LE(lmax, target_pwe);

  target_pwe = 2.5e-7;
  tester.execute(bpp, target_psnr, target_pwe);
  lmax = tester.get_lmax();
  EXPECT_LE(lmax, target_pwe);
}

//
// Test target PSNR mode
//
TEST(speck2d, PSNR_odd_dim_image)
{
  speck_tester tester("../test_data/90x90.float", 90, 90);

  const auto bpp = sperr::max_d;
  const auto pwe = 0.0;

  double target_psnr = 70.0;
  tester.execute(bpp, target_psnr, pwe);
  auto psnr = tester.get_psnr();
  auto lmax = tester.get_lmax();
  EXPECT_GT(psnr, target_psnr);

  target_psnr = 90.0;
  tester.execute(bpp, target_psnr, pwe);
  psnr = tester.get_psnr();
  lmax = tester.get_lmax();
  EXPECT_GT(psnr, target_psnr);

  target_psnr = 110.0;
  tester.execute(bpp, target_psnr, pwe);
  psnr = tester.get_psnr();
  lmax = tester.get_lmax();
  EXPECT_GT(psnr, target_psnr - 0.12);  // an example of estimate error being a little small
}

TEST(speck2d, PSNR_small_data_range)
{
  speck_tester tester("../test_data/vorticity.512_512", 512, 512);

  const auto bpp = std::numeric_limits<double>::max();
  const auto pwe = 0.0;

  double target_psnr = 80.0;
  tester.execute(bpp, target_psnr, pwe);
  auto psnr = tester.get_psnr();
  auto lmax = tester.get_lmax();
  EXPECT_GT(psnr, target_psnr);

  target_psnr = 100.0;
  tester.execute(bpp, target_psnr, pwe);
  psnr = tester.get_psnr();
  lmax = tester.get_lmax();
  EXPECT_GT(psnr, target_psnr);

  target_psnr = 120.0;
  tester.execute(bpp, target_psnr, pwe);
  psnr = tester.get_psnr();
  lmax = tester.get_lmax();
  EXPECT_GT(psnr, target_psnr);
}

//
// Test fixed-size mode
//
TEST(speck2d, BPP_lena)
{
  speck_tester tester("../test_data/lena512.float", 512, 512);

  const auto tar_psnr = std::numeric_limits<double>::max();
  const auto pwe = 0.0;

  tester.execute(4.0, tar_psnr, pwe);
  auto psnr = tester.get_psnr();
  auto lmax = tester.get_lmax();
  EXPECT_GT(psnr, 54.2751);
  EXPECT_LT(psnr, 54.2752);
  EXPECT_LT(lmax, 2.2361);

  tester.execute(2.0, tar_psnr, pwe);
  psnr = tester.get_psnr();
  lmax = tester.get_lmax();
  EXPECT_GT(psnr, 43.2831);
  EXPECT_LT(psnr, 43.2832);
  EXPECT_LT(lmax, 7.1736);

  tester.execute(1.0, tar_psnr, pwe);
  psnr = tester.get_psnr();
  lmax = tester.get_lmax();
  EXPECT_GT(psnr, 38.7955);
  EXPECT_LT(psnr, 38.7956);
  EXPECT_LT(lmax, 14.5204);

  tester.execute(0.5, tar_psnr, pwe);
  psnr = tester.get_psnr();
  lmax = tester.get_lmax();
  EXPECT_GT(psnr, 35.6198);
  EXPECT_LT(psnr, 35.6199);
  EXPECT_LT(lmax, 37.1734);
}

TEST(speck2d, BPP_odd_dim_image)
{
  speck_tester tester("../test_data/90x90.float", 90, 90);

  const auto tar_psnr = std::numeric_limits<double>::max();
  const auto pwe = 0.0;

  tester.execute(4.0, tar_psnr, pwe);
  auto psnr = tester.get_psnr();
  auto lmax = tester.get_lmax();
  EXPECT_GT(psnr, 58.4848);
  EXPECT_LT(psnr, 58.4849);
  EXPECT_LT(lmax, 0.772957);

  tester.execute(2.0, tar_psnr, pwe);
  psnr = tester.get_psnr();
  lmax = tester.get_lmax();
  EXPECT_GT(psnr, 46.5839);
  EXPECT_LT(psnr, 46.5840);
  EXPECT_LT(lmax, 2.98639);

  tester.execute(1.0, tar_psnr, pwe);
  psnr = tester.get_psnr();
  lmax = tester.get_lmax();
  EXPECT_GT(psnr, 39.7787);
  EXPECT_LT(psnr, 39.7788);
  EXPECT_LT(lmax, 6.7783);
}

TEST(speck2d, BPP_small_data_range)
{
  speck_tester tester("../test_data/vorticity.512_512", 512, 512);

  const auto tar_psnr = std::numeric_limits<double>::max();
  const auto pwe = 0.0;

  tester.execute(4.0, tar_psnr, pwe);
  auto psnr = tester.get_psnr();
  auto lmax = tester.get_lmax();
  EXPECT_GT(psnr, 71.2826);
  EXPECT_LT(psnr, 71.2827);
  EXPECT_LT(lmax, 0.000002);

  tester.execute(2.0, tar_psnr, pwe);
  psnr = tester.get_psnr();
  lmax = tester.get_lmax();
  EXPECT_GT(psnr, 59.6593);
  EXPECT_LT(psnr, 59.6594);
  EXPECT_LT(lmax, 0.0000084);

  tester.execute(1.0, tar_psnr, pwe);
  psnr = tester.get_psnr();
  lmax = tester.get_lmax();
  EXPECT_GT(psnr, 52.3874);
  EXPECT_LT(psnr, 52.3875);
  EXPECT_LT(lmax, 0.0000213);

  tester.execute(0.5, tar_psnr, pwe);
  psnr = tester.get_psnr();
  lmax = tester.get_lmax();
  EXPECT_GT(psnr, 46.8927);
  EXPECT_LT(psnr, 46.8928);
  EXPECT_LT(lmax, 0.0000475);
}

//
// Test constant fields.
//
TEST(speck2d, constant)
{
  speck_tester tester("../test_data/const32x20x16.float", 32, 320);

  const auto tar_psnr = std::numeric_limits<double>::max();
  const auto pwe = 0.0;

  auto rtn = tester.execute(2.0, tar_psnr, pwe);

  EXPECT_EQ(rtn, 0);
  auto psnr = tester.get_psnr();
  auto lmax = tester.get_lmax();
  auto infty = std::numeric_limits<float>::infinity();
  EXPECT_EQ(psnr, infty);
  EXPECT_EQ(lmax, 0.0f);
}

}  // namespace
