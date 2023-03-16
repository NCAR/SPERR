#include "gtest/gtest.h"

#include "Bitstream.h"
#include "Bitmask.h"

#include <random>
#include <vector>

namespace {

using Stream = sperr::Bitstream;
using Mask = sperr::Bitmask;

TEST(Bitstream, constructor)
{
  auto s1 = Stream();
  EXPECT_EQ(s1.capacity(), 64);

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

TEST(Bitstream, MemoryAllocation1)
{
  auto s1 = Stream(64);
  auto vec = std::vector<bool>();

  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<unsigned int> distrib(0, 1);

  s1.rewind();
  for (size_t i = 0; i < 64; i++) {
    auto val = distrib(gen);
    s1.write_bit(val);
    vec.push_back(val);
  }
  EXPECT_EQ(s1.capacity(), 64);

  // Write another bit and flush; the capacity is expected to grow to 128.
  s1.write_bit(1);
  vec.push_back(1);
  EXPECT_EQ(s1.wtell(), 65);
  EXPECT_EQ(s1.capacity(), 64);
  s1.flush();
  EXPECT_EQ(s1.capacity(), 128);

  // All saved bits should be correct too.
  s1.rewind();
  for (size_t i = 0; i < vec.size(); i++)
    EXPECT_EQ(s1.read_bit(), vec[i]) << "at idx = " << i;

  // Let's try to trigger another memory re-allocation
  s1.wseek(65);
  for (size_t i = 0; i < 64; i++) {
    auto val = distrib(gen);
    s1.write_bit(val);
    vec.push_back(val);
  }
  EXPECT_EQ(s1.wtell(), 129);
  EXPECT_EQ(s1.capacity(), 128);
  s1.flush();
  EXPECT_EQ(s1.capacity(), 256);
  s1.rewind();
  for (size_t i = 0; i < vec.size(); i++)
    EXPECT_EQ(s1.read_bit(), vec[i]) << "at idx = " << i;
}

TEST(Bitstream, StreamWriteRead)
{
  const size_t N = 150;
  auto s1 = Stream();
  auto vec = std::vector<bool>(N);

  // Make N writes
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<unsigned int> distrib1(0, 1);
  for (size_t i = 0; i < N; i++) {
    const bool bit = distrib1(gen);
    vec[i] = bit;
    s1.write_bit(bit);
  }
  EXPECT_EQ(s1.wtell(), 150);
  s1.flush();
  EXPECT_EQ(s1.wtell(), 192);

  s1.rewind();
  for (size_t i = 0; i < N; i++)
    EXPECT_EQ(s1.read_bit(), vec[i]) << " at idx = " << i;
}

TEST(Bitstream, RandomWriteRead)
{
  const size_t N = 256;
  auto s1 = Stream(59);
  auto vec = std::vector<bool>(N);

  // Make N stream writes
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<unsigned int> distrib1(0, 1);
  for (size_t i = 0; i < N; i++) {
    const bool bit = distrib1(gen);
    vec[i] = bit;
    s1.write_bit(bit);
  }
  EXPECT_EQ(s1.wtell(), 256);
  s1.flush();
  EXPECT_EQ(s1.wtell(), 256);

  // Make random writes on word boundaries
  s1.wseek(63);
  s1.write_bit(true);   vec[63] = true;
  s1.wseek(127);
  s1.write_bit(false);  vec[127] = false;
  s1.wseek(191);
  s1.write_bit(true);   vec[191] = true;
  s1.wseek(255);
  s1.write_bit(false);  vec[255] = false;
  s1.rewind();
  for (size_t i = 0; i < N; i++)
    EXPECT_EQ(s1.read_bit(), vec[i]) << " at idx = " << i;
  
  // Make random reads
  std::uniform_int_distribution<unsigned int> distrib2(0, N - 1);
  for (size_t i = 0; i < 100 ; i++) {
    const auto pos = distrib2(gen);
    s1.rseek(pos);
    EXPECT_EQ(s1.read_bit(), vec[pos]) << " at idx = " << i;
  }
}

TEST(Bitstream, CompactStream)
{
  // Test full 64-bit multiples
  size_t N = 128;
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<unsigned int> distrib1(0, 1);
  auto s1 = Stream();

  for (size_t i = 0; i < N; i++)
    s1.write_bit(distrib1(gen));
  s1.flush();

  auto buf = s1.get_bitstream(N);
  auto s2 = Stream();
  s2.parse_bitstream(buf.data(), 128);
  for (size_t i = 0; i < N; i++)
    EXPECT_EQ(s1.read_bit(), s2.read_bit());

  // Test full 64-bit multiples and 8-bit multiples
  buf = s1.get_bitstream(80);
  s2.parse_bitstream(buf.data(), 80);
  for (size_t i = 0; i < 80; i++)
    EXPECT_EQ(s1.read_bit(), s2.read_bit());

  // Test full 64-bit multiples, 8-bit multiples, and remaining bits
  buf = s1.get_bitstream(85);
  s2.parse_bitstream(buf.data(), 85);
  for (size_t i = 0; i < 85; i++)
    EXPECT_EQ(s1.read_bit(), s2.read_bit());

  // Test less than 64 bits
  buf = s1.get_bitstream(45);
  s2.parse_bitstream(buf.data(), 45);
  for (size_t i = 0; i < 45; i++)
    EXPECT_EQ(s1.read_bit(), s2.read_bit());

  // Test less than 8 bits
  buf = s1.get_bitstream(5);
  s2.parse_bitstream(buf.data(), 5);
  for (size_t i = 0; i < 5; i++)
    EXPECT_EQ(s1.read_bit(), s2.read_bit());
}

TEST(Bitmask, RandomReadWrite)
{
  const size_t N = 192;
  auto m1 = Mask(N);
  EXPECT_EQ(m1.size(), N);
  m1.write_long(0, 928798ul);
  m1.write_long(64, 9845932ul);
  m1.write_long(128, 77719821ul);
  EXPECT_EQ(m1.read_long(1), 928798ul);
  EXPECT_EQ(m1.read_long(65), 9845932ul);
  EXPECT_EQ(m1.read_long(129), 77719821ul);

  auto vec = std::vector<bool>();
  for (size_t i = 0; i < N; i++)
    vec.push_back(m1.read_bit(i));

  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<unsigned int> distrib(0, 1);
  for (size_t i = 30; i < N - 20; i++) {
    bool ran = distrib(gen);
    m1.write_bit(i, ran);
    vec[i] = ran;
  }
  for (size_t i = 0; i < N; i++)
    EXPECT_EQ(m1.read_bit(i), vec[i]) << "at idx = " << i;
}

}  // namespace
