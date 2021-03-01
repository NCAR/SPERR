//
// This is a class that performs SPECK3D compression, and also utilizes OpenMP 
// to achieve parallelization: the input volume is divided into smaller chunks 
// and then they're processed individually.
//

#ifndef SPECK3D_OMP_C_H
#define SPECK3D_OMP_C_H


#include "SPECK3D_Compressor.h"

using speck::RTNType;

class SPECK3D_OMP_C
{
public:

    void set_dims( size_t, size_t, size_t );
    void prefer_chunk_size( size_t, size_t, size_t );

    // Upon receiving incoming data, a chunking scheme is decided, multiple instances
    // of SPECK3D_Compressor are created, and they retrieve chunks from the source.
    // Thus, SPECK3D_OMP_C doesn't keep a copy of the incoming data.
    template<typename T>
    auto use_volume( const T*, size_t ) -> RTNType;

#ifdef QZ_TERM
    void set_qz_level( int32_t );
    auto set_tolerance( double ) -> RTNType;
    // Return 1) the number of outliers, and 2) the num of bytes to encode them.
    auto get_outlier_stats() const -> std::pair<size_t, size_t>;
#else
    auto set_bpp( float ) -> RTNType;
#endif

    auto compress() -> RTNType;

    // Provide a copy of the encoded bitstream to the caller.
    auto get_encoded_bitstream() const -> speck::smart_buffer_uint8;


private:
    size_t      m_dim_x       = 0;
    size_t      m_dim_y       = 0;
    size_t      m_dim_z       = 0;
    size_t      m_chunk_x     = 0;
    size_t      m_chunk_y     = 0;
    size_t      m_chunk_z     = 0;
    size_t      m_num_chunks  = 0;

    std::vector<SPECK3D_Compressor>         m_compressors;
    std::vector<speck::smart_buffer_uint8>  m_encoded_streams;

#ifdef QZ_TERM
    int32_t     m_qz_lev      = 0;
    double      m_tol         = 0.0;
    std::vector<std::pair<size_t, size_t>>  m_otlr_stats;
#else
    float       m_bpp         = 0.0;
#endif

};


#endif
