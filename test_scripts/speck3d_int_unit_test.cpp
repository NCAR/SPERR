#include "SPECK3D_INT_ENC.h"
#include "SPECK3D_INT_DEC.h"

#include "gtest/gtest.h"

#include <algorithm>
#include <cmath>
#include <random>

namespace {

auto ProduceRandomArray(size_t len, float stddev, uint32_t seed) 
    -> std::pair<std::vector<uint64_t>, std::vector<bool>>
{
  std::mt19937 gen{seed};
  std::normal_distribution<float> d{0.0, stddev};

  auto tmp = std::vector<float>(len);
  std::generate(tmp.begin(), tmp.end(), [&gen, &d](){ return d(gen); });

  auto coeffs = std::vector<uint64_t>(len);
  auto signs = std::vector<bool>(len, true);
  for (size_t i = 0; i < len; i++) {
    coeffs[i] = std::round(std::abs(tmp[i]));
    if (coeffs[i] != 0)
      signs[i] = tmp[i] > 0.f;
  }

  return {coeffs, signs};
} 

//
// Test a minimal test case
//
TEST(Speck3dInt, minimal)
{
  const auto dims = sperr::dims_type{4, 3, 8};
  const auto total_vals = dims[0] * dims[1] * dims[2];

  auto input = sperr::veci_t(total_vals, 0);
  auto input_signs = sperr::vecb_type(input.size(), true);
  input[4] = 1;
  input[7] = 3; input_signs[7] = false;
  input[10] = 7;
  input[11] = 9; input_signs[11] = false;
  input[16] = 10;
  input[19] = 12; input_signs[19] = false;
  input[26] = 18;
  input[29] = 19; input_signs[29] = false;
  input[32] = 32;
  input[39] = 32; input_signs[39] = false;

  // Encode
  auto encoder = sperr::SPECK3D_INT_ENC();
  encoder.use_coeffs(input, input_signs);
  encoder.set_dims(dims);
  encoder.encode();
  const auto& bitstream = encoder.view_encoded_bitstream();

  // Decode
  auto decoder = sperr::SPECK3D_INT_DEC();
  decoder.set_dims(dims);
  decoder.use_bitstream(bitstream);
  decoder.decode();
  auto output = decoder.release_coeffs();
  auto output_signs = decoder.release_signs();

  EXPECT_EQ(input, output);
  EXPECT_EQ(input_signs, output_signs);
}

TEST(Speck3dInt, Random1)
{
  const auto dims = sperr::dims_type{10, 20, 30};
  const auto total_vals = dims[0] * dims[1] * dims[2];

  auto [input, input_signs] = ProduceRandomArray(total_vals, 2.9, 1); 

  // Encode
  auto encoder = sperr::SPECK3D_INT_ENC();
  encoder.use_coeffs(input, input_signs);
  encoder.set_dims(dims);
  encoder.encode();
  const auto& bitstream = encoder.view_encoded_bitstream();

  // Decode
  auto decoder = sperr::SPECK3D_INT_DEC();
  decoder.set_dims(dims);
  decoder.use_bitstream(bitstream);
  decoder.decode();
  auto output = decoder.release_coeffs();
  auto output_signs = decoder.release_signs();

  EXPECT_EQ(input, output);
  EXPECT_EQ(input_signs, output_signs);
}

TEST(Speck3dInt, Random2)
{
  const auto dims = sperr::dims_type{63, 79, 128};
  const auto total_vals = dims[0] * dims[1] * dims[2];

  auto [input, input_signs] = ProduceRandomArray(total_vals, 499.0, 2); 

  // Encode
  auto encoder = sperr::SPECK3D_INT_ENC();
  encoder.use_coeffs(input, input_signs);
  encoder.set_dims(dims);
  encoder.encode();
  const auto& bitstream = encoder.view_encoded_bitstream();

  // Decode
  auto decoder = sperr::SPECK3D_INT_DEC();
  decoder.set_dims(dims);
  decoder.use_bitstream(bitstream);
  decoder.decode();
  auto output = decoder.release_coeffs();
  auto output_signs = decoder.release_signs();

  EXPECT_EQ(input, output);
  EXPECT_EQ(input_signs, output_signs);
}

TEST(Speck3dInt, Random3)
{
  const auto dims = sperr::dims_type{127, 155, 280};
  const auto total_vals = dims[0] * dims[1] * dims[2];

  auto [input, input_signs] = ProduceRandomArray(total_vals, 1234.5, 3); 

  // Encode
  auto encoder = sperr::SPECK3D_INT_ENC();
  encoder.use_coeffs(input, input_signs);
  encoder.set_dims(dims);
  encoder.encode();
  const auto& bitstream = encoder.view_encoded_bitstream();

  // Decode
  auto decoder = sperr::SPECK3D_INT_DEC();
  decoder.set_dims(dims);
  decoder.use_bitstream(bitstream);
  decoder.decode();
  auto output = decoder.release_coeffs();
  auto output_signs = decoder.release_signs();

  EXPECT_EQ(input, output);
  EXPECT_EQ(input_signs, output_signs);
}

TEST(Speck3dInt, RandomRandom)
{
  const auto dims = sperr::dims_type{63, 64, 119};
  const auto total_vals = dims[0] * dims[1] * dims[2];

  auto rd = std::random_device();
  auto [input, input_signs] = ProduceRandomArray(total_vals, 8345.3, rd()); 

  // Encode
  auto encoder = sperr::SPECK3D_INT_ENC();
  encoder.use_coeffs(input, input_signs);
  encoder.set_dims(dims);
  encoder.encode();
  const auto& bitstream = encoder.view_encoded_bitstream();

  // Decode
  auto decoder = sperr::SPECK3D_INT_DEC();
  decoder.set_dims(dims);
  decoder.use_bitstream(bitstream);
  decoder.decode();
  auto output = decoder.release_coeffs();
  auto output_signs = decoder.release_signs();

  EXPECT_EQ(input, output);
  EXPECT_EQ(input_signs, output_signs);
}


}  // namespace
