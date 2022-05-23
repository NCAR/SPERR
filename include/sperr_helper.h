#ifndef SPERR_HELPER_H
#define SPERR_HELPER_H

//
// We put common functions that are used across the project here.
//

#include <array>
#include <cstddef>  // std::size_t
#include <cstdint>  // fixed width integers
#include <cstdlib>
#include <iterator>
#include <memory>
#include <string>
#include <utility>  // std::pair
#include <vector>
#include "SperrConfig.h"

namespace sperr {

using std::size_t;  // Seems most appropriate

//
// A few shortcuts
//
using vecd_type = std::vector<double>;
using vec8_type = std::vector<uint8_t>;
using dims_type = std::array<size_t, 3>;

//
// Helper classes
//
enum class SigType : unsigned char { Insig, Sig, NewlySig, Dunno, Garbage };

enum class SetType : unsigned char { TypeS, TypeI, Garbage };

// Return Type
enum class RTNType {
  Good = 0,
  WrongDims,
  BitstreamWrongLen,
  IOError,
  InvalidParam,
  QzLevelTooBig,  // a very specific type of invalid param
  EmptyStream,    // a condition but not sure if it's an error
  BitBudgetMet,
  VersionMismatch,
  ZSTDMismatch,
  ZSTDError,
  SliceVolumeMismatch,
  QzModeMismatch,
  Error
};

//
// Iterator class that enables STL algorithms to operate on raw arrays.  Adapted from:
// https://stackoverflow.com/questions/8054273/how-to-implement-an-stl-style-iterator-and-avoid-common-pitfalls
//
template <typename T>
class ptr_iterator : public std::iterator<std::bidirectional_iterator_tag, T> {
 private:
  using iterator = ptr_iterator<T>;
  T* m_pos = nullptr;

 public:
  // explicit keyword prevents any other pointer types being converted to T*
  explicit ptr_iterator(T* p) : m_pos(p) {}
  ptr_iterator() = default;
  ptr_iterator(const ptr_iterator<T>&) = default;
  ptr_iterator(ptr_iterator<T>&&) noexcept = default;
  ptr_iterator& operator=(const ptr_iterator<T>&) = default;
  ptr_iterator& operator=(ptr_iterator<T>&&) noexcept = default;
  ~ptr_iterator() = default;

  // Requirements for a bidirectional iterator
  iterator operator++(int) { return ptr_iterator<T>(m_pos++); } /* postfix */
  iterator& operator++()                                        /* prefix  */
  {
    ++m_pos;
    return *this;
  }
  iterator operator--(int) { return ptr_iterator<T>(m_pos--); } /* postfix */
  iterator& operator--()                                        /* prefix  */
  {
    --m_pos;
    return *this;
  }
  T& operator*() const { return *m_pos; }
  T* operator->() const { return m_pos; }
  bool operator==(const iterator& rhs) const { return m_pos == rhs.m_pos; }
  bool operator!=(const iterator& rhs) const { return m_pos != rhs.m_pos; }
  iterator operator+(std::ptrdiff_t n) const { return ptr_iterator<T>(m_pos + n); }

