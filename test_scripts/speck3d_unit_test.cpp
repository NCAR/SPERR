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

#ifdef QZ_TERM
  int execute(int32_t qz_level, double tol)
#else
  int execute(double bpp)
#endif

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

#ifdef QZ_TERM
    compressor.set_qz_level(qz_level);
    compressor.set_tolerance(tol);
#else
    compressor.set_bpp(bpp);
#endif

    if (compressor.compress() != RTNType::Good)
      return 1;
    auto stream = compressor.get_encoded_bitstream();
    std::cout << "stream len = " << stream.size() << std::endl;
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
#ifdef QZ_TERM
  auto rtn = tester.execute(-1, 1);
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

TEST(speck3d_constant, omp_chunks)
{
  speck_tester_omp tester("../test_data/const32x32x59.float", {32, 32, 59}, 8);
  tester.prefer_chunk_dims({32, 32, 32});
#ifdef QZ_TERM
  auto rtn = tester.execute(-1, 1);
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

#ifdef QZ_TERM
//
// Error bound mode
//
TEST(speck3d_qz_term, narrow_data_range)
{
  // We specify to use 1 thread to make sure that object re-use has no side effects.
  // The next set of tests will use multiple threads.
  speck_tester_omp tester("../test_data/vorticity.128_128_41", {128, 128, 41}, 1);
  auto rtn = tester.execute(-16, 3e-5);
  EXPECT_EQ(rtn, 0);
  float psnr = tester.get_psnr();
  float lmax = tester.get_lmax();
  EXPECT_GT(psnr, 4.22075080e+01);
  EXPECT_LT(psnr, 4.22075081e+01);
  EXPECT_LT(lmax, 2.987783e-05);

  rtn = tester.execute(-18, 7e-6);
  EXPECT_EQ(rtn, 0);
  psnr = tester.get_psnr();
  lmax = tester.get_lmax();
  EXPECT_GT(psnr, 50.486732);
  EXPECT_LT(psnr, 50.486733);
  EXPECT_LT(lmax, 6.991625e-06);
}
TEST(speck3d_qz_term, small_tolerance)
{
  speck_tester_omp tester("../test_data/wmag128.float", {128, 128, 128}, 5);
  auto rtn = tester.execute(-3, 0.07);
  EXPECT_EQ(rtn, 0);
  float psnr = tester.get_psnr();
  float lmax = tester.get_lmax();
  EXPECT_GT(psnr, 8.1441611e+01);
  EXPECT_LT(psnr, 8.1441613e+01);
  EXPECT_LT(lmax, 6.9999696e-02);

  rtn = tester.execute(-5, 0.05);
  EXPECT_EQ(rtn, 0);
  psnr = tester.get_psnr();
  lmax = tester.get_lmax();
  EXPECT_GT(psnr, 9.1716300e+01);
  EXPECT_LT(psnr, 9.1716302e+01);
  EXPECT_LT(lmax, 4.9962998e-02);
}

#else
//
// fixed-size mode
//
TEST(speck3d_bit_rate, small)
{
  speck_tester_omp tester("../test_data/wmag17.float", {17, 17, 17}, 1);

  tester.execute(4.0);
  float psnr = tester.get_psnr();
  float lmax = tester.get_lmax();
  EXPECT_GT(psnr, 52.8208);
  EXPECT_LT(psnr, 52.8209);
  EXPECT_LT(lmax, 1.8526);

  tester.execute(2.0);
  psnr = tester.get_psnr();
  lmax = tester.get_lmax();
  EXPECT_GT(psnr, 41.2243);
  EXPECT_LT(psnr, 41.2244);
  EXPECT_LT(lmax, 6.4132);

  tester.execute(1.0);
  psnr = tester.get_psnr();
  lmax = tester.get_lmax();
  EXPECT_GT(psnr, 34.5634);
  EXPECT_LT(psnr, 34.5635);
  EXPECT_LT(lmax, 13.0171);
}

TEST(speck3d_bit_rate, big)
{
  speck_tester_omp tester("../test_data/wmag128.float", {128, 128, 128}, 3);

  tester.execute(2.0);
  float psnr = tester.get_psnr();
  float lmax = tester.get_lmax();
  EXPECT_GT(psnr, 53.8107);
  EXPECT_LT(psnr, 53.8108);
  EXPECT_LT(lmax, 9.6954);

  tester.execute(1.0);
  psnr = tester.get_psnr();
  lmax = tester.get_lmax();
  EXPECT_GT(psnr, 47.2227);
  EXPECT_LT(psnr, 47.2228);
  EXPECT_LT(lmax, 16.3006);

  tester.execute(0.5);
  psnr = tester.get_psnr();
  lmax = tester.get_lmax();
  EXPECT_GT(psnr, 42.6889);
  EXPECT_LT(psnr, 42.6890);
  EXPECT_LT(lmax, 27.5579);

  tester.execute(0.25);
  psnr = tester.get_psnr();
  lmax = tester.get_lmax();
  EXPECT_GT(psnr, 39.2783);
  EXPECT_LT(psnr, 39.2784);
  EXPECT_LT(lmax, 48.8490);
}

TEST(speck3d_bit_rate, narrow_data_range)
{
  speck_tester_omp tester("../test_data/vorticity.128_128_41", {128, 128, 41}, 2);

  tester.execute(4.0);
  float psnr = tester.get_psnr();
  float lmax = tester.get_lmax();
  EXPECT_GT(psnr, 67.7949);
  EXPECT_LT(psnr, 67.7950);
  EXPECT_LT(lmax, 1.17876e-06);

  tester.execute(2.0);
  psnr = tester.get_psnr();
  lmax = tester.get_lmax();
  EXPECT_GT(psnr, 55.7837);
  EXPECT_LT(psnr, 55.7838);
  EXPECT_LT(lmax, 4.703875e-06);

  tester.execute(0.8);
  psnr = tester.get_psnr();
  lmax = tester.get_lmax();
  EXPECT_GT(psnr, 47.2473);
  EXPECT_LT(psnr, 47.2474);
  EXPECT_LT(lmax, 1.744990e-05);

  tester.execute(0.4);
  psnr = tester.get_psnr();
  lmax = tester.get_lmax();
  EXPECT_GT(psnr, 43.1756);
  EXPECT_LT(psnr, 43.1757);
  EXPECT_LT(lmax, 3.342385e-05);
}

#endif

}  // namespace
