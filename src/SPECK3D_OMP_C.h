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
    void set_num_threads( size_t );

    // Upon receiving incoming data, a chunking scheme is decided, and the volume
    // is divided and kept in separate chunks.
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
    size_t      m_dim_x       = 0; // Dimension of the entire volume
    size_t      m_dim_y       = 0; // Dimension of the entire volume
    size_t      m_dim_z       = 0; // Dimension of the entire volume
    size_t      m_chunk_x     = 0; // Dimension of the preferred chunk size
    size_t      m_chunk_y     = 0; // Dimension of the preferred chunk size
    size_t      m_chunk_z     = 0; // Dimension of the preferred chunk size
    size_t      m_num_threads = 1; // number of theads to use in OpenMP sections

    std::vector<speck::buffer_type_d>       m_chunk_buffers;
    std::vector<speck::smart_buffer_uint8>  m_encoded_streams;

    const size_t m_header_magic = 26; // header size would be this number + num_chunks * 4

#ifdef QZ_TERM
    int32_t     m_qz_lev      = 0;
    double      m_tol         = 0.0;
    // Outlier stats include 1) the number of outliers, and 2) the num of bytes to encode them.
    std::vector<std::pair<size_t, size_t>>  m_outlier_stats;
#else
    float       m_bpp         = 0.0;
#endif

    //
    // Private methods
    //
    auto m_generate_header() const -> speck::smart_buffer_uint8;

};

#endif
