#ifndef SPECK_HELPER_H
#define SPECK_HELPER_H

//
// We put common functions that are used across the speck encoder here.
//

#include <array>
#include <cstddef> // size_t
#include <memory>
#include <vector>
#include "SpeckConfig.h"

#ifdef USE_PMR
    #include <memory_resource>
#endif

namespace speck {

#ifndef BUFFER_TYPES
#define BUFFER_TYPES

    using buffer_type_d   = std::unique_ptr<double[]>;
    using buffer_type_f   = std::unique_ptr<float[]>;
    using buffer_type_c   = std::unique_ptr<char[]>;
    using buffer_type_raw = std::unique_ptr<uint8_t[]>; // for blocks of raw memory

#ifdef USE_PMR
    using vector_bool   = std::pmr::vector<bool>;
    using vector_size_t = std::pmr::vector<size_t>;
#else
    using vector_bool   = std::vector<bool>;
    using vector_size_t = std::vector<size_t>;
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
auto make_coeff_positive(U& buf, size_t len, vector_bool&) -> typename U::element_type;

// Pack and unpack booleans to array of chars. 
// The caller should have allocated the right amount of memory for the `dest` array.
// When packing, the caller should also make sure the number of booleans is a multiplier of 8.
// It optionally takes in an offset that specifies where to start writing/reading the char array.
//
// Note: unpack_booleans() takes a raw pointer because it accesses memory provided by others,
//       and others most likely provide it by raw pointers.
auto pack_booleans( buffer_type_raw&   dest,
                    const vector_bool& src,
                    size_t             char_offset = 0 ) -> RTNType;
auto unpack_booleans( vector_bool&     dest,
                      const void*      src,
                      size_t           src_len, // total length of char array
                      size_t           char_offset = 0 ) -> RTNType;

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
