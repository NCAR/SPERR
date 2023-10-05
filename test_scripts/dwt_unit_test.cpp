#include <cstdlib>
#include "CDF97.h"
#include "Conditioner.h"
#include "gtest/gtest.h"

namespace {

TEST(dwt1d, big_image_even)
{
  const char* input = "../test_data/128x128.float";
  size_t dim_x = 128;
  const size_t total_vals = dim_x;

  // Let read in binaries as 4-byte floats
  auto in_buf = sperr::read_n_bytes(input, sizeof(float) * total_vals);
  if (in_buf.size() != sizeof(float) * total_vals)
    std::cerr << "Input read error!" << std::endl;
  const float* fptr = reinterpret_cast<const float*>(in_buf.data());

  // Make a copy and then use a conditioner
  auto in_copy = sperr::vecd_type(total_vals);
  std::copy(fptr, fptr + total_vals, in_copy.begin());
  auto condi = sperr::Conditioner();
  auto meta = condi.condition(in_copy, {dim_x, 1, 1});

  // Use a sperr::CDF97 to perform DWT and IDWT.
  sperr::CDF97 cdf;
  cdf.take_data(std::move(in_copy), {dim_x, 1, 1});
  cdf.dwt1d_pl();
  cdf.idwt1d_pl();

  // Claim that with single precision, the result is identical to the input
  auto result = cdf.release_data();
  EXPECT_EQ(result.size(), total_vals);

  // Apply the conditioner
  auto rtn = condi.inverse_condition(result, {dim_x, 1, 1}, meta);

  for (size_t i = 0; i < total_vals; i++) {
    EXPECT_EQ(fptr[i], float(result[i]));
  }
}

TEST(dwt1d, big_image_odd)
{
  const char* input = "../test_data/999x999.float";
  size_t dim_x = 999;
  const size_t total_vals = dim_x;

  // Let read in binaries as 4-byte floats
  auto in_buf = sperr::read_n_bytes(input, sizeof(float) * total_vals);
  if (in_buf.size() != sizeof(float) * total_vals)
    std::cerr << "Input read error!" << std::endl;
  const float* fptr = reinterpret_cast<const float*>(in_buf.data());

  // Make a copy and use a conditioner
  auto in_copy = sperr::vecd_type(total_vals);
  std::copy(fptr, fptr + total_vals, in_copy.begin());
  auto condi = sperr::Conditioner();
  auto meta = condi.condition(in_copy, {dim_x, 1, 1});

  // Use a sperr::CDF97 to perform DWT and IDWT.
  sperr::CDF97 cdf;
  cdf.take_data(std::move(in_copy), {dim_x, 1, 1});
  cdf.dwt1d_pl();
  cdf.idwt1d_pl();

  // Claim that with single precision, the result is identical to the input
  auto result = cdf.release_data();
  EXPECT_EQ(result.size(), total_vals);

  // Apply the conditioner again
  auto rtn = condi.inverse_condition(result, {dim_x, 1, 1}, meta);

  for (size_t i = 0; i < total_vals; i++) {
    EXPECT_EQ(fptr[i], float(result[i]));
  }
}

TEST(dwt2d, small_image_even)
{
  const char* input = "../test_data/16x16.float";
  size_t dim_x = 16, dim_y = 16;
  const size_t total_vals = dim_x * dim_y;

  // Let read in binaries as 4-byte floats
  auto in_buf = sperr::read_whole_file<float>(input);
  if (in_buf.size() != total_vals)
    std::cerr << "Input read error!" << std::endl;

  // Make a copy and use a conditioner
  auto in_copy = sperr::vecd_type(total_vals);
  std::copy(in_buf.begin(), in_buf.end(), in_copy.begin());
  auto condi = sperr::Conditioner();
  auto meta = condi.condition(in_copy, {dim_x, dim_y, 1});

  // Use a sperr::CDF97 to perform DWT and IDWT.
  sperr::CDF97 cdf;
  cdf.take_data(std::move(in_copy), {dim_x, dim_y, 1});
  cdf.dwt2d_xy_pl();
  cdf.idwt2d_xy_pl();

  // Claim that with single precision, the result is identical to the input
  auto result = cdf.release_data();
  EXPECT_EQ(result.size(), total_vals);

  // Apply the conditioner again
  auto rtn = condi.inverse_condition(result, {dim_x, dim_y, 1}, meta);

  //for (size_t i = 0; i < total_vals; i++) {
  for (size_t i = 0; i < 10; i++) {
    EXPECT_EQ(in_buf[i], float(result[i]));
  }
}

TEST(dwt2d, small_image_odd)
{
  const char* input = "../test_data/15x15.float";
  size_t dim_x = 15, dim_y = 15;
  const size_t total_vals = dim_x * dim_y;

  // Let read in binaries as 4-byte floats
  auto in_buf = sperr::read_whole_file<float>(input);
  if (in_buf.size() != total_vals)
    std::cerr << "Input read error!" << std::endl;

  // Make a copy and use a conditioner
  auto in_copy = sperr::vecd_type(total_vals);
  std::copy(in_buf.begin(), in_buf.end(), in_copy.begin());
  auto condi = sperr::Conditioner();
  auto meta = condi.condition(in_copy, {dim_x, dim_y, 1});

  // Use a sperr::CDF97 to perform DWT and IDWT.
  sperr::CDF97 cdf;
  cdf.take_data(std::move(in_copy), {dim_x, dim_y, 1});
  cdf.dwt2d_xy_pl();
  cdf.idwt2d_xy_pl();

  // Claim that with single precision, the result is identical to the input
  auto result = cdf.release_data();
  EXPECT_EQ(result.size(), total_vals);

  // Apply the conditioner
  auto rtn = condi.inverse_condition(result, {dim_x, dim_y, 1}, meta);

  for (size_t i = 0; i < total_vals; i++) {
    EXPECT_EQ(in_buf[i], float(result[i]));
  }
}

TEST(dwt2d, big_image_even)
{
  const char* input = "../test_data/128x128.float";
  size_t dim_x = 128, dim_y = 128;
  const size_t total_vals = dim_x * dim_y;

  // Let read in binaries as 4-byte floats
  auto in_buf = sperr::read_whole_file<float>(input);
  if (in_buf.size() != total_vals)
    std::cerr << "Input read error!" << std::endl;

  // Make a copy and use a conditioner
  auto in_copy = sperr::vecd_type(total_vals);
  std::copy(in_buf.begin(), in_buf.end(), in_copy.begin());
  auto condi = sperr::Conditioner();
  auto meta = condi.condition(in_copy, {dim_x, dim_y, 1});

  // Use a sperr::CDF97 to perform DWT and IDWT.
  sperr::CDF97 cdf;
  cdf.take_data(std::move(in_copy), {dim_x, dim_y, 1});
  cdf.dwt2d_xy_pl();
  cdf.idwt2d_xy_pl();

  // Claim that with single precision, the result is identical to the input
  auto result = cdf.release_data();
  EXPECT_EQ(result.size(), total_vals);

  // Apply the conditioner
  auto rtn = condi.inverse_condition(result, {dim_x, dim_y, 1}, meta);

  for (size_t i = 0; i < total_vals; i++) {
    EXPECT_EQ(in_buf[i], float(result[i]));
  }
}

TEST(dwt2d, big_image_odd)
{
  const char* input = "../test_data/127x127.float";
  size_t dim_x = 127, dim_y = 127;
  const size_t total_vals = dim_x * dim_y;

  // Let read in binaries as 4-byte floats
  auto in_buf = sperr::read_whole_file<float>(input);
  if (in_buf.size() != total_vals)
    std::cerr << "Input read error!" << std::endl;

  // Make a copy and use a conditioner
  auto in_copy = sperr::vecd_type(total_vals);
  std::copy(in_buf.begin(), in_buf.end(), in_copy.begin());
  auto condi = sperr::Conditioner();
  auto meta = condi.condition(in_copy, {dim_x, dim_y, 1});

  // Use a sperr::CDF97 to perform DWT and IDWT.
  sperr::CDF97 cdf;
  cdf.take_data(std::move(in_copy), {dim_x, dim_y, 1});
  cdf.dwt2d_xy_pl();
  cdf.idwt2d_xy_pl();

  // Claim that with single precision, the result is identical to the input
  auto result = cdf.release_data();
  EXPECT_EQ(result.size(), total_vals);

  // Apply the conditioner
  auto rtn = condi.inverse_condition(result, {dim_x, dim_y, 1}, meta);

  for (size_t i = 0; i < total_vals; i++) {
    auto x = i % dim_x;
    auto y = i / dim_x;
    EXPECT_EQ(in_buf[i], float(result[i])) << " at (x, y) = (" << x << ", " << y << ")";
  }
}

TEST(dwt3d, small_even_cube)
{
  const char* input = "../test_data/wmag16.float";
  size_t dim_x = 16, dim_y = 16, dim_z = 16;
  const size_t total_vals = dim_x * dim_y * dim_z;

  // Let read in binaries as 4-byte floats
  auto in_buf = sperr::read_whole_file<float>(input);
  if (in_buf.size() != total_vals)
    std::cerr << "Input read error!" << std::endl;

  // Make a copy and use a conditioner
  auto in_copy = sperr::vecd_type(total_vals);
  std::copy(in_buf.begin(), in_buf.end(), in_copy.begin());
  auto condi = sperr::Conditioner();
  auto meta = condi.condition(in_copy, {dim_x, dim_y, dim_z});

  // Use a sperr::CDF97 to perform DWT and IDWT.
  sperr::CDF97 cdf;
  cdf.take_data(std::move(in_copy), {dim_x, dim_y, dim_z});
  cdf.dwt3d_dyadic_pl();
  cdf.idwt3d_dyadic_pl();

  // Claim that with single precision, the result is identical to the input
  auto result = cdf.release_data();
  EXPECT_EQ(result.size(), total_vals);

  // Apply the conditioner
  auto rtn = condi.inverse_condition(result, {dim_x, dim_y, dim_z}, meta);
  for (size_t i = 0; i < total_vals; i++) {
    EXPECT_EQ(in_buf[i], float(result[i]));
  }
}

TEST(dwt3d, big_odd_cube)
{
  const char* input = "../test_data/wmag91.float";
  size_t dim_x = 91, dim_y = 91, dim_z = 91;
  const size_t total_vals = dim_x * dim_y * dim_z;

  // Let read in binaries as 4-byte floats
  auto in_buf = sperr::read_whole_file<float>(input);
  if (in_buf.size() != total_vals)
    std::cerr << "Input read error!" << std::endl;

  // Make a copy and use a conditioner
  auto in_copy = sperr::vecd_type(total_vals);
  std::copy(in_buf.begin(), in_buf.end(), in_copy.begin());
  auto condi = sperr::Conditioner();
  auto meta = condi.condition(in_copy, {dim_x, dim_y, dim_z});

  auto [min, max] = std::minmax_element(in_copy.begin(), in_copy.end());
  std::cout << "Input min max: " << *min << ",  " << *max << std::endl;

  // Use a sperr::CDF97 to perform DWT and IDWT.
  sperr::CDF97 cdf;
  cdf.take_data(std::move(in_copy), {dim_x, dim_y, dim_z});
  cdf.dwt3d_dyadic_pl();
  //cdf.dwt3d();

  const auto& coeffs = cdf.view_data();
  auto [min2, max2] = std::minmax_element(coeffs.cbegin(), coeffs.cend());
  std::cout << "Coefficient min max: " << *min2 << ",  " << *max2 << std::endl;

  cdf.idwt3d_dyadic_pl();
  //cdf.idwt3d();

  // Claim that with single precision, the result is identical to the input
  auto result = cdf.release_data();
  EXPECT_EQ(result.size(), total_vals);

  // Apply the conditioner
  auto rtn = condi.inverse_condition(result, {dim_x, dim_y, dim_z}, meta);
  for (size_t i = 0; i < total_vals; i++) {
    auto x = i % dim_x;
    auto z = i / (dim_x * dim_y);
    auto y = (i - z * dim_x * dim_y) / dim_x;
    ASSERT_EQ(in_buf[i], float(result[i])) << "at (x, y, z) = ("
                                           << x << ", " << y << ", " << z << ")";
  }
}

TEST(dwt3d, big_even_cube)
{
  const char* input = "../test_data/wmag128.float";
  size_t dim_x = 128, dim_y = 128, dim_z = 128;
  const size_t total_vals = dim_x * dim_y * dim_z;

  // Let read in binaries as 4-byte floats
  auto in_buf = sperr::read_whole_file<float>(input);
  if (in_buf.size() != total_vals)
    std::cerr << "Input read error!" << std::endl;

  // Make a copy and use a conditioner
  auto in_copy = sperr::vecd_type(total_vals);
  std::copy(in_buf.begin(), in_buf.end(), in_copy.begin());
  auto condi = sperr::Conditioner();
  auto meta = condi.condition(in_copy, {dim_x, dim_y, dim_z});

  // Use a sperr::CDF97 to perform DWT and IDWT.
  sperr::CDF97 cdf;
  cdf.take_data(std::move(in_copy), {dim_x, dim_y, dim_z});
  cdf.dwt3d_dyadic_pl();

  const auto& coeffs = cdf.view_data();
  auto [min2, max2] = std::minmax_element(coeffs.cbegin(), coeffs.cend());
  std::cout << "Coefficient min max: " << *min2 << ",  " << *max2 << std::endl;

  cdf.idwt3d_dyadic_pl();

  // Claim that with single precision, the result is identical to the input
  auto result = cdf.release_data();
  EXPECT_EQ(result.size(), total_vals);

  // Apply the conditioner
  auto rtn = condi.inverse_condition(result, {dim_x, dim_y, dim_z}, meta);
  for (size_t i = 0; i < total_vals; i++) {
    ASSERT_EQ(in_buf[i], float(result[i]));
  }
}

}  // namespace
