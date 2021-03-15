//
// This is a the class object that is supposed to be used by most users, because
// it provides easy-to-use APIs.
// 

#ifndef SPECK3D_COMPRESSOR_H
#define SPECK3D_COMPRESSOR_H


#include "CDF97.h"
#include "SPECK3D.h"
#include "SPERR.h"

using speck::RTNType;

class SPECK3D_Compressor
{
public:

    // Accept incoming data: copy from a raw memory block
    template< typename T >
    auto copy_data( const T* p, size_t len, size_t dimx, size_t dimy, size_t dimz ) -> RTNType;

    // Accept incoming data: take ownership of a memory block
    auto take_data( speck::buffer_type_d buf, size_t len, size_t dimx, size_t dimy, size_t dimz ) 
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

    // Provide a copy of the encoded bitstream to the caller with proper metadata
    auto get_encoded_bitstream() const -> speck::smart_buffer_uint8;


private:
    size_t                      m_dim_x;
    size_t                      m_dim_y;
    size_t                      m_dim_z;
    size_t                      m_total_vals;
    speck::buffer_type_d        m_val_buf;

    speck::CDF97                m_cdf;
    speck::SPECK3D              m_encoder;

    // Store bitstreams from SPECK encoding and error correction if applicable
    speck::smart_buffer_uint8   m_speck_stream = {nullptr, 0};
    speck::smart_buffer_uint8   m_sperr_stream = {nullptr, 0};

#ifdef QZ_TERM
    speck::SPERR    m_sperr;
    int32_t         m_qz_lev      = 0;
    double          m_tol         = 0.0; // tolerance used in error correction
    size_t          m_num_outlier = 0;
#else
    float           m_bpp         = 0.0;
#endif
};


#endif
