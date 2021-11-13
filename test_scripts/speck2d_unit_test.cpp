#include <cstdlib>
#include "SPECK2D_Compressor.h"
#include "SPECK2D_Decompressor.h"
#include "gtest/gtest.h"

using sperr::RTNType;

namespace {

// Create a class that executes the entire pipeline, and calculates the error metrics
class speck_tester {
 public:
  speck_tester(const char* in, size_t x, size_t y)
  {
    m_input_name = in;
    m_dim_x = x;
    m_dim_y = y;
  }

  float get_psnr() const { return m_psnr; }

  float get_lmax() const { return m_lmax; }

  // Execute the compression/decompression pipeline. Return 0 on success
  int execute(double bpp)
  {
    // Reset lmax and psnr
    m_psnr = 0.0;
    m_lmax = 10000.0;

    const size_t total_vals = m_dim_x * m_dim_y;
    auto in_buf = sperr::read_whole_file<float>(m_input_name.c_str());
    if (in_buf.size() != total_vals)
      return 1;

    //
    // Use a compressor
    //
    SPECK2D_Compressor compressor;
    if (compressor.copy_data(in_buf.data(), total_vals, {m_dim_x, m_dim_y, 1}) != RTNType::Good)
      return 1;
    if (compressor.set_bpp(bpp) != RTNType::Good)
      return 1;
    if (compressor.compress() != RTNType::Good)
      return 1;
    auto bitstream = compressor.release_encoded_bitstream();

    //
    // Then use a decompressor
    //
    SPECK2D_Decompressor decompressor;
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
  size_t m_dim_x, m_dim_y;
  std::string m_output_name = "output.tmp";
  float m_psnr, m_lmax;
};

TEST(speck2d, lena)
{
  speck_tester tester("../test_data/lena512.float", 512, 512);

  tester.execute(4.0f);
  float psnr = tester.get_psnr();
  float lmax = tester.get_lmax();
  EXPECT_GT(psnr, 54.2828);
  EXPECT_LT(psnr, 54.2829);
  EXPECT_LT(lmax, 2.2361);

  tester.execute(2.0f);
  psnr = tester.get_psnr();
  lmax = tester.get_lmax();
  EXPECT_GT(psnr, 43.2870);
  EXPECT_LT(lmax, 7.1736);

  tester.execute(1.0f);
  psnr = tester.get_psnr();
  lmax = tester.get_lmax();
  EXPECT_GT(psnr, 38.8008);
  EXPECT_LT(lmax, 14.5204);

  tester.execute(0.5f);
  psnr = tester.get_psnr();
  lmax = tester.get_lmax();
  EXPECT_GT(psnr, 35.6299);
  EXPECT_LT(lmax, 37.1734);
}

TEST(speck2d, odd_dim_image)
{
  speck_tester tester("../test_data/90x90.float", 90, 90);

  tester.execute(4.0f);
  float psnr = tester.get_psnr();
  float lmax = tester.get_lmax();
  EXPECT_GT(psnr, 58.7091);
  EXPECT_LT(psnr, 58.7092);
  EXPECT_LT(lmax, 0.7588);

  tester.execute(2.0f);
  psnr = tester.get_psnr();
  lmax = tester.get_lmax();
  EXPECT_GT(psnr, 46.7725);
  EXPECT_LT(psnr, 46.7726);
  EXPECT_LT(lmax, 2.9545);

  tester.execute(1.0f);
  psnr = tester.get_psnr();
  lmax = tester.get_lmax();
  EXPECT_GT(psnr, 40.0297);
  EXPECT_LT(psnr, 40.0298);
  EXPECT_LT(lmax, 6.25197);

  tester.execute(0.5f);
  psnr = tester.get_psnr();
  lmax = tester.get_lmax();
  EXPECT_GT(psnr, 34.9525);
  EXPECT_LT(psnr, 34.9526);
  EXPECT_LT(lmax, 18.6514);
}

TEST(speck2d, small_data_range)
{
  speck_tester tester("../test_data/vorticity.512_512", 512, 512);

  tester.execute(4.0f);
  float psnr = tester.get_psnr();
  float lmax = tester.get_lmax();
  EXPECT_GT(psnr, 71.2888);
  EXPECT_LT(psnr, 71.2889);
  EXPECT_LT(lmax, 0.000002);

  tester.execute(2.0f);
  psnr = tester.get_psnr();
  lmax = tester.get_lmax();
  EXPECT_GT(psnr, 59.6661);
  EXPECT_LT(psnr, 59.6662);
  EXPECT_LT(lmax, 0.0000084);

  tester.execute(1.0f);
  psnr = tester.get_psnr();
  lmax = tester.get_lmax();
  EXPECT_GT(psnr, 52.3953);
  EXPECT_LT(psnr, 52.3954);
  EXPECT_LT(lmax, 0.0000213);

  tester.execute(0.5f);
  psnr = tester.get_psnr();
  lmax = tester.get_lmax();
  EXPECT_GT(psnr, 46.906871);
  EXPECT_LT(lmax, 0.0000475);
}

//
// Test constant fields.
//
TEST(speck2d, constant)
{
  speck_tester tester("../test_data/const32x20x16.float", 32, 320);
  auto rtn = tester.execute(1.0f);
  EXPECT_EQ(rtn, 0);
  auto psnr = tester.get_psnr();
  auto lmax = tester.get_lmax();
  auto infty = std::numeric_limits<float>::infinity();
  EXPECT_EQ(psnr, infty);
  EXPECT_EQ(lmax, 0.0f);
}

}  // namespace
