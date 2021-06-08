//
// This is a the class object that is supposed to be used by most users, because
// it provides easy-to-use APIs.
// 

#ifndef SPECK3D_COMPRESSOR_H
#define SPECK3D_COMPRESSOR_H


#include "CDF97.h"
#include "SPECK3D.h"
#include "SPERR.h"
#include "Conditioner.h"

#ifdef USE_ZSTD
  #include "zstd.h"
#endif


using speck::RTNType;

class SPECK3D_Compressor
{
public:

    // Accept incoming data: copy from a raw memory block
    template< typename T >
    auto copy_data( const T* p, size_t len, size_t dimx, size_t dimy, size_t dimz ) -> RTNType;

    // Accept incoming data: take ownership of a memory block
    auto take_data( std::vector<double>&& buf, size_t dimx, size_t dimy, size_t dimz ) 
                    -> RTNType;

#ifdef QZ_TERM
    void set_qz_level( int32_t );
    auto set_tolerance( double ) -> RTNType;
    // Return 1) the number of outliers, and 2) the num of bytes to encode them.
    auto get_outlier_stats() const -> std::pair<size_t, size_t>;
#else
    auto set_bpp( float ) -> RTNType;
#endif

    auto compress() -> RTNType;

    auto view_encoded_bitstream() const -> const std::vector<uint8_t>&;
    auto get_encoded_bitstream() -> std::vector<uint8_t>;


private:
    size_t                      m_dim_x;
    size_t                      m_dim_y;
    size_t                      m_dim_z;
    speck::vecd_type            m_val_buf;

    speck::Conditioner          m_conditioner;
    speck::CDF97                m_cdf;
    speck::SPECK3D              m_encoder;

    // Store bitstreams from SPECK encoding and error correction if applicable
    std::vector<uint8_t>        m_condi_stream;
    std::vector<uint8_t>        m_speck_stream;

#ifdef QZ_TERM
    std::vector<uint8_t>        m_sperr_stream;
    speck::SPERR                m_sperr;
    int32_t                     m_qz_lev      = 0;
    double                      m_tol         = 0.0; // tolerance used in error correction
    size_t                      m_num_outlier = 0;
    std::vector<speck::Outlier> m_LOS; // List of OutlierS
    speck::vecd_type            m_tmp_diff;
#else
    float                       m_bpp         = 0.0;
#endif

#ifdef USE_ZSTD
    // The following resources are used repeatedly during the lifespan of an instance,
    // but they only play temporary roles, so OK to be mutable.
    mutable std::vector<uint8_t>  m_zstd_buf;
    mutable std::unique_ptr<ZSTD_CCtx, decltype(&ZSTD_freeCCtx)>  m_cctx =
            {nullptr, &ZSTD_freeCCtx};
#endif

    speck::vec8_type    m_encoded_stream;

    auto m_prepare_encoded_bitstream() -> speck::RTNType;
};


#endif
