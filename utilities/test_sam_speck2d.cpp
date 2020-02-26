#include "SPECK2D.h"
#include "CDF97.h"

#include <cstdlib>
#include <iostream>
#include <chrono>
#include <array>

extern "C"  // C Function calls, and don't include the C header!
{
    int sam_read_n_bytes( const char*, long, void* );
    int sam_write_n_doubles( const char*, long, const double* );
    int sam_get_statsd( const double* arr1, const double* arr2, long len,
                        double* rmse,       double* lmax,   double* psnr,
                        double* arr1min,    double* arr1max            );
}


int main( int argc, char* argv[] )
{
    if( argc != 4 )
    {
        std::cerr << "Usage: ./a.out input_filename dim_x dim_y" << std::endl;
        return 1;
    }

    const char* input = argv[1];
    const long  dim_x = std::atol( argv[2] );
    const long  dim_y = std::atol( argv[3] );
    const long  total_vals = dim_x * dim_y;

    // Let read in binaries as 4-byte floats
    std::unique_ptr<float[]> in_buf( new float[ total_vals ] );
    if( sam_read_n_bytes( input, sizeof(float) * total_vals, in_buf.get() ) )
    {
        std::cerr << "Input read error!" << std::endl;
        return 1;
    }

    // Take input to go through DWT.
    speck::CDF97 cdf;
    cdf.copy_data( in_buf.get(), dim_x, dim_y );
    cdf.dwt2d();

    speck::SPECK2D speck;
    speck.assign_mean_dims( cdf.get_mean(), dim_x, dim_y );
    speck.take_coeffs( cdf.release_data() );
    const float cratio        = 10.0f;  /* compression ratio */
    const long  total_bits    = long(32.0f * total_vals / cratio);
    speck.assign_bit_budget( total_bits );
    speck.speck2d();
    

}
