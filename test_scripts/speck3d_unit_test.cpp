#include "SPERR3D_OMP_C.h"
#include "SPERR3D_OMP_D.h"

#include <cstring>
#include "gtest/gtest.h"

namespace {

using sperr::RTNType;

// Create a class that executes the entire pipeline, and calculates the error metrics.
class speck_tester_omp {
 public:
  speck_tester_omp(const char* in, sperr::dims_type dims, size_t num_t)
  {
    m_input_name = in;
    m_dims = dims;
    m_num_t = num_t;
  }

  void prefer_chunk_dims(sperr::dims_type dims) { m_chunk_dims = dims; }

  float get_psnr() const { return m_psnr; }

  float get_lmax() const { return m_lmax; }

  //
  // Execute the compression/decompression pipeline. Return 0 on success
  //
  int execute(double bpp, double psnr, double pwe)
  {
    // Reset lmax and psnr
    m_psnr = 0.0;
    m_lmax = 1000.0;

    const size_t total_vals = m_dims[0] * m_dims[1] * m_dims[2];

    //
    // Use a compressor
    //
    auto in_buf = sperr::read_whole_file<float>(m_input_name);
    if (in_buf.size() != total_vals)
      return 1;

    auto compressor = SPERR3D_OMP_C();
    compressor.set_num_threads(m_num_t);

    if (compressor.copy_data(in_buf.data(), total_vals, m_dims, m_chunk_dims) != RTNType::Good)
      return 1;

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
    auto vol = decompressor.get_data<float>();
    if (vol.size() != total_vals)
      return 1;

    //
    // Compare results
    //
    const size_t nbytes = sizeof(float) * total_vals;
    auto orig = sperr::read_whole_file<float>(m_input_name);

    auto ret = sperr::calc_stats(orig.data(), vol.data(), total_vals, 8);
    m_psnr = ret[2];
    m_lmax = ret[1];

    return 0;
  }

 private:
  std::string m_input_name;
  sperr::dims_type m_dims = {0, 0, 0};
  sperr::dims_type m_chunk_dims = {64, 64, 64};
  std::string m_output_name = "output.tmp";
  float m_psnr, m_lmax;
  size_t m_num_t;
};

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
  auto infty = std::numeric_limits<float>::infinity();
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
  auto infty = std::numeric_limits<float>::infinity();
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
  float lmax = tester.get_lmax();
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

  auto pwe = double{1.4e-7};
  tester.execute(bpp, target_psnr, pwe);
  float lmax = tester.get_lmax();
  EXPECT_LE(lmax, pwe);

  pwe = 7.3e-7;
  tester.execute(bpp, target_psnr, pwe);
  lmax = tester.get_lmax();
  EXPECT_LE(lmax, pwe);

  pwe = 6.7e-8;
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
  float lmax = tester.get_lmax();
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
  float psnr = tester.get_psnr();
  float lmax = tester.get_lmax();
  EXPECT_GT(psnr, target_psnr);

  target_psnr = 120.0;
  tester.execute(bpp, target_psnr, pwe);
  psnr = tester.get_psnr();
  lmax = tester.get_lmax();
  EXPECT_GT(psnr, target_psnr);
}

TEST(speck3d_target_psnr, big)
{
  speck_tester_omp tester("../test_data/wmag128.float", {128, 128, 128}, 3);

  const auto bpp = sperr::max_d;
  const auto pwe = 0.0;

  auto target_psnr = 75.0;
  tester.execute(bpp, target_psnr, pwe);
  float psnr = tester.get_psnr();
  float lmax = tester.get_lmax();
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
  float psnr = tester.get_psnr();
  float lmax = tester.get_lmax();
  EXPECT_GT(psnr, target_psnr);

  target_psnr = 109.0;
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
  float psnr = tester.get_psnr();
  float lmax = tester.get_lmax();
  EXPECT_GT(psnr, 52.808);
  EXPECT_LT(psnr, 52.809);
  EXPECT_LT(lmax, 1.8526);

  tester.execute(2.0, tar_psnr, pwe);
  psnr = tester.get_psnr();
  lmax = tester.get_lmax();
  EXPECT_GT(psnr, 41.1909);
  EXPECT_LT(psnr, 41.1910);
  EXPECT_LT(lmax, 6.4132);

  tester.execute(1.0, tar_psnr, pwe);
  psnr = tester.get_psnr();
  lmax = tester.get_lmax();
  EXPECT_GT(psnr, 34.5465);
  EXPECT_LT(psnr, 34.5466);
  EXPECT_LT(lmax, 13.0171);
}

TEST(speck3d_bit_rate, big)
{
  speck_tester_omp tester("../test_data/wmag128.float", {128, 128, 128}, 0);

  const auto tar_psnr = std::numeric_limits<double>::max();
  const auto pwe = 0.0;

  tester.execute(2.0, tar_psnr, pwe);
  float psnr = tester.get_psnr();
  float lmax = tester.get_lmax();
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
  float psnr = tester.get_psnr();
  float lmax = tester.get_lmax();
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
