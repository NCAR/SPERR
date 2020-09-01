//
// This is a the class object that is supposed to be used by most users, because
// it provides easy-to-use APIs.
// Functionality wise, it does not bring anything new though.
// 

#ifndef SPECK2D_DECOMPRESSOR_H
#define SPECK2D_DECOMPRESSOR_H


#include "CDF97.h"
#include "SPECK2D.h"

class SPECK2D_Decompressor {

public:
    // Accept incoming data: copy from a raw memory block
    void copy_bitstream( const void* p, size_t len );

    // Accept incoming data by taking ownership of the memory block
    void take_bitstream( speck::buffer_type_raw buf, size_t len );

    // Accept incoming data by reading a file from disk.
    auto read_bitstream( const char* filename ) -> int;

    auto set_bpp( float ) -> int;

    auto decompress() -> int;

    // Get the decompressed slice in a float or double buffer.
    // The caller doesn't need to allocate any memory for `buf`.
    // After calling this method, the caller will hold ownership of the memory.
    // `buf_size` will be the number of elements in buf.
    auto get_decompressed_slice_f( speck::buffer_type_f& buf, size_t& buf_size ) const -> int;
    auto get_decompressed_slice_d( speck::buffer_type_d& buf, size_t& buf_size ) const -> int;

    // Write the decompressed slice as floats or doubles to a file on disk.
    auto write_slice_f( const char* filename ) const -> int;
    auto write_slice_d( const char* filename ) const -> int;

    void get_slice_dims( size_t&, size_t& ) const;

private:
    size_t m_dim_x           = 0;
    size_t m_dim_y           = 0;
    size_t m_total_vals      = 0;
    size_t m_stream_buf_size = 0;
    speck::buffer_type_raw  m_stream_buf;

    speck::CDF97    m_cdf;
    speck::SPECK2D  m_decoder;

    float m_bpp = 0.0;
};


#endif