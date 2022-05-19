#include "SPECK3D_Compressor.h"
#include "SPECK3D_Decompressor.h"

#include "SPECK3D_OMP_C.h"
#include "SPECK3D_OMP_D.h"

#include <cstring>
#include "gtest/gtest.h"

namespace {

using sperr::RTNType;

// Create a class that executes the entire pipeline, and calculates the error metrics
// This class tests objects SPECK3D_Compressor and SPECK3D_Decompressor
class speck_tester {
 public:
  speck_tester(const char* in, sperr::dims_type dims)
  {
    m_input_name = in;
    m_dims = dims;
  }

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
    auto in_buf = sperr::read_whole_file<float>(m_input_name.c_str());
    if (in_buf.size() != total_vals)
      return 1;
    SPECK3D_Compressor compressor;
    if (compressor.copy_data(in_buf.data(), total_vals, m_dims) != RTNType::Good)
      return 1;

#ifdef QZ_TERM
    compressor.set_qz_level(qz_level);
    compressor.set_tolerance(tol);
#else
    if (compressor.set_bpp(bpp) != RTNType::Good)
      return 1;
#endif

    if (compressor.compress() != RTNType::Good)
      return 1;
    auto stream = compressor.release_encoded_bitstream();
    if (stream.empty())
      return 1;

    //
    // Use a decompressor
    //
    SPECK3D_Decompressor decompressor;
    if (decompressor.use_bitstream(stream.data(), stream.size()) != RTNType::Good)
      return 1;
    if (decompressor.decompress() != RTNType::Good)
      return 1;
    auto vol = decompressor.get_data<float>();
    if (vol.size() != total_vals)
      return 1;

    //
    // Compare results
    //
    auto ret = sperr::calc_stats(in_buf.data(), vol.data(), total_vals, 8);
    m_psnr = ret[2];
    m_lmax = ret[1];

    return 0;
  }

 private:
  std::string m_input_name;
  sperr::dims_type m_dims = {0, 0, 0};
  std::string m_output_name = "output.tmp";
  float m_psnr, m_lmax;
};

// Create a class that executes the entire pipeline, and calculates the error metrics
// This class tests objects SPECK3D_OMP_C and SPECK3D_OMP_D
class speck_tester_omp {
 public:
  speck_tester_omp(const char* in, sperr::dims_type dims, size_t num_t)
  {
    m_input_name = in;
    m_dims = dims;
    m_num_t = num_t;
  }

  void prefer_chunk_dims(sperr::dims_type dims) { m_chunk_dims = dims; }

  [[nodiscard]] float get_psnr() const { return m_psnr; }

  [[nodiscard]] float get_lmax() const { return m_lmax; }

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
    auto in_buf = sperr::read_whole_file<float>(m_input_name.c_str());
    if (in_buf.size() != total_vals)
      return 1;

    SPECK3D_OMP_C compressor;
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
    if (stream.empty())
      return 1;

    //
    // Use a decompressor
    //
    SPECK3D_OMP_D decompressor;
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
    auto orig = std::make_unique<float[]>(total_vals);
    if (sperr::read_n_bytes(m_input_name.c_str(), nbytes, orig.get()) != sperr::RTNType::Good)
      return 1;

