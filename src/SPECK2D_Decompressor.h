//
// This is a the class object that is supposed to be used by most users, because
// it provides easy-to-use APIs.
// Functionality wise, it does not bring anything new though.
// 

#ifndef SPECK2D_DECOMPRESSOR_H
#define SPECK2D_DECOMPRESSOR_H


#include "CDF97.h"
#include "SPECK2D.h"

using speck::RTNType;

class SPECK2D_Decompressor {

public:
    // Accept incoming data: copy from a raw memory block
    void copy_bitstream( const void* p, size_t len );

    // Accept incoming data by taking ownership of the memory block
    void take_bitstream( speck::buffer_type_uint8 buf, size_t len );

    // Accept incoming data by reading a file from disk.
    auto read_bitstream( const char* filename ) -> RTNType;

    auto set_bpp( float ) -> RTNType;

    auto decompress() -> RTNType;

    // Get the decompressed slice in a float or double buffer.
    // The returned buffer is a copy of the slice, so caller can use
    // that buffer withint making another copy.
    auto get_decompressed_slice_f() const -> std::pair<speck::buffer_type_f, size_t>;
    auto get_decompressed_slice_d() const -> std::pair<speck::buffer_type_d, size_t>;

    // Write the decompressed slice as floats or doubles to a file on disk.
    auto write_slice_f( const char* filename ) const -> RTNType;
    auto write_slice_d( const char* filename ) const -> RTNType;

    void get_slice_dims( size_t&, size_t& ) const;

private:
    size_t m_dim_x           = 0;
    size_t m_dim_y           = 0;
    size_t m_total_vals      = 0;
    size_t m_stream_buf_size = 0;
    speck::buffer_type_uint8  m_stream_buf;

    speck::CDF97    m_cdf;
    speck::SPECK2D  m_decoder;

    float m_bpp = 0.0;
};


#endif
