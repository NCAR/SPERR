//
// This is a the class object that is supposed to be used by most users, because
// it provides easy-to-use APIs.
// Functionality wise, it does not bring anything new though.
// 

#ifndef SPECK3D_COMPRESSOR_H
#define SPECK3D_COMPRESSOR_H


#include "CDF97.h"
#include "SPECK3D.h"

class SPECK3D_Compressor {

public:
    // Constructor
    SPECK3D_Compressor( size_t x, size_t y, size_t z );

    // Accept incoming data: copy from a raw memory block
    template< typename T >
    auto copy_data( const T* p, size_t len ) -> int;

    // Accept incoming data by taking ownership of the memory block
    auto take_data( speck::buffer_type_d buf, size_t len ) -> int;

    // Accept incoming data by reading a file from disk.
    // The file is supposed to contain blocks of 32-bit floating values, with
    // X direction varying fastest, and Z direction varying slowest.
    auto read_floats( const char* filename ) -> int;

#ifdef QZ_TERM
    void set_qz_level( int );
#else
    auto set_bpp( float ) -> int;
#endif

    auto compress() -> int;
    // Provide a copy of the compressed buffer to the caller.
    // The caller will take over the ownership.
    auto get_compressed_buffer( speck::buffer_type_raw&, size_t& ) const -> int;
    auto write_bitstream( const char* filename ) const -> int;

private:
    const size_t m_dim_x, m_dim_y, m_dim_z, m_total_vals;
    speck::buffer_type_d  m_val_buf;

    speck::CDF97    m_cdf;
    speck::SPECK3D  m_encoder;

#ifdef QZ_TERM
    int qz_lev;
#else
    float m_bpp = 0.0;
#endif
};


#endif
