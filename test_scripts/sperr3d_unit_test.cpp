#include "SPERR3D_OMP_C.h"
#include "SPERR3D_OMP_D.h"

#include <cstring>
#include "gtest/gtest.h"

namespace {

using sperr::RTNType;

// Create a class that executes the entire pipeline, and calculates the error metrics.
class sperr3d_tester {
 public:
  sperr3d_tester(const char* in, sperr::dims_type dims, size_t num_t)
  {
    m_input_name = in;
    m_dims = dims;
    m_num_t = num_t;
  }

  void prefer_chunk_dims(sperr::dims_type dims) { m_chunk_dims = dims; }

  double get_psnr() const { return m_psnr; }

  double get_lmax() const { return m_lmax; }

  //
  // Execute the compression/decompression pipeline. Return 0 on success
  //
  int execute(double bpp, double psnr, double pwe, bool prec64 = false)
  {
    // Reset lmax and psnr
    m_psnr = 0.0;
    m_lmax = 1000.0;

    const size_t total_vals = m_dims[0] * m_dims[1] * m_dims[2];

    //
    // Read in the input file
    //
    auto in_f = sperr::vecf_type();
    auto in_d = sperr::vecd_type();
    if (prec64) {
      in_d = sperr::read_whole_file<double>(m_input_name);
      if (in_d.size() != total_vals)
        return 1;
    }
    else {
      in_f = sperr::read_whole_file<float>(m_input_name);
      if (in_f.size() != total_vals)
        return 1;
    }

    //
    // Use a compressor
    //
    auto compressor = SPERR3D_OMP_C();
    compressor.set_num_threads(m_num_t);

    if (prec64) {
      if (compressor.copy_data(in_d.data(), total_vals, m_dims, m_chunk_dims) != RTNType::Good)
        return 1;
    }
    else {
      if (compressor.copy_data(in_f.data(), total_vals, m_dims, m_chunk_dims) != RTNType::Good)
        return 1;
    }

    size_t total_bits = 0;
    if (bpp > 64.0)
      total_bits = sperr::max_size;
    else
      total_bits = static_cast<size_t>(bpp * total_vals);

    auto rtn = sperr::RTNType::Good;
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

    if (compressor.compress() != RTNType::Good) {
      return 1;
    }
    auto stream = compressor.get_encoded_bitstream();
    if (stream.empty())
      return 1;

    //
    // Use a decompressor
    //
    auto decompressor = SPERR3D_OMP_D();
    decompressor.set_num_threads(m_num_t);
    if (decompressor.use_bitstream(stream.data(), stream.size()) != RTNType::Good)
      return 1;
    if (decompressor.decompress(stream.data()) != RTNType::Good)
      return 1;
    const auto& vol = decompressor.view_data();
    if (vol.size() != total_vals)
      return 1;

    //
    // Compare results
    //
    if (prec64) {
      auto ret = sperr::calc_stats(in_d.data(), vol.data(), total_vals, 0);
      m_psnr = ret[2];
      m_lmax = ret[1];
    }
    else {
      auto orig = sperr::vecd_type(total_vals);
      std::copy(in_f.cbegin(), in_f.cend(), orig.begin());
      auto ret = sperr::calc_stats(orig.data(), vol.data(), total_vals, 0);
      m_psnr = ret[2];
      m_lmax = ret[1];
    }

    return 0;
  }

