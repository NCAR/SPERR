#include "SPECK3D_Compressor.h"

#include <cstdlib>
#include <iostream>
#include <chrono>
#include <cstring>

extern "C"  // C Function calls, and don't include the C header!
{
    int sam_read_n_bytes( const char*, size_t, void* );
}

int main( int argc, char* argv[] )
{
    if( argc != 7 )
    {
        std::cerr << "Usage: ./a.out input_filename dim_x dim_y dim_z "
                  << "output_filename bits_per_pixel " << std::endl;
        return 1;
    }

    const char*   input   = argv[1];
    const size_t  dim_x   = std::atol( argv[2] );
    const size_t  dim_y   = std::atol( argv[3] );
    const size_t  dim_z   = std::atol( argv[4] );
    const char*   output  = argv[5];
    const float   bpp     = std::atof( argv[6] );
    const size_t  total_vals = dim_x * dim_y * dim_z;

    int rtn;

    SPECK3D_Compressor compressor ( dim_x, dim_y, dim_z );
    
    rtn = compressor.read_floats( input );
    if( rtn != 0 )
        return rtn;
    compressor.set_bpp( bpp );
    rtn = compressor.compress();
    if( rtn != 0 )
        return rtn;
    
    rtn = compressor.write_bitstream( output );

    return 0;
}
