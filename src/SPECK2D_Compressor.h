//
// This is a the class object that is supposed to be used by most users, because
// it provides easy-to-use APIs.
// Functionality wise, it does not bring anything new though.
// 

#ifndef SPECK2D_COMPRESSOR_H
#define SPECK2D_COMPRESSOR_H


#include "CDF97.h"
#include "SPECK2D.h"

using speck::RTNType;

class SPECK2D_Compressor {

public:

    // Accept incoming data: copy from a raw memory block
    template< typename T >
    auto copy_data( const T* p, size_t len, size_t dimx, size_t dimy ) -> RTNType;

    // Accept incoming data by taking ownership of the memory block
    auto take_data( speck::buffer_type_d buf, size_t len, size_t dimx, size_t dimy ) -> RTNType;

//#ifdef QZ_TERM
    //void set_qz_level( int32_t );
//#else
    auto set_bpp( float ) -> RTNType;
//#endif

    auto compress() -> RTNType;
    auto get_encoded_bitstream() const -> std::pair<speck::buffer_type_uint8, size_t>;
    auto write_bitstream( const char* filename ) const -> RTNType;

private:
    size_t                  m_dim_x;
    size_t                  m_dim_y;
    size_t                  m_total_vals;
    const size_t            m_meta_size = 2;
    speck::buffer_type_d    m_val_buf;

    speck::CDF97            m_cdf;
    speck::SPECK2D          m_encoder;

//#ifdef QZ_TERM
    //int32_t m_qz_lev = 0;
//#else
    float m_bpp = 0.0;
//#endif
};


#endif
