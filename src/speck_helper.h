#ifndef SPECK_HELPER_H
#define SPECK_HELPER_H

//
// We put common functions that are used across the speck encoder here.
//

#include <array>
#include <cstddef> // size_t
#include <cstdlib>
#include <memory>
#include <vector>
#include <iterator>
#include "SpeckConfig.h"

#ifdef USE_PMR
    #include <memory_resource>
#endif

namespace speck {

    extern const uint8_t  u8_true;
    extern const uint8_t  u8_false;
    extern const uint8_t  u8_discard;

#ifndef BUFFER_TYPES
#define BUFFER_TYPES
    using buffer_type_d     = std::unique_ptr<double[]>;
    using buffer_type_f     = std::unique_ptr<float[]>;
    using buffer_type_c     = std::unique_ptr<char[]>;
    using buffer_type_b     = std::unique_ptr<bool[]>;
    using buffer_type_uint8 = std::unique_ptr<uint8_t[]>;

  #ifdef USE_PMR
    using vector_bool     = std::pmr::vector<bool>;
    using vector_size_t   = std::pmr::vector<size_t>;
    using vector_uint8_t  = std::pmr::vector<uint8_t>;
  #else
    using vector_bool     = std::vector<bool>;
    using vector_size_t   = std::vector<size_t>;
    using vector_uint8_t  = std::vector<uint8_t>;
  #endif

#endif

//
// Helper classes
//
enum class Significance : unsigned char {
    Insig,
    Sig,
    NewlySig
};

enum class SetType : unsigned char {
    TypeS,
    TypeI,
    Garbage
};

enum class RTNType {
    Good,
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
// Auxiliary class to hold a 1D SPECK Set
// TODO: evaluate if it makes sense to move these 1D objects to SEPCK_Err
//
class SPECKSet1D {
public:
    size_t        start      = 0;
    size_t        length     = 0;
    uint32_t      part_level = 0;
    Significance  signif     = Significance::Insig;
    SetType       type       = SetType::TypeS;    // only to indicate if it's garbage

    SPECKSet1D() = default;
    SPECKSet1D( size_t start, size_t len, uint32_t part_lev );
};

//
// Auxiliary struct to hold represent an outlier
//
struct Outlier {
    size_t location = 0;
    float  error    = 0.0f;

    Outlier() = default;
    Outlier( size_t, float );
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
    ptr_iterator(T* p) : m_pos(p) {}
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

// Helper function to produce a ptr_iterator from a raw pointer or a unique_ptr.
// For a raw array with size N, the begin and end iterators are:
// auto begin = ptr2itr( buf ); auto end = ptr2itr( buf + N );
template<typename T>
auto ptr2itr(T *val) -> ptr_iterator<T>;

template<typename T>
auto uptr2itr( const std::unique_ptr<T[]>&, size_t offset = 0 ) -> ptr_iterator<T>;


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
auto make_coeff_positive(U& buf, size_t len, vector_uint8_t&) -> typename U::element_type;

// Pack and unpack booleans to array of chars. 
// The caller should have allocated the right amount of memory for the `dest` array.
// When packing, the caller should also make sure the number of booleans is a multiplier of 8.
// It optionally takes in an offset that specifies where to start writing/reading the char array.
//
// Note: unpack_booleans() takes a raw pointer because it accesses memory provided by others,
//       and others most likely provide it by raw pointers.
auto pack_booleans( buffer_type_uint8&    dest,
                    const vector_uint8_t& src,
                    size_t                dest_offset = 0 ) -> RTNType;
auto unpack_booleans( vector_uint8_t&     dest,
                      const void*         src,
                      size_t              src_len,
                      size_t              src_offset = 0 ) -> RTNType;

// Pack and unpack exactly 8 booleans to/from a single byte
// Note that memory for the 8 booleans should already be allocated!
// Note on the choice of using bool* instead of std::array<bool, 8>: the former is less pixie dust
void pack_8_booleans(   uint8_t& dest,  const bool* src );
void unpack_8_booleans( bool* dest,     uint8_t src );

// Allocate an array of a certain type, and return as a unique pointer.
template <typename T>
auto unique_malloc( size_t size ) -> std::unique_ptr<T[]>;

};  // End of speck namespace.

#endif
