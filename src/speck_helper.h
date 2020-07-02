#ifndef SPECK_HELPER_H
#define SPECK_HELPER_H

//
// We put common functions that are used across the speck encoder here.
//

#include <array>
#include <cstddef> // size_t
#include <memory>
#include <vector>

namespace speck {

#ifndef BUFFER_TYPES
#define BUFFER_TYPES
using buffer_type_d = std::unique_ptr<double[]>;
using buffer_type_f = std::unique_ptr<float[]>;
using buffer_type_c = std::unique_ptr<char[]>;
#endif

//
// Helper classes
//
enum class Significance : unsigned char {
    Insig,
    Sig,
    NewlySig,
    Empty
};

enum class SetType : unsigned char {
    TypeS,
    TypeI,
    Garbage
};

class Outlier {
public:
    //
    // Member data
    //
    uint32_t x   = 0;
    uint32_t y   = 0;
    uint32_t z   = 0;
    float    err = 0.0;

    // Constructors
    Outlier() = default;
    Outlier(uint32_t x, uint32_t y, uint32_t z, float err);
};

//
// Auxiliary class to hold a 3D SPECK Set
//
class SPECKSet3D {
public:
    //
    // Member data
    //
    uint32_t start_x  = 0;
    uint32_t start_y  = 0;
    uint32_t start_z  = 0;
    uint32_t length_x = 0;
    uint32_t length_y = 0;
    uint32_t length_z = 0;
    // which partition level is this set at (starting from zero, in all 3 directions).
    // This data member is the sum of all 3 partition levels.
    uint16_t     part_level = 0;
    Significance signif     = Significance::Insig;
    SetType      type       = SetType::TypeS; // This field is only used to indicate garbage status

public:
    //
    // Member functions
    //
    auto is_pixel() const -> bool;
    auto is_empty() const -> bool;
#ifdef PRINT
    void print() const;
#endif
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
auto make_coeff_positive(U& buf, size_t len, std::vector<bool>&) -> typename U::element_type;

// Divide a SPECKSet3D into 8, 4, or 2 smaller subsets.
void partition_S_XYZ(const SPECKSet3D& set, std::array<SPECKSet3D, 8>& subsets);
void partition_S_XY(const SPECKSet3D& set, std::array<SPECKSet3D, 4>& subsets);
void partition_S_Z(const SPECKSet3D& set, std::array<SPECKSet3D, 2>& subsets);

// Pack and unpack booleans to array of chars. 
// The caller should have allocated sufficient amount of memory for the char array.
// When packing, the caller should also make sure the number of booleans is a multiplier of 8.
// It optionally takes in an offset that specifies where to start writing/reading the char array.
auto pack_booleans( buffer_type_c&           dest,
                    const std::vector<bool>& src,
                    size_t char_offset = 0 ) -> int;
auto unpack_booleans( std::vector<bool>&    dest,
                      const buffer_type_c&  src,
                      size_t                src_len, // total length of char array
                      size_t char_offset = 0 ) -> int;
};

#endif
