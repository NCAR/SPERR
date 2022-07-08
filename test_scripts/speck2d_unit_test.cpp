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
  int execute(double bpp, int32_t qz, double psnr, double pwe)
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
    const auto mode = sperr::compression_mode(total_bits, qz, psnr, pwe);
    switch (mode) {
      case sperr::CompMode::FixedSize :
        rtn = compressor.set_target_bpp(bpp);
        break;
      case sperr::CompMode::FixedQz :
        compressor.set_target_qz_level(qz);
        break;
      case sperr::CompMode::FixedPSNR :
        compressor.set_target_psnr(psnr);
        break;
      case sperr::CompMode::FixedPWE :
        compressor.set_target_pwe(pwe);
        break;
      default :
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

    // DEBUG
    // Find out what percentage of points are outliers, and how much it costs to encode them.
    //if (mode == sperr::CompMode::FixedPWE) {
    //  auto [num_o, num_byte] = compressor.get_outlier_stats();
    //  std::printf("Outlier pct = %f, bpp = %f, outlier storage pct = %f\n", 
    //              double(num_o) / double(total_vals) * 100.0,
    //              double(num_byte) * 8.0 / double(num_o),
    //              double(num_byte) / double(bitstream.size()) * 100.0);
    //}

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
  const auto q = sperr::lowest_int32;
  const auto target_psnr = sperr::max_d;

  double target_pwe = 1.0;
  tester.execute(bpp, q, target_psnr, target_pwe);
  auto lmax = tester.get_lmax();
  EXPECT_LE(lmax, target_pwe);

  target_pwe = 0.5;
  tester.execute(bpp, q, target_psnr, target_pwe);
  lmax = tester.get_lmax();
  EXPECT_LE(lmax, target_pwe);

  target_pwe = 0.1;
  tester.execute(bpp, q, target_psnr, target_pwe);
  lmax = tester.get_lmax();
  EXPECT_LE(lmax, target_pwe);
}

TEST(speck2d, PWE_lena_image)
{
  speck_tester tester("../test_data/lena512.float", 512, 512);

  const auto bpp = sperr::max_d;
  const auto q = sperr::lowest_int32;
  const auto target_psnr = sperr::max_d;

  double target_pwe = 0.7;
  tester.execute(bpp, q, target_psnr, target_pwe);
  auto lmax = tester.get_lmax();
  EXPECT_LE(lmax, target_pwe);

  target_pwe = 0.3;
  tester.execute(bpp, q, target_psnr, target_pwe);
  lmax = tester.get_lmax();
  EXPECT_LE(lmax, target_pwe);

  target_pwe = 0.1;
  tester.execute(bpp, q, target_psnr, target_pwe);
  lmax = tester.get_lmax();
  EXPECT_LE(lmax, target_pwe);
}

TEST(speck2d, PWE_small_data_range)
{
  speck_tester tester("../test_data/vorticity.512_512", 512, 512);

  const auto bpp = sperr::max_d;
  const auto q = sperr::lowest_int32;
  const auto target_psnr = sperr::max_d;

  double target_pwe = 3.2e-6;
  tester.execute(bpp, q, target_psnr, target_pwe);
  auto lmax = tester.get_lmax();
  EXPECT_LE(lmax, target_pwe);

  target_pwe = 1e-6;
  tester.execute(bpp, q, target_psnr, target_pwe);
  lmax = tester.get_lmax();
  EXPECT_LE(lmax, target_pwe);

  target_pwe = 5.5e-7;
  tester.execute(bpp, q, target_psnr, target_pwe);
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
  const auto q = sperr::lowest_int32;
  const auto pwe = 0.0;

  double target_psnr = 70.0;
  tester.execute(bpp, q, target_psnr, pwe);
  auto psnr = tester.get_psnr();
  auto lmax = tester.get_lmax();
  EXPECT_GT(psnr, target_psnr);

  target_psnr = 90.0;
  tester.execute(bpp, q, target_psnr, pwe);
  psnr = tester.get_psnr();
  lmax = tester.get_lmax();
  EXPECT_GT(psnr, target_psnr);

  target_psnr = 110.0;
  tester.execute(bpp, q, target_psnr, pwe);
  psnr = tester.get_psnr();
  lmax = tester.get_lmax();
  EXPECT_GT(psnr, target_psnr);
}

