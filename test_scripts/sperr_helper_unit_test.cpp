
#include <random>
#include "gtest/gtest.h"
#include "sperr_helper.h"

namespace {

TEST(sperr_helper, num_of_xforms)
{
  EXPECT_EQ(sperr::num_of_xforms(1), 0);
  EXPECT_EQ(sperr::num_of_xforms(7), 0);
  EXPECT_EQ(sperr::num_of_xforms(8), 1);
  EXPECT_EQ(sperr::num_of_xforms(9), 1);
  EXPECT_EQ(sperr::num_of_xforms(15), 1);
  EXPECT_EQ(sperr::num_of_xforms(16), 2);
  EXPECT_EQ(sperr::num_of_xforms(17), 2);
  EXPECT_EQ(sperr::num_of_xforms(31), 2);
  EXPECT_EQ(sperr::num_of_xforms(32), 3);
  EXPECT_EQ(sperr::num_of_xforms(63), 3);
  EXPECT_EQ(sperr::num_of_xforms(64), 4);
  EXPECT_EQ(sperr::num_of_xforms(127), 4);
  EXPECT_EQ(sperr::num_of_xforms(128), 5);
}

TEST(sperr_helper, approx_detail_len)
{
  auto len = sperr::calc_approx_detail_len(7, 0);
  EXPECT_EQ(len[0], 7);
  EXPECT_EQ(len[1], 0);
  len = sperr::calc_approx_detail_len(7, 1);
  EXPECT_EQ(len[0], 4);
  EXPECT_EQ(len[1], 3);
  len = sperr::calc_approx_detail_len(8, 1);
  EXPECT_EQ(len[0], 4);
  EXPECT_EQ(len[1], 4);
  len = sperr::calc_approx_detail_len(8, 2);
  EXPECT_EQ(len[0], 2);
  EXPECT_EQ(len[1], 2);
  len = sperr::calc_approx_detail_len(16, 2);
  EXPECT_EQ(len[0], 4);
  EXPECT_EQ(len[1], 4);
}

TEST(sperr_helper, bit_packing)
{
  const size_t num_of_bytes = 11;
  const size_t byte_offset = 1;
  std::vector<bool> input{true,  true,  true,  true,  true,  true,  true,  true,   // 1st byte
                          false, false, false, false, false, false, false, false,  // 2nd byte
                          true,  false, true,  false, true,  false, true,  false,  // 3rd byte
                          false, true,  false, true,  false, true,  false, true,   // 4th byte
                          true,  true,  false, false, true,  true,  false, false,  // 5th byte
                          false, false, true,  true,  false, false, true,  true,   // 6th byte
                          false, false, true,  true,  false, false, true,  false,  // 7th byte
                          true,  false, false, false, true,  true,  true,  false,  // 8th byte
                          false, false, false, true,  false, false, false, true,   // 9th byte
                          true,  true,  true,  false, true,  true,  true,  false,  // 10th byte
                          false, false, true,  true,  true,  false, false, true};  // 11th byte

  auto bytes = sperr::vec8_type(num_of_bytes + byte_offset);

  // Pack booleans
  auto rtn1 = sperr::pack_booleans(bytes, input, byte_offset);
  EXPECT_EQ(rtn1, sperr::RTNType::Good);
  // Unpack booleans
  auto output = std::vector<bool>(num_of_bytes * 8);
  auto rtn2 = sperr::unpack_booleans(output, bytes.data(), bytes.size(), byte_offset);
  EXPECT_EQ(rtn2, sperr::RTNType::Good);

  if (rtn1 == sperr::RTNType::Good && rtn1 == rtn2) {
    for (size_t i = 0; i < input.size(); i++)
      EXPECT_EQ(input[i], output[i]);
  }
}

TEST(sperr_helper, bit_packing_one_byte)
{
  // All true
  auto input = std::array<bool, 8>{true, true, true, true, true, true, true, true};
  // Pack booleans
  auto byte = sperr::pack_8_booleans(input);
  // Unpack booleans
  auto output = sperr::unpack_8_booleans(byte);
  for (size_t i = 0; i < 8; i++)
    EXPECT_EQ(input[i], output[i]);

  // Odd locations false
  for (size_t i = 1; i < 8; i += 2)
    input[i] = false;
  byte = sperr::pack_8_booleans(input);
  output = sperr::unpack_8_booleans(byte);
  for (size_t i = 0; i < 8; i++)
    EXPECT_EQ(input[i], output[i]);

  // All false
  for (bool& val : input)
    val = false;
  byte = sperr::pack_8_booleans(input);
  output = sperr::unpack_8_booleans(byte);
  for (size_t i = 0; i < 8; i++)
    EXPECT_EQ(input[i], output[i]);

  // Odd locations true
  for (size_t i = 1; i < 8; i += 2)
    input[i] = true;
  byte = sperr::pack_8_booleans(input);
  output = sperr::unpack_8_booleans(byte);
  for (size_t i = 0; i < 8; i++)
    EXPECT_EQ(input[i], output[i]);
}

TEST(sperr_helper, bit_packing_1032_bools)
{
  const size_t num_of_bits = 1032;  // 1024 + 8, so the last stride is partial.
  const size_t num_of_bytes = num_of_bits / 8;
  const size_t byte_offset = 23;

  std::random_device rd;   // Will be used to obtain a seed for the random number engine
  std::mt19937 gen(rd());  // Standard mersenne_twister_engine seeded with rd()
  std::uniform_int_distribution<> distrib(0, 2);
  auto input = std::vector<bool>(num_of_bits);
  auto bytes = sperr::vec8_type(num_of_bytes + byte_offset);
  for (size_t i = 0; i < num_of_bits; i++)
    input[i] = distrib(gen);

  // Pack booleans
  auto rtn1 = sperr::pack_booleans(bytes, input, byte_offset);
  EXPECT_EQ(rtn1, sperr::RTNType::Good);
  // Unpack booleans
  auto output = std::vector<bool>(num_of_bits);
  auto rtn2 = sperr::unpack_booleans(output, bytes.data(), bytes.size(), byte_offset);
  EXPECT_EQ(rtn2, sperr::RTNType::Good);

  if (rtn1 == sperr::RTNType::Good && rtn1 == rtn2) {
    for (size_t i = 0; i < input.size(); i++)
      EXPECT_EQ(input[i], output[i]);
  }
}

TEST(sperr_helper, domain_decomposition)
{
  using sperr::dims_type;
  using cdef = std::array<size_t, 6>;  // chunk definition

  auto vol = dims_type{4, 4, 4};
  auto subd = dims_type{1, 2, 3};

  auto chunks = sperr::chunk_volume(vol, subd);
  EXPECT_EQ(chunks.size(), 8);
  auto chunk = cdef{0, 1, 0, 2, 0, 4};
  EXPECT_EQ(chunks[0], chunk);
  chunk = cdef{1, 1, 0, 2, 0, 4};
  EXPECT_EQ(chunks[1], chunk);
  chunk = cdef{2, 1, 0, 2, 0, 4};
  EXPECT_EQ(chunks[2], chunk);
  chunk = cdef{3, 1, 0, 2, 0, 4};
  EXPECT_EQ(chunks[3], chunk);

  chunk = cdef{0, 1, 2, 2, 0, 4};
  EXPECT_EQ(chunks[4], chunk);
  chunk = cdef{1, 1, 2, 2, 0, 4};
  EXPECT_EQ(chunks[5], chunk);
  chunk = cdef{2, 1, 2, 2, 0, 4};
  EXPECT_EQ(chunks[6], chunk);
  chunk = cdef{3, 1, 2, 2, 0, 4};
  EXPECT_EQ(chunks[7], chunk);

  vol = dims_type{4, 4, 1};  // will essentially require a decomposition on a 2D plane.
  chunks = sperr::chunk_volume(vol, subd);
  EXPECT_EQ(chunks.size(), 8);
  chunk = cdef{0, 1, 0, 2, 0, 1};
  EXPECT_EQ(chunks[0], chunk);
  chunk = cdef{1, 1, 0, 2, 0, 1};
  EXPECT_EQ(chunks[1], chunk);
  chunk = cdef{2, 1, 0, 2, 0, 1};
  EXPECT_EQ(chunks[2], chunk);
  chunk = cdef{3, 1, 0, 2, 0, 1};
  EXPECT_EQ(chunks[3], chunk);

  chunk = cdef{0, 1, 2, 2, 0, 1};
  EXPECT_EQ(chunks[4], chunk);
  chunk = cdef{1, 1, 2, 2, 0, 1};
  EXPECT_EQ(chunks[5], chunk);
  chunk = cdef{2, 1, 2, 2, 0, 1};
  EXPECT_EQ(chunks[6], chunk);
  chunk = cdef{3, 1, 2, 2, 0, 1};
  EXPECT_EQ(chunks[7], chunk);
}

TEST(sperr_helper, read_sections)
{
  // Create an array, and write to disk.
  auto vec = std::vector<uint8_t>(256, 0);
  std::iota(vec.begin(), vec.end(), 0);
  sperr::write_n_bytes("test.tmp", 256, vec.data());
  auto buf = sperr::vec8_type();

  // Create a section that exceeds the file size.
  auto secs = std::vector<size_t>(2, 0);
  secs.insert(secs.end(), {200, 56});
  secs.insert(secs.end(), {101, 156});
  auto rtn = sperr::read_sections("test.tmp", secs, buf);
  EXPECT_EQ(rtn, sperr::RTNType::WrongLength);

  // Pop out the offending section requests.
  secs.pop_back();
  secs.pop_back();
  rtn = sperr::read_sections("test.tmp", secs, buf);
  EXPECT_EQ(rtn, sperr::RTNType::Good);

  // Add another section, and try reading them.
  secs.insert(secs.end(), {30, 5});
  buf.assign(10, 1);
  sperr::read_sections("test.tmp", secs, buf);
  EXPECT_EQ(buf.size(), 71);
  for (size_t i = 0; i < 10; i++) // First 10 elements should remain the same.
    EXPECT_EQ(buf[i], 1);
  for (size_t i = 0; i < 56; i++) // Next 56 elements should start from 200.
    EXPECT_EQ(buf[i + 10], i + 200);
  for (size_t i = 0; i < 5; i++)  // Next 5 elements should start from 30.
    EXPECT_EQ(buf[i + 66], i + 30);

  // Test the extract version too. Read and extract should give the same results.
  buf.clear();
  sperr::read_sections("test.tmp", secs, buf);
  auto full_input = sperr::read_whole_file<uint8_t>("test.tmp");
  auto buf2 = sperr::vec8_type();
  auto rtn2 = sperr::extract_sections(full_input.data(), full_input.size(), secs, buf2);
  EXPECT_EQ(rtn, sperr::RTNType::Good);
  EXPECT_EQ(buf, buf2);
}

}  // namespace