 private:
  std::string m_input_name;
  sperr::dims_type m_dims = {0, 0, 0};
  sperr::dims_type m_chunk_dims = {64, 64, 64};
  std::string m_output_name = "output.tmp";
  double m_psnr, m_lmax;
  size_t m_num_t;
};

//
// Test constant fields.
//
TEST(sperr3d_constant, one_chunk)
{
  sperr3d_tester tester("../test_data/const32x20x16.float", {32, 20, 16}, 2);

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

TEST(sperr3d_constant, omp_chunks)
{
  sperr3d_tester tester("../test_data/const32x32x59.float", {32, 32, 59}, 8);
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
TEST(sperr3d_target_pwe, small)
{
  sperr3d_tester tester("../test_data/wmag17.float", {17, 17, 17}, 1);

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

TEST(sperr3d_target_pwe, small_data_range)
{
  sperr3d_tester tester("../test_data/vorticity.128_128_41", {128, 128, 41}, 2);

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

TEST(sperr3d_target_pwe, big)
{
  sperr3d_tester tester("../test_data/wmag128.float", {128, 128, 128}, 0);

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

TEST(sperr3d_target_pwe, tiny_tolerance)
{
  // Reported in https://github.com/NCAR/SPERR/issues/184
  sperr3d_tester tester("../test_data/data16.in", {16, 16, 16}, 0);
  const bool is_double = true;

  const auto bpp = sperr::max_d;
  const auto target_psnr = sperr::max_d;

  double pwe = 1e-48;
  tester.execute(bpp, target_psnr, pwe, is_double);
  double lmax = tester.get_lmax();
  EXPECT_LE(lmax, pwe);

  pwe = 1e-50;
  tester.execute(bpp, target_psnr, pwe, is_double);
  lmax = tester.get_lmax();
  EXPECT_LE(lmax, pwe);

  pwe = 1e-52;
  tester.execute(bpp, target_psnr, pwe, is_double);
  lmax = tester.get_lmax();
  EXPECT_LE(lmax, pwe);

  pwe = 1e-55;
  tester.execute(bpp, target_psnr, pwe, is_double);
  lmax = tester.get_lmax();
  EXPECT_LE(lmax, pwe);
}

//
// Test target PSNR
//
TEST(sperr3d_target_psnr, small)
{
  sperr3d_tester tester("../test_data/wmag17.float", {17, 17, 17}, 1);

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

TEST(sperr3d_target_psnr, big)
{
  sperr3d_tester tester("../test_data/wmag128.float", {128, 128, 128}, 3);

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

TEST(sperr3d_target_psnr, small_data_range)
{
  sperr3d_tester tester("../test_data/vorticity.128_128_41", {128, 128, 41}, 0);

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
TEST(sperr3d_bit_rate, small)
{
  sperr3d_tester tester("../test_data/wmag17.float", {17, 17, 17}, 1);

  const auto tar_psnr = std::numeric_limits<double>::max();
  const auto pwe = 0.0;

  tester.execute(4.0, tar_psnr, pwe);
  auto psnr = tester.get_psnr();
  auto lmax = tester.get_lmax();
  EXPECT_FLOAT_EQ(psnr, 52.929482);
  EXPECT_LT(lmax, 1.811367);

  tester.execute(2.0, tar_psnr, pwe);
  psnr = tester.get_psnr();
  lmax = tester.get_lmax();
  EXPECT_FLOAT_EQ(psnr, 41.308178);
  EXPECT_LT(lmax, 6.357375);

  tester.execute(1.0, tar_psnr, pwe);
  psnr = tester.get_psnr();
  lmax = tester.get_lmax();
  EXPECT_FLOAT_EQ(psnr, 34.684952);
  EXPECT_LT(lmax, 12.511442);
}

TEST(sperr3d_bit_rate, big)
{
  sperr3d_tester tester("../test_data/wmag128.float", {128, 128, 128}, 0);

  const auto tar_psnr = std::numeric_limits<double>::max();
  const auto pwe = 0.0;

  tester.execute(2.0, tar_psnr, pwe);
  auto psnr = tester.get_psnr();
  auto lmax = tester.get_lmax();
  EXPECT_FLOAT_EQ(psnr, 53.810936);
  EXPECT_LT(lmax, 7.695327);

  tester.execute(1.0, tar_psnr, pwe);
  psnr = tester.get_psnr();
  lmax = tester.get_lmax();
  EXPECT_FLOAT_EQ(psnr, 47.223034);
  EXPECT_LT(lmax, 16.300520);

  tester.execute(0.5, tar_psnr, pwe);
  psnr = tester.get_psnr();
  lmax = tester.get_lmax();
  EXPECT_FLOAT_EQ(psnr, 42.689213);
  EXPECT_LT(lmax, 27.557829);
}

TEST(sperr3d_bit_rate, narrow_data_range)
{
  sperr3d_tester tester("../test_data/vorticity.128_128_41", {128, 128, 41}, 2);

  const auto tar_psnr = std::numeric_limits<double>::max();
  const auto pwe = 0.0;

  tester.execute(4.0, tar_psnr, pwe);
  auto psnr = tester.get_psnr();
  auto lmax = tester.get_lmax();
  EXPECT_FLOAT_EQ(psnr, 67.795227);
  EXPECT_LT(lmax, 1.178883e-06);

  tester.execute(2.0, tar_psnr, pwe);
  psnr = tester.get_psnr();
  lmax = tester.get_lmax();
  EXPECT_FLOAT_EQ(psnr, 55.784035);
  EXPECT_LT(lmax, 4.704819e-06);

  tester.execute(0.8, tar_psnr, pwe);
  psnr = tester.get_psnr();
  lmax = tester.get_lmax();
  EXPECT_FLOAT_EQ(psnr, 47.248665);
  EXPECT_LT(lmax, 1.745131e-05);

  tester.execute(0.4, tar_psnr, pwe);
  psnr = tester.get_psnr();
  lmax = tester.get_lmax();
  EXPECT_FLOAT_EQ(psnr, 43.176487);
  EXPECT_LT(lmax, 3.3408496e-05);
}

}  // namespace
