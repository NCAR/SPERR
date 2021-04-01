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

#ifdef USE_ZSTD
  #include "zstd.h"
#endif


using speck::RTNType;

class SPECK3D_Decompressor {

public:
    // Accept incoming data; this data is expected to have a header.
    auto use_bitstream( const void* p, size_t len ) -> RTNType;

    auto set_bpp( float ) -> RTNType;

    auto decompress() -> RTNType;

    // Get the decompressed volume in a float or double buffer.
    // It returns a smart_buffer_f or smart_buffer_d
    template<typename T>
    auto get_decompressed_volume() const -> std::pair<std::unique_ptr<T[]>, size_t>;

    auto get_dims() const -> std::array<size_t, 3>;

private:
    float                       m_bpp               = 0.0;
    size_t                      m_dim_x             = 0;
    size_t                      m_dim_y             = 0;
    size_t                      m_dim_z             = 0;

    std::vector<uint8_t>        m_speck_stream;

    speck::CDF97                m_cdf;
    speck::SPECK3D              m_decoder;

#ifdef QZ_TERM
    speck::SPERR                m_sperr;
    std::vector<uint8_t>        m_sperr_stream;
    std::vector<speck::Outlier> m_LOS;
#endif

#ifdef USE_ZSTD
    // The following resources are used repeatedly during the lifespan of an instance.
    // Note that `m_tmp_buf` will be written to using direct pointer access, so not
    // using a std::vector<uint_8> type here.
    speck::smart_buffer_uint8                             m_tmp_buf = {nullptr, 0};
    std::unique_ptr<ZSTD_DCtx, decltype(&ZSTD_freeDCtx)>  m_dctx = {nullptr, &ZSTD_freeDCtx};
#endif

};


#endif
