#include "CDF97.h"

#include <cstdlib>
#include <iostream>
#include <chrono>
#include <cassert>

extern "C"  // C Function calls, and don't include the C header!
{
    int sam_read_n_bytes(  const char*, size_t, void* );
    int sam_write_n_bytes( const char*, size_t, const void* );
    int sam_get_statsd( const double* arr1, const double* arr2, size_t len,
                        double* rmse,       double* lmax,   double* psnr,
                        double* arr1min,    double* arr1max            );
}


int main( int argc, char* argv[] )
{
    if( argc != 5 )
    {
        std::cerr << "Usage: ./a.out input_filename dim_x dim_y output_filename" << std::endl;
        return 1;
    }

    const char*   input = argv[1];
    const size_t  dim_x = std::atoi( argv[2] );
    const size_t  dim_y = std::atoi( argv[3] );
    const char*  output = argv[4];
    const size_t  total_vals = dim_x * dim_y;

    // Let read in binaries as 4-byte floats
    std::unique_ptr<float[]> in_buf( new float[ total_vals ] );
    if( sam_read_n_bytes( input, sizeof(float) * total_vals, in_buf.get() ) )
    {
        std::cerr << "Input read error!" << std::endl;
        return 1;
    }

    // Use a speck::CDF97 to perform DWT and IDWT.
    speck::CDF97 cdf;
    cdf.set_dims( dim_x, dim_y );
    cdf.copy_data( in_buf.get(), dim_x * dim_y );

    const auto startT = std::chrono::high_resolution_clock::now();
    cdf.dwt2d();
    cdf.idwt2d();
    const auto endT   = std::chrono::high_resolution_clock::now();
    const std::chrono::duration<double> diffT  = endT - startT;
    std::cout << "Time for transforms: " << diffT.count() << std::endl;

    // Write out the results after DWT.
    std::cout << "Mean is = " << cdf.get_mean() << std::endl;
    auto coeffs = cdf.get_read_only_data( );
    assert( coeffs.second == total_vals && coeffs.first != nullptr );
    if( sam_write_n_bytes( output, total_vals * sizeof(double), coeffs.first.get() ) )
    {
        std::cerr << "Output write error!" << std::endl;
        return 1;
    }

    // Compare the result with the original input in double precision
    std::unique_ptr<double[]> in_bufd( new double[ total_vals ] );
    for( size_t i = 0; i < total_vals; i++ )
        in_bufd[i] = in_buf[i];
    double rmse, lmax, psnr, arr1min, arr1max;
    sam_get_statsd( in_bufd.get(), coeffs.first.get(), 
                    total_vals, &rmse, &lmax, &psnr, &arr1min, &arr1max );
    printf("Sam: rmse = %f, lmax = %f, psnr = %fdB, orig_min = %f, orig_max = %f\n", 
            rmse, lmax, psnr, arr1min, arr1max );
}
