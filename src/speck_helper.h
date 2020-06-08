#ifndef SPECK_HELPER_H
#define SPECK_HELPER_H

/* We put common functions that are used across the speck encoder here. */

#include <array>
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

//
// Helper functions
//
// Given a certain length, how many transforms to be performed?
size_t calc_num_of_xforms(size_t len);

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
typename U::element_type make_coeff_positive(U& buf, size_t len, std::vector<bool>&);

};

#endif
