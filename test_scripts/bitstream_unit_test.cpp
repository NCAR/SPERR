#include "gtest/gtest.h"

#include "ZFP_bitstream.h"

#include <random>
#include <vector>

namespace {

using Stream = sperr::ZFP_bitstream;

TEST(ZFP_bitstream, constructor)
{
  auto s1 = Stream();
  EXPECT_EQ(s1.capacity(), 0);

  auto s2 = Stream(1024 + 1);
  EXPECT_EQ(s2.capacity(), 1024 + 64);

  auto s3 = Stream(1024 + 63);
  EXPECT_EQ(s3.capacity(), 1024 + 64);

  auto s4 = Stream(1024 + 64);
  EXPECT_EQ(s4.capacity(), 1024 + 64);

  auto s5 = Stream(1024 + 65);
  EXPECT_EQ(s5.capacity(), 1024 + 64 * 2);

  auto s6 = Stream(1024 - 1);
  EXPECT_EQ(s6.capacity(), 1024);

  auto s7 = Stream(1024 - 63);
  EXPECT_EQ(s7.capacity(), 1024);

  auto s8 = Stream(1024 - 64);
  EXPECT_EQ(s8.capacity(), 1024 - 64);

  auto s9 = Stream(1024 - 65);
  EXPECT_EQ(s9.capacity(), 1024 - 64);
}

TEST(ZFP_bitstream, MemoryAllocation1)
{
  auto s1 = Stream(64);
  auto vec = std::vector<bool>();

  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<unsigned int> distrib(0, 1);

  s1.rewind();
  for (size_t i = 0; i < 64; i++) {
    auto val = distrib(gen);
    s1.stream_write_bit(val);
    vec.push_back(val);
  }
  EXPECT_EQ(s1.capacity(), 64);

  // Write another bit; the capacity is expected to grow to 128.
  s1.stream_write_bit(1);
  vec.push_back(1);
  EXPECT_EQ(s1.capacity(), 128);

  // All saved bits should be correct too.
  EXPECT_EQ(s1.wtell(), 65);
  s1.flush();
  s1.rewind();
  for (size_t i = 0; i < vec.size(); i++)
    EXPECT_EQ(s1.stream_read_bit(), vec[i]) << "at idx = " << i;

  // Let's try to trigger another memory re-allocation
  s1.wseek(65);
  for (size_t i = 0; i < 64; i++) {
    auto val = distrib(gen);
    s1.stream_write_bit(val);
    vec.push_back(val);
  }
  EXPECT_EQ(s1.capacity(), 256);
  EXPECT_EQ(s1.wtell(), 129);
  s1.flush();
  s1.rewind();
  for (size_t i = 0; i < vec.size(); i++)
    EXPECT_EQ(s1.stream_read_bit(), vec[i]) << "at idx = " << i;
}

TEST(ZFP_bitstream, MemoryAllocation2)
{
  auto s1 = Stream(130);
  EXPECT_EQ(s1.capacity(), 192);

  s1.stream_write_n_bits(928798ul, 64);
  s1.stream_write_n_bits(9845932ul, 64);
  s1.stream_write_n_bits(19821ul, 63);
  EXPECT_EQ(s1.wtell(), 191);
  EXPECT_EQ(s1.capacity(), 192);

  s1.stream_write_n_bits(8219821ul, 63);
  EXPECT_TRUE(s1.capacity() == 256 || s1.capacity() == 384);

  // Let's try one more time!
  s1.flush();
  s1.rewind();
  auto vec = std::vector<bool>();
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<unsigned int> distrib(0, 1);
  for (size_t itr = 0; itr < 4; itr++) {
    uint64_t value = 0ul;
    for (size_t i = 0; i < 64; i++) {
      uint64_t ran = distrib(gen);
      value += ran << i;
      vec.push_back(ran);
    }
    s1.stream_write_n_bits(value, 64);
  }
  EXPECT_EQ(s1.wtell(), vec.size());
  EXPECT_TRUE(s1.capacity() == 256 || s1.capacity() == 384);
  s1.flush();
  s1.rewind();
  for (size_t i = 0; i < vec.size(); i++)
    EXPECT_EQ(s1.stream_read_bit(), vec[i]) << "at idx = " << i;

  s1.wseek(vec.size());
  for (size_t itr = 0; itr < 3; itr++) {
    uint64_t value = 0ul;
    for (size_t i = 0; i < 63; i++) {
      uint64_t ran = distrib(gen);
      value += ran << i;
      vec.push_back(ran);
    }
    s1.stream_write_n_bits(value, 63);
  }
  EXPECT_EQ(s1.wtell(), vec.size());
  EXPECT_TRUE(s1.capacity() == 512 || s1.capacity() == 768);
  s1.flush();
  s1.rewind();
  for (size_t i = 0; i < vec.size(); i++)
    EXPECT_EQ(s1.stream_read_bit(), vec[i]) << "at idx = " << i;
}

TEST(ZFP_bitstream, TestRange)
{
  auto s1 = Stream(19);

  // Test buffered word
  s1.stream_write_n_bits(0ul, 63);
  s1.stream_write_bit(true);
  s1.flush();
  EXPECT_EQ(s1.test_range(2, 45), false);
  EXPECT_EQ(s1.test_range(2, 62), true);

  // Test whole integers
  s1.rewind();
  for (size_t i = 0; i < 3; i++)
    s1.stream_write_n_bits(0ul, 63);
  s1.stream_write_bit(true);
  s1.stream_write_bit(false);
  s1.stream_write_bit(false);
  EXPECT_EQ(s1.wtell(), 192);
  s1.flush();
  EXPECT_EQ(s1.test_range(0, 63), false);
  EXPECT_EQ(s1.test_range(63, 63), false);
  EXPECT_EQ(s1.test_range(126, 63), false);
  EXPECT_EQ(s1.test_range(0, 64), false);
  EXPECT_EQ(s1.test_range(64, 64), false);
  EXPECT_EQ(s1.test_range(128, 61), false);
  EXPECT_EQ(s1.test_range(64, 125), false);
  EXPECT_EQ(s1.test_range(64, 126), true);
  EXPECT_EQ(s1.test_range(63, 127), true);
  EXPECT_EQ(s1.test_range(0, 190), true);

  // Test remaining bits
  s1.rewind();
  for (size_t i = 0; i < 3; i++)
    s1.stream_write_n_bits(0ul, 64);
  s1.stream_write_bit(true);
  s1.flush();
  EXPECT_EQ(s1.test_range(0, 192), false);
  EXPECT_EQ(s1.test_range(1, 192), true);
  EXPECT_EQ(s1.test_range(2, 191), true);
  EXPECT_EQ(s1.test_range(3, 190), true);
  EXPECT_EQ(s1.test_range(3, 189), false);
}

TEST(ZFP_bitstream, TestRandomWrite)
{
  auto s1 = Stream(256);
  s1.stream_write_n_bits(192878ul, 64);
  s1.stream_write_n_bits(598932ul, 64);
  s1.stream_write_n_bits(792878ul, 64);
  s1.stream_write_n_bits(594932ul, 64);
  s1.flush();

  // Make a copy to a bit vector
  auto vec = std::vector<bool>(256);
  s1.rewind();
  for (size_t i = 0; i < vec.size(); i++)
    vec[i] = s1.stream_read_bit();

  // Make many random writes
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<unsigned int> distrib1(0, 1);
  std::uniform_int_distribution<unsigned int> distrib2(0, 255);
  for (size_t i = 0; i < 50; i++) {
    const auto bit = distrib1(gen);
    const auto pos = distrib2(gen);
    s1.random_write_bit(bit, pos);
    s1.rseek(pos);
    EXPECT_EQ(s1.stream_read_bit(), bit);
    vec[pos] = bool(bit);
  }

  s1.rewind();
  for (size_t i = 0; i < vec.size(); i++)
    EXPECT_EQ(s1.stream_read_bit(), vec[i]);
}

TEST(ZFP_bitstream, TestRandomRead)
{
  auto s1 = Stream(256);
  s1.stream_write_n_bits(5192878ul, 64);
  s1.stream_write_n_bits(1598932ul, 64);
  s1.stream_write_n_bits(8792878ul, 64);
  s1.stream_write_n_bits(3594932ul, 64);
  s1.flush();

  // Make many random reads 
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<unsigned int> distrib2(0, 255);
  for (size_t i = 0; i < 50; i++) {
    const auto pos = distrib2(gen);
    s1.rseek(pos);
    EXPECT_EQ(s1.random_read_bit(pos), s1.stream_read_bit());
  }
}

TEST(ZFP_bitstream, CompactStream)
{
  // Test full 64-bit multiples
  auto s1 = Stream(128);
  s1.stream_write_n_bits(35192878ul, 64);
  s1.stream_write_n_bits(85192878ul, 64);
  s1.flush();

  auto buf = s1.get_bitstream(128);
  auto s2 = Stream();
  s2.parse_bitstream(buf.data(), 128);

  s1.rewind();
  for (size_t i = 0; i < 128; i++)
    EXPECT_EQ(s1.stream_read_bit(), s2.stream_read_bit());

  // Test full 64-bit multiples and 8-bit multiples
  buf = s1.get_bitstream(80);
  s2.parse_bitstream(buf.data(), 80);
  s1.rewind();
  for (size_t i = 0; i < 80; i++)
    EXPECT_EQ(s1.stream_read_bit(), s2.stream_read_bit());

  // Test full 64-bit multiples, 8-bit multiples, and remaining bits
  buf = s1.get_bitstream(85);
  s2.parse_bitstream(buf.data(), 85);
  s1.rewind();
  for (size_t i = 0; i < 85; i++)
    EXPECT_EQ(s1.stream_read_bit(), s2.stream_read_bit());
}

}  // namespace
