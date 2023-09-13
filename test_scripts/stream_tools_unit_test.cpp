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
  auto part = tools.progressive_read(filename, 100); // also populates data fields inside of `tools`

  // The returned bitstream should remain the same, because we requested way more bit budget
  // than the compressed bitstream has.
  EXPECT_EQ(part, stream);
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
  auto part = tools.progressive_read(filename, 50); // also populates data fields inside of `tools`.

  // The returned bitstream should (largely) remain the same, because the header of a constant 
  // is always recorded.
  EXPECT_EQ(part.size(), stream.size());
  EXPECT_EQ(part[0], stream[0]);
  EXPECT_EQ(part[1], stream[1] + 128);
  for (size_t i = 2; i < part.size(); i++)
    EXPECT_EQ(part[i], stream[i]);
}

//
// Test a non-constant field.
//
TEST(stream_tools, regular_1chunk)
{
  // Produce a bigstream to disk
  auto filename = std::string("./test.tmp");
  auto input = sperr::read_whole_file<float>("../test_data/vorticity.128_128_41");
  assert(!input.empty());
  auto encoder = sperr::SPERR3D_OMP_C();
  encoder.set_dims_and_chunks({128, 128, 41}, {128, 128, 41});
  encoder.set_psnr(100.0);  // Resulting about 10bpp.
  encoder.compress(input.data(), input.size());
  auto stream = encoder.get_encoded_bitstream();
  sperr::write_n_bytes(filename, stream.size(), stream.data());

  // Test progressive read!
  auto tools = sperr::SPERR3D_Stream_Tools();
  auto part = tools.progressive_read(filename, 50);

  // Header should (mostly) remain the same.
  EXPECT_EQ(part[0], stream[0]);
  EXPECT_EQ(part[1], stream[1] + 128);
  for (size_t i = 2; i < tools.header_len - 4; i++) // Exclude the last 4 bytes (chunk len).
    EXPECT_EQ(part[i], stream[i]);

  // The header of each chunk (first 26 bytes) should also remain the same.
  //    To know offsets of each chunk of the new portioned bitstream, we use another
  //    stream tool to parse it.
  auto tools_part = sperr::SPERR3D_Stream_Tools();
  tools_part.populate_stream_info(part.data());
  EXPECT_EQ(tools.chunk_offsets.size(), tools_part.chunk_offsets.size());

  for (size_t i = 0; i < tools.chunk_offsets.size() / 2; i++) {
    auto orig_start = tools.chunk_offsets[i * 2];
    auto part_start = tools_part.chunk_offsets[i * 2];
    // We actually test first 90 bytes. There must be 90 bytes there because of the 50% bytes request.
    for (size_t j = 0; j < 90; j++)
      EXPECT_EQ(stream[orig_start + j], part[part_start + j]);
  }
}

TEST(stream_tools, regular_nchunks)
{
  // Produce a bigstream to disk
  auto filename = std::string("./test.tmp");
  auto input = sperr::read_whole_file<float>("../test_data/vorticity.128_128_41");
  assert(!input.empty());
  auto encoder = sperr::SPERR3D_OMP_C();
  encoder.set_dims_and_chunks({128, 128, 41}, {31, 40, 21});
  encoder.set_psnr(100.0);  // Resulting about 10bpp.
  encoder.compress(input.data(), input.size());
  auto stream = encoder.get_encoded_bitstream();
  sperr::write_n_bytes(filename, stream.size(), stream.data());

  // Test progressive read!
  auto tools = sperr::SPERR3D_Stream_Tools();
  auto part = tools.progressive_read(filename, 35);

  // Header should (mostly) remain the same.
  EXPECT_EQ(part[0], stream[0]);
  EXPECT_EQ(part[1], stream[1] + 128);

  // The header of each chunk (first 26 bytes) should also remain the same.
  //    To know offsets of each chunk of the new portioned bitstream, we use another
  //    stream tool to parse it.
  auto tools_part = sperr::SPERR3D_Stream_Tools();
  tools_part.populate_stream_info(part.data());
  EXPECT_EQ(tools.chunk_offsets.size(), tools_part.chunk_offsets.size());

  for (size_t i = 0; i < tools.chunk_offsets.size() / 2; i++) {
    auto orig_start = tools.chunk_offsets[i * 2];
    auto part_start = tools_part.chunk_offsets[i * 2];
    // We actually test first 30 bytes. There must be 30 bytes there because of the 35% bytes request.
    for (size_t j = 0; j < 30; j++)
      EXPECT_EQ(stream[orig_start + j], part[part_start + j]);
  }
}

TEST(stream_tools, min_chunk_len)
{
  // Produce a bigstream to disk
  auto filename = std::string("./test.tmp");
  auto input = sperr::read_whole_file<float>("../test_data/wmag17.float");
  assert(!input.empty());
  auto encoder = sperr::SPERR3D_OMP_C();
  encoder.set_dims_and_chunks({17, 17, 17}, {8, 8, 8});
  encoder.set_psnr(100.0);
  encoder.compress(input.data(), input.size());
  auto stream = encoder.get_encoded_bitstream();
  sperr::write_n_bytes(filename, stream.size(), stream.data());

  // Test progressive read! The requested 1% of bytes results in every chunk to have ~10 bytes,
  // which would be changed to 26 bytes eventually, and not resulting in failed assertions.
  auto tools = sperr::SPERR3D_Stream_Tools();
  auto part = tools.progressive_read(filename, 1);

  // Header should (mostly) remain the same.
  EXPECT_EQ(part[0], stream[0]);
  EXPECT_EQ(part[1], stream[1] + 128);

  // The header of each chunk (first 26 bytes) should also remain the same.
  //    To know offsets of each chunk of the new portioned bitstream, we use another
  //    stream tool to parse it.
  auto tools_part = sperr::SPERR3D_Stream_Tools();
  tools_part.populate_stream_info(part.data());
  EXPECT_EQ(tools.chunk_offsets.size(), tools_part.chunk_offsets.size());

  for (size_t i = 0; i < tools.chunk_offsets.size() / 2; i++) {
    auto orig_start = tools.chunk_offsets[i * 2];
    auto part_start = tools_part.chunk_offsets[i * 2];
    // We actually test first 26 bytes.
    for (size_t j = 0; j < 26; j++)
      EXPECT_EQ(stream[orig_start + j], part[part_start + j]);
  }
}

} // anonymous namespace
