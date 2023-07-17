#include "SPERR3D_Stream_Tools.h"
#include "SPERR3D_OMP_C.h"

#include "gtest/gtest.h"

namespace {

using sperr::RTNType;

// Test constant field
TEST(stream_tools, constant_1chunk)
{
  // Produce a bigstream to disk
  auto filename = std::string("./test.tmp");
  auto input = sperr::read_whole_file<float>("../test_data/const32x20x16.float");
  assert(!input.empty());
  auto encoder = sperr::SPERR3D_OMP_C();
  encoder.set_dims_and_chunks({32, 20, 16}, {32, 20, 16});
  encoder.set_psnr(99.0);
  encoder.compress(input.data(), input.size());
  auto stream = encoder.get_encoded_bitstream();
  sperr::write_n_bytes(filename, stream.size(), stream.data());

  // Test progressive read!
  auto tools = sperr::SPERR3D_Stream_Tools();
  auto part = tools.progressive_read(filename, 10.); // populates data fields inside of `tools`
  auto header_len = tools.header_len;

  // Header should (mostly) remain the same.
  EXPECT_EQ(part[0], stream[0]);
  EXPECT_EQ(part[1], stream[1] + 128);
  for (size_t i = 2; i < tools.header_len; i++)
    EXPECT_EQ(part[i], stream[i]);

  // The header of each chunk (first 17 bytes) should also remain the same.
  //    To know offsets of each chunk of the new portioned bitstream, we use another
  //    stream tool to parse it.
  auto tools_part = sperr::SPERR3D_Stream_Tools();
  tools_part.populate_stream_info(part.data());
  EXPECT_EQ(tools.chunk_offsets.size(), tools_part.chunk_offsets.size());

  for (size_t i = 0; i < tools.chunk_offsets.size() / 2; i++) {
    auto orig_start = tools.chunk_offsets[i * 2];
    auto part_start = tools_part.chunk_offsets[i * 2];
    for (size_t j = 0; j < 17 ; j++)
      EXPECT_EQ(stream[orig_start + j], part[part_start + j]);
  }
}

TEST(stream_tools, constant_nchunks)
{
  // Produce a bigstream to disk
  auto filename = std::string("./test.tmp");
  auto input = sperr::read_whole_file<float>("../test_data/const32x20x16.float");
  assert(!input.empty());
  auto encoder = sperr::SPERR3D_OMP_C();
  encoder.set_dims_and_chunks({32, 20, 16}, {10, 10, 8});
  encoder.set_psnr(99.0);
  encoder.compress(input.data(), input.size());
  auto stream = encoder.get_encoded_bitstream();
  sperr::write_n_bytes(filename, stream.size(), stream.data());

  // Test progressive read!
  auto tools = sperr::SPERR3D_Stream_Tools();
  auto part = tools.progressive_read(filename, 10.); // populates data fields inside of `tools`
  auto header_len = tools.header_len;

  // Header should (mostly) remain the same.
  EXPECT_EQ(part[0], stream[0]);
  EXPECT_EQ(part[1], stream[1] + 128);
  for (size_t i = 2; i < tools.header_len; i++)
    EXPECT_EQ(part[i], stream[i]);

  // The header of each chunk (first 17 bytes) should also remain the same.
  //    To know offsets of each chunk of the new portioned bitstream, we use another
  //    stream tool to parse it.
  auto tools_part = sperr::SPERR3D_Stream_Tools();
  tools_part.populate_stream_info(part.data());
  EXPECT_EQ(tools.chunk_offsets.size(), tools_part.chunk_offsets.size());

  for (size_t i = 0; i < tools.chunk_offsets.size() / 2; i++) {
    auto orig_start = tools.chunk_offsets[i * 2];
    auto part_start = tools_part.chunk_offsets[i * 2];
    for (size_t j = 0; j < 17; j++)
      EXPECT_EQ(stream[orig_start + j], part[part_start + j]);
  }
}

//
// Test a non-constant field.
//
TEST(stream_tools, regular_nchunks)
{
  // Produce a bigstream to disk
  auto filename = std::string("./test.tmp");
  auto input = sperr::read_whole_file<float>("../test_data/vorticity.128_128_41");
  assert(!input.empty());
  auto encoder = sperr::SPERR3D_OMP_C();
  encoder.set_dims_and_chunks({128, 128, 41}, {20, 20, 8});
  encoder.set_psnr(99.0);
  encoder.compress(input.data(), input.size());
  auto stream = encoder.get_encoded_bitstream();
  sperr::write_n_bytes(filename, stream.size(), stream.data());

  // Test progressive read!
  auto tools = sperr::SPERR3D_Stream_Tools();
  auto part = tools.progressive_read(filename, 10.); // populates data fields inside of `tools`
  auto header_len = tools.header_len;

  // Header should (mostly) remain the same.
  EXPECT_EQ(part[0], stream[0]);
  EXPECT_EQ(part[1], stream[1] + 128);
  for (size_t i = 2; i < tools.header_len; i++)
    EXPECT_EQ(part[i], stream[i]);

  // The header of each chunk (first 9 bytes) should also remain the same.
  //    To know offsets of each chunk of the new portioned bitstream, we use another
  //    stream tool to parse it.
  auto tools_part = sperr::SPERR3D_Stream_Tools();
  tools_part.populate_stream_info(part.data());
  EXPECT_EQ(tools.chunk_offsets.size(), tools_part.chunk_offsets.size());

  for (size_t i = 0; i < tools.chunk_offsets.size() / 2; i++) {
    auto orig_start = tools.chunk_offsets[i * 2];
    auto part_start = tools_part.chunk_offsets[i * 2];
    for (size_t j = 0; j < 9; j++)
      EXPECT_EQ(stream[orig_start + j], part[part_start + j]);
  }
}

} // anonymous namespace