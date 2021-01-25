//
// This is a the class object that is supposed to be used by most users, because
// it provides easy-to-use APIs.
// Functionality wise, it does not bring anything new though.
// 

#ifndef SPECK3D_DECOMPRESSOR_H
#define SPECK3D_DECOMPRESSOR_H


#include "CDF97.h"
#include "SPECK3D.h"

using speck::RTNType;

class SPECK3D_Decompressor {

public:
    // Accept incoming data: copy from a raw memory block
    void copy_bitstream( const void* p, size_t len );

    // Accept incoming data by taking ownership of the memory block
    void take_bitstream( speck::buffer_type_uint8 buf, size_t len );

    // Accept incoming data by reading a file from disk.
    auto read_bitstream( const char* filename ) -> RTNType;

    auto set_bpp( float ) -> RTNType;

    auto decompress() -> RTNType;

    // Get the decompressed volume in a float or double buffer.
    auto get_decompressed_volume_f() const -> std::pair<speck::buffer_type_f, size_t>;
    auto get_decompressed_volume_d() const -> std::pair<speck::buffer_type_d, size_t>;

    // Write the decompressed volume as floats or doubles to a file on disk.
    auto write_volume_f( const char* filename ) const -> RTNType;
    auto write_volume_d( const char* filename ) const -> RTNType;

private:
    const size_t                m_meta_size         = 2;
    float                       m_bpp               = 0.0;
    speck::buffer_type_uint8    m_entire_stream;
    size_t                      m_entire_stream_len = 0;
    bool                        m_metadata_parsed   = false;

    speck::CDF97                m_cdf;
    speck::SPECK3D              m_decoder;

    //
    // Private methods
    //
    auto m_parse_metadata() -> RTNType;
};


#endif
