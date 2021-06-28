#ifndef CONDITIONER_H
#define CONDITIONER_H

//
// Applies conditioning operations to an array of data.
//

#include "speck_helper.h"

namespace speck {

class Conditioner {

public:
    Conditioner() = default;
    Conditioner(bool sub_mean); // if subtract mean or not

    void toggle_all_false();
    void toggle_subtract_mean( bool );
    void toggle_divide_by_rms( bool );
    auto get_meta_size() const -> size_t;

    // The 17 bytes returned by `condition()`: 1 byte (8 booleans) followed by two doubles.
    // The byte of booleans records if the following operation is applied:
    // bool[0] : subtract mean
    // bool[1] : divide by rms
    // bool[2-7] : unused
    // Accordingly, `inverse_condition()` takes a buffer of 17 bytes.
    template<typename T>
    auto condition( T&, size_t ) const -> std::pair<RTNType, std::array<uint8_t, 17>>;
    template<typename T>
    auto inverse_condition( T&, size_t, const uint8_t* ) const -> RTNType;
    template<typename T>
    auto inverse_condition( T&, size_t, const std::array<uint8_t, 17>& ) const -> RTNType;

private:
    //
    // what treatments are applied?
    //
    bool m_s_mean  = false; // subtract mean
    bool m_d_rms   = false; // divide by rms

    // Calculations are carried out by strides, which 
    // should be a divisor of the input data size.
    mutable size_t m_num_strides  = 2048; // should be good enough for most applications.
    mutable std::vector<double> m_stride_buf;
    const size_t m_meta_size = 17;

    // Action items
    // Buffers passed in here are guaranteed to have correct lengths and conditions.
    template<typename T>
    auto m_calc_mean( T& buf, size_t len ) const -> double;
    template<typename T>
    auto m_calc_rms(  T& buf, size_t len ) const -> double;
    // adjust the value of `m_num_strides` so it'll be a divisor of `len`
    void m_adjust_strides( size_t len ) const;
};

};

#endif
