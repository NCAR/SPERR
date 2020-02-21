#include "SPECK2D.h"

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

    speck::SPECK2D speck;
    speck.assign_mean_dims( 0.0, dim_x, dim_y );
    speck.copy_coeffs( in_buf.get() );
    speck.speck2d();
    std::array<speck::SPECKSet2D, 3> subsets;
    speck.m_partition_I( subsets );
    speck.m_partition_I( subsets );
    speck.m_partition_I( subsets );
    

    // Compare the result with the original input in double precision
/*    std::unique_ptr<double[]> in_bufd( new double[ total_vals ] );
    for( long i = 0; i < total_vals; i++ )
        in_bufd[i] = in_buf[i];
    double rmse, lmax, psnr, arr1min, arr1max;
    sam_get_statsd( in_bufd.get(), cdf.get_data(), total_vals, 
                    &rmse, &lmax, &psnr, &arr1min, &arr1max );
    printf("Sam: rmse = %f, lmax = %f, psnr = %fdB, orig_min = %f, orig_max = %f\n", 
            rmse, lmax, psnr, arr1min, arr1max );
*/
}
