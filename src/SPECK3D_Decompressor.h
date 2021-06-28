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
#include "Conditioner.h"

#ifdef USE_ZSTD
  #include "zstd.h"
#endif


using speck::RTNType;

class SPECK3D_Decompressor {

public:
    // Accept incoming data.
    auto use_bitstream( const void* p, size_t len ) -> RTNType;

#ifndef QZ_TERM
    auto set_bpp( float ) -> RTNType;
#endif

    auto decompress() -> RTNType;

    // Get the decompressed volume in a float or double buffer.
    template<typename T>
    auto get_data()  const -> std::vector<T>;
    auto view_data() const -> const std::vector<double>&;
    auto release_data()    -> std::vector<double>;
    auto get_dims() const  -> std::array<size_t, 3>;

private:
    speck::dims_type            m_dims = {0, 0, 0};

    speck::vec8_type            m_condi_stream;
    speck::vec8_type            m_speck_stream;
    speck::vecd_type            m_val_buf;

    speck::Conditioner          m_conditioner;
    speck::CDF97                m_cdf;
    speck::SPECK3D              m_decoder;

#ifdef QZ_TERM
    speck::SPERR                m_sperr;
    speck::vec8_type            m_sperr_stream;
#else
    float                       m_bpp   = 0.0;
#endif

#ifdef USE_ZSTD
    // The following resources are used repeatedly during the lifespan of an instance.
    std::unique_ptr<uint8_t[]>  m_zstd_buf     = nullptr;
    size_t                      m_zstd_buf_len = 0;
    std::unique_ptr<ZSTD_DCtx, decltype(&ZSTD_freeDCtx)>  m_dctx = {nullptr, &ZSTD_freeDCtx};
#endif

};


#endif
