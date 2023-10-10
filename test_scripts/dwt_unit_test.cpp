#include <cstdlib>
#include "CDF97_PL.h"
#include "Conditioner.h"
#include "gtest/gtest.h"

#include <numeric>

namespace {

#if 0
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

  // Use a sperr::CDF97_PL to perform DWT and IDWT.
  sperr::CDF97_PL cdf;
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
#endif


TEST(dwt3d, big_odd_cube)
{
  const char* input = "./test_data/den.133.0mean";
  size_t dim_x = 133, dim_y = 133, dim_z = 133;
  const size_t total_vals = dim_x * dim_y * dim_z;

  // Let read in binaries as 8-byte floats
  auto in_buf = sperr::read_whole_file<double>(input);
  if (in_buf.size() != total_vals)
    std::cerr << "Input read error!" << std::endl;

  auto [min, max] = std::minmax_element(in_buf.begin(), in_buf.end());
  std::cout << "Input min max: " << *min << ",  " << *max << std::endl;

  // Use a sperr::CDF97_PL to perform DWT and IDWT.
  sperr::CDF97_PL cdf;
  cdf.copy_data(in_buf.data(), dim_x * dim_y * dim_z, {dim_x, dim_y, dim_z});

  cdf.dwt3d_dyadic_pl();

  const auto& coeffs = cdf.view_data();
  auto [min2, max2] = std::minmax_element(coeffs.cbegin(), coeffs.cend());
  std::cout << "Coefficient min max: " << (long double)(*min2) << ",  " << 
                                          (long double)(*max2) << std::endl;
  auto absmin = std::reduce(coeffs.cbegin(), coeffs.cend(), __Float128{1.0},
                     [](auto v1, auto v2) { return std::min(std::abs(__Float128{v1}), 
                                                            std::abs(__Float128{v2})); });
  std::cout << "Coefficient abs min: " << (long double)(absmin) << std::endl;

  cdf.idwt3d_dyadic_pl();

  // Claim that with single precision, the result is identical to the input
  const auto& results = cdf.view_data();

  auto maxdiff = std::transform_reduce(in_buf.begin(), in_buf.end(), results.cbegin(), __Float128{0.0},
                 [](auto v1, auto v2) { return std::max(__Float128(v1), __Float128(v2)); }, // reduction op
                 [](auto v1, auto v2) { return std::abs(__Float128(v1) - __Float128(v2)); }); // transform op
  std::cout << "max diff = " << (long double)(maxdiff) << std::endl;

  //for (size_t i = 0; i < total_vals; i++) {
  //  auto x = i % dim_x;
  //  auto z = i / (dim_x * dim_y);
  //  auto y = (i - z * dim_x * dim_y) / dim_x;
  //  ASSERT_EQ(in_buf[i], float(result[i])) << "at (x, y, z) = ("
  //                                         << x << ", " << y << ", " << z << ")";
  //}
}


}  // namespace
