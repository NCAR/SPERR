#ifndef SPECK_HELPER_H
#define SPECK_HELPER_H

//
// We put common functions that are used across the speck encoder here.
//

#include <array>
#include <cstddef> // size_t
#include <cstdlib>
#include <utility> // std::pair
#include <memory>
#include <vector>
#include <iterator>
#include "SpeckConfig.h"

namespace speck {

//
// A few shortcuts
//
using buffer_type_d     = std::unique_ptr<double[]>;
using buffer_type_f     = std::unique_ptr<float[]>;
using buffer_type_uint8 = std::unique_ptr<uint8_t[]>;
using smart_buffer_d    = std::pair<buffer_type_d, size_t>; // It's smart because
using smart_buffer_f    = std::pair<buffer_type_f, size_t>; // it knows its size.
using smart_buffer_uint8= std::pair<buffer_type_uint8, size_t>;

//
// Helper classes
//
enum class SigType : unsigned char {
    Insig,
    Sig,
    NewlySig,
    Dunno,
    Garbage
};

enum class SetType : unsigned char {
    TypeS,
    TypeI,
    Garbage
};

enum class RTNType { // Return Type
    Good,            // Initial value
    WrongSize,
    IOError,
    InvalidParam,
    BitBudgetMet,
    VersionMismatch,
    ZSTDMismatch,
    ZSTDError,
    DimMismatch,
    Error
};


//
// Iterator class that enables STL algorithms to operate on raw arrays.  Adapted from: 
// https://stackoverflow.com/questions/8054273/how-to-implement-an-stl-style-iterator-and-avoid-common-pitfalls
//
template<typename T>
class ptr_iterator : public std::iterator<std::bidirectional_iterator_tag, T>
{
private:
    using iterator = ptr_iterator<T>;
    T* m_pos       = nullptr;

public:
    explicit ptr_iterator(T* p) : m_pos(p) {}
    ptr_iterator()                                   = default;
    ptr_iterator           (const ptr_iterator<T>& ) = default;
    ptr_iterator           (      ptr_iterator<T>&&) = default;
    ptr_iterator& operator=(const ptr_iterator<T>& ) = default;
    ptr_iterator& operator=(      ptr_iterator<T>&&) = default;
    ~ptr_iterator()                                  = default;

    // Requirements for a bidirectional iterator
    iterator  operator++(int) /* postfix */         { return ptr_iterator<T>(m_pos++); }
    iterator& operator++()    /* prefix  */         { ++m_pos; return *this; }
    iterator  operator--(int) /* postfix */         { return ptr_iterator<T>(m_pos--); }
    iterator& operator--()    /* prefix  */         { --m_pos; return *this; }
    T&        operator* () const                    { return *m_pos; }
    T*        operator->() const                    { return m_pos; }
    bool      operator==(const iterator& rhs) const { return m_pos == rhs.m_pos; }
    bool      operator!=(const iterator& rhs) const { return m_pos != rhs.m_pos; }
    iterator  operator+ (std::ptrdiff_t  n  ) const { return ptr_iterator<T>(m_pos+n); }

