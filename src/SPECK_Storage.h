#ifndef SPECK_STORAGE_H
#define SPECK_STORAGE_H

/* 
 * This class implements mechanisms and interfaces to help 2D and 3D SPECK classes 
 * manage storage of coefficients and bitstreams.
 */

#include <vector>
#include "speck_helper.h"

namespace speck
{

#define SPECK_USE_DOUBLE

class SPECK_Storage
{
public:
    // memory management: input
    void take_coeffs( buffer_type_d, size_t );      // Take ownership of the incoming buffer 
    void take_coeffs( buffer_type_f, size_t );      // Take ownership of the incoming buffer 
    template <typename T>
    void copy_coeffs( const T, size_t len );        // Make a copy of the incoming data.
    void take_bitstream( std::vector<bool>& );      // Take ownership of the bitstream.
    void copy_bitstream( const std::vector<bool>& );  // Make a copy of the bitstream.

    // memory management: output
    const std::vector<bool>& get_read_only_bitstream() const;
    std::vector<bool>&       release_bitstream();       // The bitstream will be up to changes.
    buffer_type_d            release_coeffs_double();   // Others take ownership of the data
    buffer_type_f            release_coeffs_float() ;   // Others take ownership of the data


protected:

#ifdef SPECK_USE_DOUBLE
    buffer_type_d  m_coeff_buf = nullptr;   // All coefficients are kept here
#else
    buffer_type_f  m_coeff_buf = nullptr;   // All coefficients are kept here
#endif

    size_t         m_coeff_len = 0;
    std::vector<bool> m_bit_buffer;         // Output/Input bit buffer

};

};

#endif