TEST(speck2d, PSNR_small_data_range)
{
  speck_tester tester("../test_data/vorticity.512_512", 512, 512);

  const auto bpp = std::numeric_limits<double>::max();
  const auto q = sperr::lowest_int32;
  const auto pwe = 0.0;

  double target_psnr = 80.0;
  tester.execute(bpp, q, target_psnr, pwe);
  auto psnr = tester.get_psnr();
  auto lmax = tester.get_lmax();
  EXPECT_GT(psnr, target_psnr);

  target_psnr = 100.0;
  tester.execute(bpp, q, target_psnr, pwe);
  psnr = tester.get_psnr();
  lmax = tester.get_lmax();
  EXPECT_GT(psnr, target_psnr);

  target_psnr = 120.0;
  tester.execute(bpp, q, target_psnr, pwe);
  psnr = tester.get_psnr();
  lmax = tester.get_lmax();
  EXPECT_GT(psnr, target_psnr);
}



//
// Test target quantization level mode
//
TEST(speck2d, QZ_lena)
{
  speck_tester tester("../test_data/lena512.float", 512, 512);

  const auto bpp = std::numeric_limits<double>::max();
  const auto tar_psnr = std::numeric_limits<double>::max();
  const auto pwe = 0.0;

  tester.execute(bpp, 1, tar_psnr, pwe);
  auto psnr = tester.get_psnr();
  auto lmax = tester.get_lmax();
  EXPECT_GT(psnr, 48.0146);
  EXPECT_LT(psnr, 48.0147);
  EXPECT_LT(lmax, 4.16155);

  tester.execute(bpp, -1, tar_psnr, pwe);
  psnr = tester.get_psnr();
  lmax = tester.get_lmax();
  EXPECT_GT(psnr, 62.0407);
  EXPECT_LT(psnr, 62.0408);
  EXPECT_LT(lmax, 0.899415);
}

TEST(speck2d, QZ_small_data_range)
{
  speck_tester tester("../test_data/vorticity.512_512", 512, 512);

  const auto bpp = std::numeric_limits<double>::max();
  const auto tar_psnr = std::numeric_limits<double>::max();
  const auto pwe = 0.0;

  tester.execute(bpp, -18, tar_psnr, pwe);
  auto psnr = tester.get_psnr();
  auto lmax = tester.get_lmax();
  EXPECT_GT(psnr, 59.5477);
  EXPECT_LT(psnr, 59.5478);
  EXPECT_LT(lmax, 8.37071e-06);

  tester.execute(bpp, -22, tar_psnr, pwe);
  psnr = tester.get_psnr();
  lmax = tester.get_lmax();
  EXPECT_GT(psnr, 84.3168);
  EXPECT_LT(psnr, 84.3169);
  EXPECT_LT(lmax, 4.59038e-07);
}


//
// Test fixed-size mode
//
TEST(speck2d, BPP_lena)
{
  speck_tester tester("../test_data/lena512.float", 512, 512);

  const auto q = std::numeric_limits<int32_t>::lowest();
  const auto tar_psnr = std::numeric_limits<double>::max();
  const auto pwe = 0.0;

  tester.execute(4.0, q, tar_psnr, pwe);
  auto psnr = tester.get_psnr();
  auto lmax = tester.get_lmax();
  EXPECT_GT(psnr, 54.2755);
  EXPECT_LT(psnr, 54.2756);
  EXPECT_LT(lmax, 2.2361);

  tester.execute(2.0, q, tar_psnr, pwe);
  psnr = tester.get_psnr();
  lmax = tester.get_lmax();
  EXPECT_GT(psnr, 43.2833);
  EXPECT_LT(psnr, 43.2834);
  EXPECT_LT(lmax, 7.1736);

  tester.execute(1.0, q, tar_psnr, pwe);
  psnr = tester.get_psnr();
  lmax = tester.get_lmax();
  EXPECT_GT(psnr, 38.7957);
  EXPECT_LT(psnr, 38.7958);
  EXPECT_LT(lmax, 14.5204);

  tester.execute(0.5, q, tar_psnr, pwe);
  psnr = tester.get_psnr();
  lmax = tester.get_lmax();
  EXPECT_GT(psnr, 35.6206);
  EXPECT_LT(psnr, 35.6207);
  EXPECT_LT(lmax, 37.1734);
}

