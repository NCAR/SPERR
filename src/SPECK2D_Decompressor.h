//
// This is a the class object that is supposed to be used by most users, because
// it provides easy-to-use APIs.
// Functionality wise, it does not bring anything new though.
// 

#ifndef SPECK2D_DECOMPRESSOR_H
#define SPECK2D_DECOMPRESSOR_H


#include "CDF97.h"
#include "SPECK2D.h"
#include "Conditioner.h"

using speck::RTNType;

class SPECK2D_Decompressor {

public:
    // Accept incoming data.
    auto use_bitstream( const void* p, size_t len ) -> RTNType;

    auto set_bpp( float ) -> RTNType;

    auto decompress() -> RTNType;

    // Get the decompressed data in a float or double buffer.
    template<typename T>
    auto get_data()  const -> std::vector<T>;
    auto view_data() const -> const std::vector<double>&;
    auto release_data()    -> std::vector<double>;
    auto get_dims()  const -> std::array<size_t, 3>;

private:
    speck::dims_type            m_dims = {0, 0, 0};
    speck::vec8_type            m_condi_stream;
    speck::vec8_type            m_speck_stream;
    speck::vecd_type            m_val_buf;

    const size_t                m_meta_size = 2;
    float                       m_bpp       = 0.0;

    speck::Conditioner          m_conditioner;
    speck::CDF97                m_cdf;
    speck::SPECK2D              m_decoder;

};


#endif
