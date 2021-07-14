#ifndef CONDITIONER_H
#define CONDITIONER_H

//
// Applies conditioning operations to an array of data.
//

#include "speck_helper.h"

namespace speck {

class Conditioner {

public:

    // Booleans recording what operations are applied:
    // bool[0] : subtract mean
    // bool[1] : divide by rms
    // bool[2-7] : unused
    void toggle_all_settings( std::array<bool, 8> );

    auto get_meta_size() const -> size_t;

    // The 17 bytes returned by `condition()`: 1 byte (8 booleans) followed by two doubles.
    // The byte of booleans records what operations are applied:
    // Accordingly, `inverse_condition()` takes a buffer of 17 bytes.
    auto condition( speck::vecd_type& ) const -> std::pair<RTNType, std::array<uint8_t, 17>>;
    auto inverse_condition( speck::vecd_type&, const uint8_t* ) const -> RTNType;

private:
    //
    // what treatments are applied?
    //
    std::array<bool, 8> m_settings = {true,   // subtract mean
                                      false,  // divide by rms
                                      false,  // unused
                                      false,  // unused
                                      false,  // unused
                                      false,  // unused
                                      false,  // unused
                                      false}; // unused

    const size_t m_meta_size = 17;
    using meta_type = std::array<uint8_t, 17>;

    // Calculations are carried out by strides, which 
    // should be a divisor of the input data size.
    mutable size_t m_num_strides  = 2048; // should be good enough for most applications.
    mutable std::vector<double> m_stride_buf;

    // Action items
    // Buffers passed in here are guaranteed to have correct lengths and conditions.
    auto m_calc_mean( const speck::vecd_type& buf ) const -> double;
    auto m_calc_rms( const speck::vecd_type& buf ) const -> double;
    // adjust the value of `m_num_strides` so it'll be a divisor of `len`
    void m_adjust_strides( size_t len ) const;
};

};

#endif