TEST(speck2d, BPP_odd_dim_image)
{
  speck_tester tester("../test_data/90x90.float", 90, 90);

  const auto q = std::numeric_limits<int32_t>::lowest();
  const auto tar_psnr = std::numeric_limits<double>::max();
  const auto pwe = 0.0;

  tester.execute(4.0, q, tar_psnr, pwe);
  auto psnr = tester.get_psnr();
  auto lmax = tester.get_lmax();
  EXPECT_GT(psnr, 58.4989);
  EXPECT_LT(psnr, 58.4990);
  EXPECT_LT(lmax, 0.772957);

  tester.execute(2.0, q, tar_psnr, pwe);
  psnr = tester.get_psnr();
  lmax = tester.get_lmax();
  EXPECT_GT(psnr, 46.5943);
  EXPECT_LT(psnr, 46.5944);
  EXPECT_LT(lmax, 2.98639);

  tester.execute(1.0, q, tar_psnr, pwe);
  psnr = tester.get_psnr();
  lmax = tester.get_lmax();
  EXPECT_GT(psnr, 39.7869);
  EXPECT_LT(psnr, 39.7870);
  EXPECT_LT(lmax, 6.7783);

  tester.execute(0.5, q, tar_psnr, pwe);
  psnr = tester.get_psnr();
  lmax = tester.get_lmax();
  EXPECT_GT(psnr, 34.4064);
  EXPECT_LT(psnr, 34.4065);
  EXPECT_LT(lmax, 24.1439);
}

TEST(speck2d, BPP_small_data_range)
{
  speck_tester tester("../test_data/vorticity.512_512", 512, 512);

  const auto q = std::numeric_limits<int32_t>::lowest();
  const auto tar_psnr = std::numeric_limits<double>::max();
  const auto pwe = 0.0;

  tester.execute(4.0, q, tar_psnr, pwe);
  auto psnr = tester.get_psnr();
  auto lmax = tester.get_lmax();
  EXPECT_GT(psnr, 71.2830);
  EXPECT_LT(psnr, 71.2831);
  EXPECT_LT(lmax, 0.000002);

  tester.execute(2.0, q, tar_psnr, pwe);
  psnr = tester.get_psnr();
  lmax = tester.get_lmax();
  EXPECT_GT(psnr, 59.6598);
  EXPECT_LT(psnr, 59.6599);
  EXPECT_LT(lmax, 0.0000084);

  tester.execute(1.0, q, tar_psnr, pwe);
  psnr = tester.get_psnr();
  lmax = tester.get_lmax();
  EXPECT_GT(psnr, 52.3877);
  EXPECT_LT(psnr, 52.3878);
  EXPECT_LT(lmax, 0.0000213);

  tester.execute(0.5, q, tar_psnr, pwe);
  psnr = tester.get_psnr();
  lmax = tester.get_lmax();
  EXPECT_GT(psnr, 46.8937);
  EXPECT_LT(psnr, 46.8938);
  EXPECT_LT(lmax, 0.0000475);
}

//
// Test constant fields.
//
TEST(speck2d, constant)
{
  speck_tester tester("../test_data/const32x20x16.float", 32, 320);

  const auto q = std::numeric_limits<int32_t>::lowest();
  const auto tar_psnr = std::numeric_limits<double>::max();
  const auto pwe = 0.0;

  auto rtn = tester.execute(2.0, q, tar_psnr, pwe);

  EXPECT_EQ(rtn, 0);
  auto psnr = tester.get_psnr();
  auto lmax = tester.get_lmax();
  auto infty = std::numeric_limits<float>::infinity();
  EXPECT_EQ(psnr, infty);
  EXPECT_EQ(lmax, 0.0f);
}

}  // namespace