    auto ret = sperr::calc_stats(orig.get(), vol.data(), total_vals, 8);
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
  speck_tester tester("../test_data/const32x20x16.float", {32, 20, 16});
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
TEST(speck3d_qz_term, large_tolerance)
{
  const float tol = 1.0;
  speck_tester tester("../test_data/wmag128.float", {128, 128, 128});
  auto rtn = tester.execute(2, tol);
  EXPECT_EQ(rtn, 0);
  float psnr = tester.get_psnr();
  float lmax = tester.get_lmax();
  EXPECT_GT(psnr, 5.76309120e+01);
  EXPECT_LT(psnr, 5.76309166e+01);
  EXPECT_LE(lmax, tol);

  rtn = tester.execute(-1, tol);
  EXPECT_EQ(rtn, 0);
  psnr = tester.get_psnr();
  lmax = tester.get_lmax();
  EXPECT_GT(psnr, 6.5578315e+01);
  EXPECT_LT(psnr, 6.5578317e+01);
  EXPECT_LT(lmax, tol);

  rtn = tester.execute(-2, tol);
  EXPECT_EQ(rtn, 0);
  psnr = tester.get_psnr();
  lmax = tester.get_lmax();
  EXPECT_GT(psnr, 7.2108818e+01);
  EXPECT_LT(psnr, 7.2108826e+01);
  EXPECT_LT(lmax, 5.623770e-01);
}
TEST(speck3d_qz_term, small_tolerance)
{
  const double tol = 0.07;
  speck_tester tester("../test_data/wmag128.float", {128, 128, 128});
  auto rtn = tester.execute(-3, tol);
  EXPECT_EQ(rtn, 0);
  float psnr = tester.get_psnr();
  float lmax = tester.get_lmax();
  EXPECT_GT(psnr, 8.1451713e+01);
  EXPECT_LT(psnr, 8.1451722e+01);
  EXPECT_LT(lmax, 6.9999696e-02);

  rtn = tester.execute(-5, tol);
  EXPECT_EQ(rtn, 0);
  psnr = tester.get_psnr();
  lmax = tester.get_lmax();
  EXPECT_GT(psnr, 9.1753349e+01);
  EXPECT_LT(psnr, 9.1753350e+01);
  EXPECT_LE(lmax, 5.3462983e-02);
}
TEST(speck3d_qz_term, narrow_data_range)
{
  speck_tester tester("../test_data/vorticity.128_128_41", {128, 128, 41});
  auto rtn = tester.execute(-16, 3e-5);
  EXPECT_EQ(rtn, 0);
  float psnr = tester.get_psnr();
  float lmax = tester.get_lmax();
  EXPECT_GT(psnr, 42.293655);
  EXPECT_LT(psnr, 42.293656);
  EXPECT_LT(lmax, 2.983993e-05);

  rtn = tester.execute(-18, 7e-6);
  EXPECT_EQ(rtn, 0);
  psnr = tester.get_psnr();
  lmax = tester.get_lmax();
  EXPECT_GT(psnr, 5.05222282e+01);
  EXPECT_LT(psnr, 5.05222283e+01);
  EXPECT_LT(lmax, 6.997870e-06);
}
TEST(speck3d_qz_term_omp, narrow_data_range)
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
TEST(speck3d_qz_term_omp, small_tolerance)
{
  speck_tester_omp tester("../test_data/wmag128.float", {128, 128, 128}, 7);
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
  speck_tester tester("../test_data/wmag17.float", {17, 17, 17});

  tester.execute(4.0);
  float psnr = tester.get_psnr();
  float lmax = tester.get_lmax();
  EXPECT_GT(psnr, 53.1573);
  EXPECT_LT(psnr, 53.1574);
  EXPECT_LT(lmax, 1.8177);

  tester.execute(2.0);
  psnr = tester.get_psnr();
  lmax = tester.get_lmax();
  EXPECT_GT(psnr, 41.5385);
  EXPECT_LT(psnr, 41.5386);
  EXPECT_LT(lmax, 6.329695);

  tester.execute(1.0);
  psnr = tester.get_psnr();
  lmax = tester.get_lmax();
  EXPECT_GT(psnr, 35.0593);
  EXPECT_LT(psnr, 35.0594);
  EXPECT_LT(lmax, 12.0156);
}

TEST(speck3d_bit_rate, big)
{
  speck_tester tester("../test_data/wmag128.float", {128, 128, 128});

  tester.execute(2.0);
  float psnr = tester.get_psnr();
  float lmax = tester.get_lmax();
  EXPECT_GT(psnr, 54.8401);
  EXPECT_LT(psnr, 54.8402);
  EXPECT_LT(lmax, 4.9368744e+00);

  tester.execute(1.0);
  psnr = tester.get_psnr();
  lmax = tester.get_lmax();
  EXPECT_GT(psnr, 48.0096);
  EXPECT_LT(psnr, 48.0097);
  EXPECT_LT(lmax, 1.0140460e+01);

  tester.execute(0.5);
  psnr = tester.get_psnr();
  lmax = tester.get_lmax();
  EXPECT_GT(psnr, 43.4279);
  EXPECT_LT(psnr, 43.42795);
  EXPECT_LT(lmax, 2.2674592e+01);

  tester.execute(0.25);
  psnr = tester.get_psnr();
  lmax = tester.get_lmax();
  EXPECT_GT(psnr, 39.8506);
  EXPECT_LT(psnr, 39.8507);
  EXPECT_LT(lmax, 3.9112084e+01);
}

TEST(speck3d_bit_rate, narrow_data_range)
{
  speck_tester tester("../test_data/vorticity.128_128_41", {128, 128, 41});

  tester.execute(4.0);
  float psnr = tester.get_psnr();
  float lmax = tester.get_lmax();
  EXPECT_GT(psnr, 69.0418);
  EXPECT_LT(psnr, 69.0419);
  EXPECT_LT(lmax, 9.103715e-07);

  tester.execute(2.0);
  psnr = tester.get_psnr();
  lmax = tester.get_lmax();
  EXPECT_GT(psnr, 56.7857);
  EXPECT_LT(psnr, 56.7858);
  EXPECT_LT(lmax, 4.199554e-06);

  tester.execute(1.0);
  psnr = tester.get_psnr();
  lmax = tester.get_lmax();
  EXPECT_GT(psnr, 49.7494);
  EXPECT_LT(psnr, 49.7495);
  EXPECT_LT(lmax, 1.0023e-05);

  tester.execute(0.5);
  psnr = tester.get_psnr();
  lmax = tester.get_lmax();
  EXPECT_GT(psnr, 45.2061);
  EXPECT_LT(psnr, 45.2062);
  EXPECT_LT(lmax, 0.000024);

  tester.execute(0.25);
  psnr = tester.get_psnr();
  lmax = tester.get_lmax();
  EXPECT_GT(psnr, 41.7523);
  EXPECT_LT(psnr, 41.7524);
  EXPECT_LT(lmax, 3.329716e-05);
}

TEST(speck3d_bit_rate_omp, narrow_data_range)
{
  speck_tester_omp tester("../test_data/vorticity.128_128_41", {128, 128, 41}, 1);

  tester.execute(4.0);
  float psnr = tester.get_psnr();
  float lmax = tester.get_lmax();
  EXPECT_GT(psnr, 67.8000);
  EXPECT_LT(psnr, 67.80005);
  EXPECT_LT(lmax, 1.175035e-06);

  tester.execute(1.0);
  psnr = tester.get_psnr();
  lmax = tester.get_lmax();
  EXPECT_GT(psnr, 48.9001);
  EXPECT_LT(psnr, 48.9002);
  EXPECT_LT(lmax, 1.396333e-05);
}

TEST(speck3d_bit_rate_omp, big)
{
  speck_tester_omp tester("../test_data/wmag128.float", {128, 128, 128}, 8);

  tester.execute(2.0);
  float psnr = tester.get_psnr();
  float lmax = tester.get_lmax();
  EXPECT_GT(psnr, 53.8137);
  EXPECT_LT(psnr, 53.8138);
  EXPECT_LT(lmax, 7.6953269e+00);

  tester.execute(1.0);
  psnr = tester.get_psnr();
  lmax = tester.get_lmax();
  EXPECT_GT(psnr, 47.2266);
  EXPECT_LT(psnr, 47.2267);
  EXPECT_LT(lmax, 1.6300519e+01);
}

#endif

}  // namespace