  // The last operation is a random access iterator requirement.
  // To make it a complete random access iterator, more operations need to be
  // added. A good example is available here:
  // https://github.com/shaomeng/cppcon2019_class/blob/master/labs/01-vector_walkthrough/code/trnx_vector_impl.h
};

//
// Helper functions
//
// Given a certain length, how many transforms to be performed?
auto num_of_xforms(size_t len) -> size_t;

// How many partition operation could we perform given a length?
// Length 0 and 1 can do 0 partitions; len=2 can do 1; len=3 can do 2, len=4 can
// do 2, etc.
auto num_of_partitions(size_t len) -> size_t;

// Determine the approximation and detail signal length at a certain
// transformation level lev: 0 <= lev < num_of_xforms.
// It puts the approximation and detail length as the 1st and 2nd
// element of the return array.
auto calc_approx_detail_len(size_t orig_len, size_t lev) -> std::array<size_t, 2>;

// Pack and unpack booleans to array of chars.
// When packing, the caller should make sure the number of booleans is a
// multiplier of 8. It optionally takes in an offset that specifies where to
// start writing/reading the char array.
//
// Note 1: unpack_booleans() takes a raw pointer because it accesses memory
// provided by others, and others most likely provide it by raw pointers.
// Note 2: these two methods only work on little endian machines.
// Note 3: the caller should have already allocated enough space for `dest`.
auto pack_booleans(std::vector<uint8_t>& dest, const std::vector<bool>& src, size_t dest_offset = 0)
    -> RTNType;
auto unpack_booleans(std::vector<bool>& dest,
                     const void* src,
                     size_t src_len,
                     size_t src_offset = 0) -> RTNType;

// Pack and unpack exactly 8 booleans to/from a single byte
// Note: memory for the 8 booleans should already be allocated!
// Note: these two methods only work on little endian machines.
auto pack_8_booleans(std::array<bool, 8>) -> uint8_t;
auto unpack_8_booleans(uint8_t) -> std::array<bool, 8>;

// Read from and write to a file
// Note: not using references for `filename` to allow a c-style string literal to be passed in.
auto write_n_bytes(std::string filename, size_t n_bytes, const void* buffer) -> RTNType;
template <typename T>
auto read_whole_file(std::string filename) -> std::vector<T>;

// Upon success, it returns a vector of size `n_bytes`.
// Otherwise, it returns an empty vector.
auto read_n_bytes(std::string filename, size_t n_bytes) -> std::vector<uint8_t>;

// Calculate a suite of statistics.
// Note that arr1 is considered as the ground truth array, so it's the range of
// arr1 that is used internally for psnr calculations.
// The return array contains statistics in the following order:
// ret[0] : RMSE
// ret[1] : L-Infinity
// ret[2] : PSNR
// ret[3] : min of arr1
// ret[4] : max of arr1
template <typename T>
auto calc_stats(const T* arr1, const T* arr2, size_t arr_len, size_t omp_nthreads)
    -> std::array<T, 5>;

template <typename T>
auto kahan_summation(const T*, size_t) -> T;

// Given a whole volume size and a desired chunk size, this helper function
// returns a list of chunks specified by 6 integers:
// chunk[0], [2], [4]: starting index of this chunk in X, Y, and Z;
// chunk[1], [3], [5]: length of this chunk in X, Y, and Z.
// Note 1: the values in `chunk_dim` is only suggestive, meaning that when the volume
//         dimension is not exact multiplies of requested chunk dimension,
//         approximate values are used.
// Note 2: this function works on degraded 2D or 1D volumes too.
auto chunk_volume(const dims_type& vol_dim, const dims_type& chunk_dim)
    -> std::vector<std::array<size_t, 6>>;

// Gather a chunk from a bigger volume
// If the requested chunk lives outside of the volume, whole or part,
// this function returns an empty vector.
template <typename T1, typename T2>
auto gather_chunk(const T1* vol, dims_type vol_dim, const std::array<size_t, 6>& chunk)
    -> std::vector<T2>;

// Put this chunk to a bigger volume
// The `big_vol` should have enough space allocated, and the `small_vol` should contain
// enough elements to scatter. Memory errors will occur if the conditions are not met.
template <typename TBIG, typename TSML>
void scatter_chunk(std::vector<TBIG>& big_vol,
                   dims_type vol_dim,
                   const std::vector<TSML>& small_vol,
                   const std::array<size_t, 6>& chunk);

// Structure that holds information extracted from SPERR headers.
// This structure is returned by helper function `parse_header()`.
struct Header_Info {
  uint8_t version_major = 0;
  uint8_t version_minor = 0;
  bool zstd_applied = false;
  bool is_3d = false;

  // True <--> the bitstream is in fixed-error mode.
  // False <--> the bitstream is in fixed-size mode.
  bool is_qz_term = false;

  // This is the dimension of a 3D volume (NX, NY, NZ) or a 2D slice (NX, NY, 1).
  dims_type vol_dims = {0, 0, 0};

  // If the bitstream represents a 3D volume, this field holds the dimension of chunks.
  // For 2D slices, this field holds undefined values.
  dims_type chunk_dims = {0, 0, 0};
};
auto parse_header(const void*) -> Header_Info;

// For research use only:
// calculate the sum of squares of one vector.
auto calc_sq_sum(const vecd_type&) -> double;

};  // namespace sperr

#endif
