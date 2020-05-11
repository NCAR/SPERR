#include "SPECK3D.h"
#include "CDF97.h"

#include <cstdlib>
#include <iostream>
#include <chrono>
#include <cstring>

#include "SpeckConfig.h"

extern "C"  // C Function calls, and don't include the C header!
{
    int sam_read_n_bytes( const char*, size_t, void* );
}

int main( int argc, char* argv[] )
{
    if( argc != 8 )
    {
        std::cerr << "Usage: ./a.out input_filename dim_x dim_y dim_z "
                  << "ordering(XYZ or ZYX) output_filename bits_per_pixel " << std::endl;
        return 1;
    }

    const char*   input   = argv[1];
    const size_t  dim_x   = std::atol( argv[2] );
    const size_t  dim_y   = std::atol( argv[3] );
    const size_t  dim_z   = std::atol( argv[4] );
    const char*   fastest = argv[5];
    const char*   output  = argv[6];
    const float   bpp     = std::atof( argv[7] );
    const size_t  total_vals = dim_x * dim_y * dim_z;
    if( std::strcmp( fastest, "XYZ" ) != 0 && std::strcmp( fastest, "ZYX" ) != 0 )
    {
        std::cerr << "Must specify one of the two orderings: XYZ or ZYX!" << std::endl;
        return 1;
    }

    // Let's read in binaries as 4-byte floats
#ifdef NO_CPP14
    speck::buffer_type_f in_buf( new float[total_vals] );
#else
    speck::buffer_type_f in_buf = std::make_unique<float[]>( total_vals );
#endif
    if( sam_read_n_bytes( input, sizeof(float) * total_vals, in_buf.get() ) )
    {
        std::cerr << "Input read error!" << std::endl;
        return 1;
    }

    // Re-organize values if ZYX ordering is specified
    if( std::strcmp( fastest, "ZYX" ) == 0 )
    {
#ifdef NO_CPP14
        speck::buffer_type_f tmp( new float[total_vals] );
#else
        speck::buffer_type_f tmp = std::make_unique<float[]>( total_vals );
#endif
        size_t idx = 0;
        for( size_t z = 0; z < dim_z; z++ )
            for( size_t y = 0; y < dim_y; y++ )
                for( size_t x = 0; x < dim_x; x++ )
                {
                    size_t zyx_idx = x * dim_y * dim_z + y * dim_z + z;
                    tmp[ idx++ ] = in_buf[ zyx_idx ];
                }
        in_buf.reset();
        in_buf = std::move( tmp );
    }

    // Take input to go through DWT.
    speck::CDF97 cdf;
    cdf.set_dims( dim_x, dim_y, dim_z );
    cdf.take_data( std::move(in_buf), total_vals );
#ifdef TIME_EXAMPLES
    auto startT = std::chrono::high_resolution_clock::now();
#endif
    cdf.dwt3d();
#ifdef TIME_EXAMPLES
    auto endT   = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> diffT  = endT - startT;
    std::cout << "# Encoding time in milliseconds: Bit-per-Pixel  XForm_Time  SPECK_Time" 
              << std::endl; 
    std::cout << bpp << "  " << diffT.count() * 1000.0f << "  ";
#endif

    // Do a speck encoding
    speck::SPECK3D encoder;
    encoder.set_dims( dim_x, dim_y, dim_z );
    encoder.set_image_mean( cdf.get_mean() );
    const size_t total_bits = size_t(bpp * total_vals);
    encoder.set_bit_budget( total_bits );
    encoder.take_coeffs( cdf.release_data(), total_vals );
#ifdef TIME_EXAMPLES
    startT = std::chrono::high_resolution_clock::now();
#endif
    encoder.encode();
#ifdef TIME_EXAMPLES
    endT   = std::chrono::high_resolution_clock::now();
    diffT  = endT - startT;
    std::cout << diffT.count() * 1000.0f << std::endl;
#endif

    if( encoder.write_to_disk( output ) )
    {
        std::cerr << "Output write error!" << std::endl;
        return 1;
    }

    return 0;
}
