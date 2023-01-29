#include <cstdlib>
#include "SPERR2D_Compressor.h"
#include "SPERR2D_Decompressor.h"
#include "gtest/gtest.h"

using sperr::RTNType;

namespace {

// Create a class that executes the entire pipeline, and calculates the error metrics
class sperr2d_tester {
 public:
  sperr2d_tester(const char* in, size_t x, size_t y)
  {
    m_input_name = in;
    m_dims[0] = x;
    m_dims[1] = y;
  }

  double get_psnr() const { return m_psnr; }

  double get_lmax() const { return m_lmax; }

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
    const auto& slice = decompressor.view_data();
    if (slice.size() != total_vals)
      return 1;

    //
    // Compare results
    //
    auto orig = sperr::vecd_type(total_vals);
    std::copy(in_buf.cbegin(), in_buf.cend(), orig.begin());
    auto ret = sperr::calc_stats(orig.data(), slice.data(), total_vals, 0);
    m_psnr = ret[2];
    m_lmax = ret[1];

    return 0;
  }

 private:
  std::string m_input_name;
  sperr::dims_type m_dims = {0, 0, 1};
  std::string m_output_name = "output.tmp";
  double m_psnr, m_lmax;
};

//
// Test target PWE mode
//
TEST(sperr2d_pwe, odd_dim_image)
{
  sperr2d_tester tester("../test_data/90x90.float", 90, 90);

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

TEST(sperr2d_pwe, lena_image)
{
  sperr2d_tester tester("../test_data/lena512.float", 512, 512);

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

TEST(sperr2d_pwe, small_data_range)
{
  sperr2d_tester tester("../test_data/vorticity.512_512", 512, 512);

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
TEST(sperr2d_psnr, odd_dim_image)
{
  sperr2d_tester tester("../test_data/90x90.float", 90, 90);

  const auto bpp = sperr::max_d;
  const auto pwe = 0.0;

  double target_psnr = 70.0;
  tester.execute(bpp, target_psnr, pwe);
  auto psnr = tester.get_psnr();
  auto lmax = tester.get_lmax();
  EXPECT_GT(psnr, target_psnr - 0.07);  // an example of estimate error being a little small

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

TEST(sperr2d_psnr, small_data_range)
{
  sperr2d_tester tester("../test_data/vorticity.512_512", 512, 512);

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
TEST(sperr2d_bpp, lena)
{
  sperr2d_tester tester("../test_data/lena512.float", 512, 512);

  const auto tar_psnr = std::numeric_limits<double>::max();
  const auto pwe = 0.0;

  tester.execute(4.0, tar_psnr, pwe);
  auto psnr = tester.get_psnr();
  auto lmax = tester.get_lmax();
  EXPECT_FLOAT_EQ(psnr, 54.27597);
  EXPECT_LT(lmax, 2.101561);

  tester.execute(2.0, tar_psnr, pwe);
  psnr = tester.get_psnr();
  lmax = tester.get_lmax();
  EXPECT_FLOAT_EQ(psnr, 43.283691);
  EXPECT_LT(lmax, 7.143872);

  tester.execute(1.0, tar_psnr, pwe);
  psnr = tester.get_psnr();
  lmax = tester.get_lmax();
  EXPECT_FLOAT_EQ(psnr, 38.796112);
  EXPECT_LT(lmax, 14.520393);

  tester.execute(0.5, tar_psnr, pwe);
  psnr = tester.get_psnr();
  lmax = tester.get_lmax();
  EXPECT_FLOAT_EQ(psnr, 35.621635);
  EXPECT_LT(lmax, 37.173374);
}

TEST(sperr2d_bpp, odd_dim_image)
{
  sperr2d_tester tester("../test_data/90x90.float", 90, 90);

  const auto tar_psnr = std::numeric_limits<double>::max();
  const auto pwe = 0.0;

  tester.execute(4.0, tar_psnr, pwe);
  auto psnr = tester.get_psnr();
  auto lmax = tester.get_lmax();
  EXPECT_FLOAT_EQ(psnr, 58.508751);
  EXPECT_LT(lmax, 0.772956);

  tester.execute(2.0, tar_psnr, pwe);
  psnr = tester.get_psnr();
  lmax = tester.get_lmax();
  EXPECT_FLOAT_EQ(psnr, 46.603992);
  EXPECT_LT(lmax, 2.913250);

  tester.execute(1.0, tar_psnr, pwe);
  psnr = tester.get_psnr();
  lmax = tester.get_lmax();
  EXPECT_FLOAT_EQ(psnr, 39.796303);
  EXPECT_LT(lmax, 6.805244);
}

TEST(sperr2d_bpp, small_data_range)
{
  sperr2d_tester tester("../test_data/vorticity.512_512", 512, 512);

  const auto tar_psnr = std::numeric_limits<double>::max();
  const auto pwe = 0.0;

  tester.execute(4.0, tar_psnr, pwe);
  auto psnr = tester.get_psnr();
  auto lmax = tester.get_lmax();
  EXPECT_FLOAT_EQ(psnr, 71.283524);
  EXPECT_LT(lmax, 1.792614e-06);

  tester.execute(2.0, tar_psnr, pwe);
  psnr = tester.get_psnr();
  lmax = tester.get_lmax();
  EXPECT_FLOAT_EQ(psnr, 59.660282);
  EXPECT_LT(lmax, 8.366274e-06);

  tester.execute(1.0, tar_psnr, pwe);
  psnr = tester.get_psnr();
  lmax = tester.get_lmax();
  EXPECT_FLOAT_EQ(psnr, 52.388039);
  EXPECT_LT(lmax, 2.125763e-05);

  tester.execute(0.5, tar_psnr, pwe);
  psnr = tester.get_psnr();
  lmax = tester.get_lmax();
  EXPECT_FLOAT_EQ(psnr, 46.894363);
  EXPECT_LT(lmax, 4.732156e-05);
}

//
// Test constant fields.
//
TEST(sperr2d_constant, constant)
{
  sperr2d_tester tester("../test_data/const32x20x16.float", 32, 320);

  const auto tar_psnr = std::numeric_limits<double>::max();
  const auto pwe = 0.0;

  auto rtn = tester.execute(2.0, tar_psnr, pwe);

  EXPECT_EQ(rtn, 0);
  auto psnr = tester.get_psnr();
  auto lmax = tester.get_lmax();
  auto infty = std::numeric_limits<double>::infinity();
  EXPECT_EQ(psnr, infty);
  EXPECT_EQ(lmax, 0.0f);
}

}  // namespace
