//
// This is a the class object that is supposed to be used by most users, because
// it provides easy-to-use APIs.
// Functionality wise, it does not bring anything new though.
// 

#ifndef SPECK3D_DECOMPRESSOR_H
#define SPECK3D_DECOMPRESSOR_H


#include "CDF97.h"
#include "SPECK3D.h"
#include "SPERR.h"

using speck::RTNType;

class SPECK3D_Decompressor {

public:
    // Accept incoming data: copy from a raw memory block
    void copy_bitstream( const void* p, size_t len );

    // Accept incoming data by taking ownership of the memory block
    void take_bitstream( speck::smart_buffer_uint8 );

    // Accept incoming data by reading a file from disk.
    auto read_bitstream( const char* filename ) -> RTNType;

    auto set_bpp( float ) -> RTNType;

    auto decompress() -> RTNType;

    // Get the decompressed volume in a float or double buffer.
    auto get_decompressed_volume_f() const -> speck::smart_buffer_f;
    auto get_decompressed_volume_d() const -> speck::smart_buffer_d;

    // Write the decompressed volume as floats or doubles to a file on disk.
    auto write_volume_f( const char* filename ) const -> RTNType;
    auto write_volume_d( const char* filename ) const -> RTNType;

private:
    const size_t                m_meta_size         = 2;
    float                       m_bpp               = 0.0;

    // bitstreams can live in EITHER `m_entire_stream` OR 
    // `m_speck_stream` plus `m_sperr_stream`, but not both.
    // When a bitstream is first read from disk or passed in by others, 
    // it lives in `m_entire_stream`.
    // After the bitstream is parsed, the approperiate portions go to
    // `m_speck_stream` and/or `m_sperr_stream`.
    speck::smart_buffer_uint8   m_entire_stream     = {nullptr, 0};
    speck::smart_buffer_uint8   m_speck_stream      = {nullptr, 0};

    speck::CDF97                m_cdf;
    speck::SPECK3D              m_decoder;

#ifdef QZ_TERM
    speck::SPERR                m_sperr;
    speck::smart_buffer_uint8   m_sperr_stream      = {nullptr, 0};
    std::vector<speck::Outlier> m_sperr_los;
#endif

    //
    // Private methods
    //
    auto m_parse_metadata() -> RTNType;
};


#endif
