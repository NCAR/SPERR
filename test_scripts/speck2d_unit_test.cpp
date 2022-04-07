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
    m_dims[0] = x;
    m_dims[1] = y;
  }

  [[nodiscard]] float get_psnr() const { return m_psnr; }

  [[nodiscard]] float get_lmax() const { return m_lmax; }

  //
  // Execute the compression/decompression pipeline. Return 0 on success
  //

#ifdef QZ_TERM
  int execute(int32_t q, double tol)
#else
  int execute(double bpp)
#endif

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
    SPECK2D_Compressor compressor;
    if (compressor.copy_data(in_buf.data(), total_vals, m_dims) != RTNType::Good)
      return 1;

#ifdef QZ_TERM
    compressor.set_qz_level(q);
    if (compressor.set_tolerance(tol) != RTNType::Good)
      return 1;
#else
    if (compressor.set_bpp(bpp) != RTNType::Good)
      return 1;
#endif

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
  sperr::dims_type m_dims = {0, 0, 1};
  std::string m_output_name = "output.tmp";
  float m_psnr, m_lmax;
};

#ifdef QZ_TERM
//
// Fixed-error mode
//
TEST(speck2d, lena)
{
  speck_tester tester("../test_data/lena512.float", 512, 512);

  tester.execute(0, 10.0);
  auto psnr = tester.get_psnr();
  auto lmax = tester.get_lmax();
  EXPECT_GT(psnr, 55.0896);
  EXPECT_LT(psnr, 55.0897);
  EXPECT_LT(lmax, 1.77392);

  tester.execute(0, 1.0);
  psnr = tester.get_psnr();
  lmax = tester.get_lmax();
  EXPECT_GT(psnr, 55.4833);
  EXPECT_LT(psnr, 55.4834);
  EXPECT_LT(lmax, 1.0);
}

TEST(speck2d, odd_dim_image)
{
  speck_tester tester("../test_data/90x90.float", 90, 90);

  tester.execute(-2, 100.0);
  auto psnr = tester.get_psnr();
  auto lmax = tester.get_lmax();
  EXPECT_GT(psnr, 63.6754);
  EXPECT_LT(psnr, 63.6755);
  EXPECT_LT(lmax, 0.382059);

  tester.execute(-2, 0.3);
  psnr = tester.get_psnr();
  lmax = tester.get_lmax();
  EXPECT_GT(psnr, 63.8074);
  EXPECT_LT(psnr, 63.8075);
  EXPECT_LT(lmax, 0.3);
}

TEST(speck2d, small_data_range)
{
  speck_tester tester("../test_data/vorticity.512_512", 512, 512);

  tester.execute(-20, 1.0);
  auto psnr = tester.get_psnr();
  auto lmax = tester.get_lmax();
  EXPECT_GT(psnr, 71.2005);
  EXPECT_LT(psnr, 71.2006);
  EXPECT_LT(lmax, 1.80363e-06);

  tester.execute(-20, 1.7e-6);
  psnr = tester.get_psnr();
  lmax = tester.get_lmax();
  EXPECT_GT(psnr, 71.2033);
  EXPECT_LT(psnr, 71.2034);
  EXPECT_LT(lmax, 1.7e-6);
}

#else
//
// Fixed-rate mode
//
TEST(speck2d, lena)
{
  speck_tester tester("../test_data/lena512.float", 512, 512);

  tester.execute(4.0);
  auto psnr = tester.get_psnr();
  auto lmax = tester.get_lmax();
  EXPECT_GT(psnr, 54.2790);
  EXPECT_LT(psnr, 54.2791);
  EXPECT_LT(lmax, 2.2361);

  tester.execute(2.0);
  psnr = tester.get_psnr();
  lmax = tester.get_lmax();
  EXPECT_GT(psnr, 43.2851);
  EXPECT_LT(psnr, 43.2852);
  EXPECT_LT(lmax, 7.1736);

  tester.execute(1.0);
  psnr = tester.get_psnr();
  lmax = tester.get_lmax();
  EXPECT_GT(psnr, 38.7981);
  EXPECT_LT(psnr, 38.7982);
  EXPECT_LT(lmax, 14.5204);

  tester.execute(0.5);
  psnr = tester.get_psnr();
  lmax = tester.get_lmax();
  EXPECT_GT(psnr, 35.6254);
  EXPECT_LT(psnr, 35.6255);
  EXPECT_LT(lmax, 37.1734);
}

TEST(speck2d, odd_dim_image)
{
  speck_tester tester("../test_data/90x90.float", 90, 90);

  tester.execute(4.0);
  auto psnr = tester.get_psnr();
  auto lmax = tester.get_lmax();
  EXPECT_GT(psnr, 58.6157);
  EXPECT_LT(psnr, 58.6158);
  EXPECT_LT(lmax, 0.772957);

  tester.execute(2.0);
  psnr = tester.get_psnr();
  lmax = tester.get_lmax();
  EXPECT_GT(psnr, 46.6838);
  EXPECT_LT(psnr, 46.6839);
  EXPECT_LT(lmax, 2.98639);

  tester.execute(1.0);
  psnr = tester.get_psnr();
  lmax = tester.get_lmax();
  EXPECT_GT(psnr, 39.8890);
  EXPECT_LT(psnr, 39.8891);
  EXPECT_LT(lmax, 6.43229);

  tester.execute(0.5f);
  psnr = tester.get_psnr();
  lmax = tester.get_lmax();
  EXPECT_GT(psnr, 34.5442);
  EXPECT_LT(psnr, 34.5443);
  EXPECT_LT(lmax, 19.6903);
}

TEST(speck2d, small_data_range)
{
  speck_tester tester("../test_data/vorticity.512_512", 512, 512);

  tester.execute(4.0);
  auto psnr = tester.get_psnr();
  auto lmax = tester.get_lmax();
  EXPECT_GT(psnr, 71.2856);
  EXPECT_LT(psnr, 71.2857);
  EXPECT_LT(lmax, 0.000002);

  tester.execute(2.0);
  psnr = tester.get_psnr();
  lmax = tester.get_lmax();
  EXPECT_GT(psnr, 59.6629);
  EXPECT_LT(psnr, 59.6630);
  EXPECT_LT(lmax, 0.0000084);

  tester.execute(1.0);
  psnr = tester.get_psnr();
  lmax = tester.get_lmax();
  EXPECT_GT(psnr, 52.3908);
  EXPECT_LT(psnr, 52.3909);
  EXPECT_LT(lmax, 0.0000213);

  tester.execute(0.5);
  psnr = tester.get_psnr();
  lmax = tester.get_lmax();
  EXPECT_GT(psnr, 46.8999);
  EXPECT_LT(psnr, 46.89995);
  EXPECT_LT(lmax, 0.0000475);
}
#endif

//
// Test constant fields.
//
TEST(speck2d, constant)
{
  speck_tester tester("../test_data/const32x20x16.float", 32, 320);

#ifdef QZ_TERM
  auto rtn = tester.execute(1, 2.0);
#else
  auto rtn = tester.execute(1.0);
#endif

  EXPECT_EQ(rtn, 0);
  auto psnr = tester.get_psnr();
  auto lmax = tester.get_lmax();
  auto infty = std::numeric_limits<float>::infinity();
  EXPECT_EQ(psnr, infty);
  EXPECT_EQ(lmax, 0.0f);
}

}  // namespace
