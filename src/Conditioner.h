#ifndef CONDITIONER_H
#define CONDITIONER_H

//
// Applies conditioning operations to an array of data.
//

#include "speck_helper.h"

namespace speck {

class Conditioner {

public:
    void toggle_subtract_mean( bool );
    void toggle_divide_by_rms( bool );
    void set_num_strides( size_t ); // must be a divisor of the buffer length

    // The 17 bytes returned by `condition()`: 1 byte (8 booleans) followed by two doubles.
    // The byte of booleans records if the following operation is applied:
    // bool[0] : subtract mean
    // bool[1] : divide by rms
    // bool[2-7] : unused
    // Accordingly, `inverse_condition()` takes a buffer of 17 bytes.
    auto condition( buffer_type_d&, size_t ) const 
                    -> std::pair<RTNType, std::array<uint8_t, 17>>;
    auto inverse_condition( buffer_type_d&, size_t, const uint8_t* ) const -> RTNType;


private:
    //
    // what treatments are applied?
    //
    bool m_s_mean  = false; // subtract mean
    bool m_d_rms   = false; // divide by rms
    //bool m_n_range = false; // normalize by range, so all values are in (-1.0, 1.0)

    // Calculations are carried out by strides, so the number of strides
    // is something configurable.
    // The number of strides should be a divisor of the input data size though.
    size_t m_num_strides  = 2048; // should be good enough for most applications.
    mutable std::vector<double> m_stride_buf;
    const size_t m_meta_size = 17;

    // Action items
    // Buffers passed in here are guaranteed to have correct lengths and conditions.
    auto m_calc_mean( buffer_type_d& buf, size_t len ) const -> double;
    auto m_calc_rms(  buffer_type_d& buf, size_t len ) const -> double;

};

};

#endif
