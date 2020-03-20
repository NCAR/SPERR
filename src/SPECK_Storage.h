#ifndef SPECK_STORAGE_H
#define SPECK_STORAGE_H

/* 
 * This class implements mechanisms and interfaces to help 2D and 3D SPECK classes 
 * manage storage of coefficients and bitstreams.
 */

#include <memory>
#include <vector>
#include "speck_helper.h"

namespace speck
{

class SPECK_Storage
{
public:
    // memory management: input
    void take_coeffs( std::unique_ptr<double[]> );  // Take ownership of a chunck of memory.
    template <typename T>
    void copy_coeffs( const T*, size_t len );       // Make a copy of the incoming data.
    void take_bitstream( std::vector<bool>& );      // Take ownership of the bitstream.
    void copy_bitstream( const std::vector<bool>& );// Make a copy of the bitstream.

    // memory management: output
    const std::vector<bool>&  get_read_only_bitstream() const;
    std::vector<bool>&        release_bitstream();          // The bitstream will be up to changes.
    const buffer_type&        get_read_only_coeffs() const; // Others can read the data
    std::unique_ptr<double[]> release_coeffs();             // Others take ownership of the data


protected:
    buffer_type  m_coeff_buf = nullptr;     // All coefficients are kept here
    std::vector<bool> m_bit_buffer;         // Output/Input bit buffer

};

};

#endif