    // The last operation is a random access iterator requirement.
    // To make it a complete random access iterator, more operations need to be added.
    // A good example is available here: 
    // https://github.com/shaomeng/cppcon2019_class/blob/master/labs/01-vector_walkthrough/code/trnx_vector_impl.h
};

// Helper functions to generate a ptr_iterator from a unique_ptr.
// (Their names resemble std::begin() and std::end().)
// For an array with size N, the begin and end iterators are:
// auto begin = speck::begin( buf ); auto end = speck::begin( buf ) + N;
template<typename T>
auto begin( const std::unique_ptr<T[]>& ) -> ptr_iterator<T>;
// Generate a ptr_iterator from a smart_buffer.
template<typename T>
auto begin( const std::pair<std::unique_ptr<T[]>, size_t>& ) -> ptr_iterator<T>;
template<typename T>
auto end(   const std::pair<std::unique_ptr<T[]>, size_t>& ) -> ptr_iterator<T>;


//
// Helper functions
//
// Given a certain length, how many transforms to be performed?
auto num_of_xforms(size_t len) -> size_t;
    
// How many partition operation could we perform given a length?
// Length 0 and 1 can do 0 partitions; len=2 can do 1; len=3 can do 2, len=4 can do 2, etc.
auto num_of_partitions(size_t len) -> size_t;

// Determine the approximation and detail signal length at a certain
// transformation level lev: 0 <= lev < num_of_xforms.
// It puts the approximation and detail length as the 1st and 2nd
// element of the output array.
void calc_approx_detail_len(size_t orig_len, size_t lev, // input
                            std::array<size_t, 2>&);     // output

// 1) fill sign_array based on coeff_buffer signs, and
// 2) make coeff_buffer containing all positive values.
// 3) returns the maximum magnitude of all encountered values.
template <typename U>
auto make_coeff_positive(U& buf, size_t len, std::vector<bool>&) -> typename U::element_type;

// Pack and unpack booleans to array of chars. 
// When packing, the caller should make sure the number of booleans is a multiplier of 8.
// It optionally takes in an offset that specifies where to start writing/reading the char array.
//
// Note: unpack_booleans() takes a raw pointer because it accesses memory provided by others,
//       and others most likely provide it by raw pointers.
auto pack_booleans( buffer_type_uint8& dest, // !!This space should be allocated by the caller!!
                    const std::vector<bool>&  src,
                    size_t                    dest_offset = 0 ) -> RTNType;
auto unpack_booleans( std::vector<bool>&      dest,
                      const void*             src,
                      size_t                  src_len,
                      size_t                  src_offset = 0 ) -> RTNType;

// Pack and unpack exactly 8 booleans to/from a single byte
// Note that memory for the 8 booleans should already be allocated!
// Note on the choice of using bool* instead of std::array<bool, 8>: the former is less pixie dust
void pack_8_booleans(   uint8_t& dest,  const bool* src );
void unpack_8_booleans( bool* dest,     uint8_t src );

// Read from and write to a file
auto write_n_bytes(  const char* filename, size_t n_bytes, const void* buffer ) -> RTNType;
auto read_n_bytes(   const char* filename, size_t n_bytes,       void* buffer ) -> RTNType;
template <typename T>
auto read_whole_file(const char* filename) -> std::pair<std::unique_ptr<T[]>, size_t>;

// Calculate a suite of statistics
// Note that arr1 is considered as the ground truth array, so it's the range of arr1
// that is used internally for psnr calculations.
template <typename T>
void calc_stats( const T* arr1,   const T* arr2,  size_t len,
                 T* rmse, T* linfty, T* psnr, T* arr1min, T* arr1max );
template <typename T>
auto kahan_summation( const T*, size_t ) -> T;

// Test if a smart_buffer is empty. Might support other types of buffers in the future.
template <typename T>
auto empty_buf( const std::pair<T, size_t>& smart_buf) -> bool;

// Test if a smart_buffer is non-empty, AND correct in size.
template <typename T>
auto size_is( const std::pair<T, size_t>& smart_buf, size_t expected_size ) -> bool;


// Given a whole volume size and a desired chunk size, this helper function returns
// a list of chunks specified by 6 integers:
// chunk[0], [2], [4]: starting index of this chunk 
// chunk[1], [3], [5]: length of this chunk
auto chunk_volume( const std::array<size_t, 3>& vol_dim, 
                   const std::array<size_t, 3>& chunk_dim )
                   -> std::vector< std::array<size_t, 6> >;

// Gather a chunk from a bigger volume
template<typename T>
auto gather_chunk( const T* vol, const std::array<size_t, 3>& vol_dim, 
                   const std::array<size_t, 6>& chunk ) -> buffer_type_d;

// Put this chunk to a bigger volume
template<typename T>
void scatter_chunk( T* big_vol,  const std::array<size_t, 3>& vol_dim,
                    const buffer_type_d&         small_vol,
                    const std::array<size_t, 6>& chunk);

};  // End of speck namespace.

#endif
