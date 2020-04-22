#include "SPECK2D.h"
#include "CDF97.h"

#include <cstdlib>
#include <iostream>
#include <chrono>
#include <array>

extern "C"  // C Function calls, and don't include the C header!
{
    int sam_read_n_bytes( const char*, size_t, void* );
    int sam_write_n_doubles( const char*, size_t, const double* );
    int sam_get_statsd( const double* arr1, const double* arr2, size_t len,
                        double* rmse,       double* lmax,   double* psnr,
                        double* arr1min,    double* arr1max            );
}


int main( int argc, char* argv[] )
{
    if( argc != 5 )
    {
        std::cerr << "Usage: ./a.out input_filename dim_x dim_y comp_ratio" << std::endl;
        return 1;
    }

    const char*   input  = argv[1];
    const size_t  dim_x  = std::atol( argv[2] );
    const size_t  dim_y  = std::atol( argv[3] );
    const float   cratio = std::atof( argv[4] );
    const size_t  total_vals = dim_x * dim_y;
    const std::string output("sam.tmp");

    // Let's read in binaries as 4-byte floats
    speck::buffer_type_f in_buf = std::make_unique<float[]>( total_vals );
    if( sam_read_n_bytes( input, sizeof(float) * total_vals, in_buf.get() ) )
    {
        std::cerr << "Input read error!" << std::endl;
        return 1;
    }

    // Take input to go through DWT.
    speck::CDF97 cdf;
    cdf.set_dims( dim_x, dim_y );
    cdf.copy_data( in_buf, dim_x * dim_y );
    const auto startT = std::chrono::high_resolution_clock::now();
    cdf.dwt2d();

    // Do a speck encoding
    speck::SPECK2D encoder;
    encoder.assign_dims( dim_x, dim_y );
    encoder.set_image_mean( cdf.get_mean() );
    encoder.copy_coeffs( cdf.get_read_only_data(), dim_x * dim_y );
    const size_t header_size  = 18;
    const size_t total_bits   = size_t(32.0f * total_vals / cratio) + header_size * 8;
    encoder.assign_bit_budget( total_bits );
    encoder.encode();
    encoder.write_to_disk( output );
    
    // Do a speck decoding
    speck::SPECK2D decoder;
    decoder.read_from_disk( output );
    decoder.assign_bit_budget( total_bits );
    decoder.decode();

    speck::CDF97 idwt;
    size_t dim_x_r, dim_y_r;
    decoder.get_dims( dim_x_r, dim_y_r );
    idwt.set_dims( dim_x_r, dim_y_r );
    idwt.set_mean( decoder.get_image_mean() );
    idwt.take_data( decoder.release_coeffs_double() );
    idwt.idwt2d();

    // Finish timer and print timing
    const auto endT   = std::chrono::high_resolution_clock::now();
    const std::chrono::duration<double> diffT  = endT - startT;
    std::cout << "Time for SPECK: " << diffT.count() * 1000.0f << std::endl;

    // Compare the result with the original input in double precision
    std::unique_ptr<double[]> in_bufd( new double[ total_vals ] );
    for( size_t i = 0; i < total_vals; i++ )
        in_bufd[i] = in_buf[i];
    double rmse, lmax, psnr, arr1min, arr1max;
    sam_get_statsd( in_bufd.get(), idwt.get_read_only_data().get(), 
                    total_vals, &rmse, &lmax, &psnr, &arr1min, &arr1max );
    printf("Sam: rmse = %f, lmax = %f, psnr = %fdB, orig_min = %f, orig_max = %f\n", 
            rmse, lmax, psnr, arr1min, arr1max );
}
