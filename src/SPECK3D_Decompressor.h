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
    auto use_bitstream( const void* p, size_t len ) -> RTNType;

    auto set_bpp( float ) -> RTNType;

    auto decompress() -> RTNType;

    // Get the decompressed volume in a float or double buffer.
    auto get_decompressed_volume_f() const -> speck::smart_buffer_f;
    auto get_decompressed_volume_d() const -> speck::smart_buffer_d;

private:
    const size_t                m_meta_size         = 2;
    float                       m_bpp               = 0.0;

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
    //auto m_parse_metadata() -> RTNType;
};


#endif
