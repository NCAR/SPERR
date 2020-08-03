#include "SPECK3D.h"
#include "CDF97.h"

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <chrono>


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


#ifdef QZ_TERM
    if( argc != 7 ) {
        std::cerr << "Usage: ./a.out input_filename dim_x dim_y dim_z " <<
                     "absolute/n_iterations qz_levels" << std::endl;
#else
    if( argc != 6 ) {
        std::cerr << "Usage: ./a.out input_filename dim_x dim_y dim_z bit-per-pixel" << std::endl;
#endif
        return 1;
    }
    
    const char*   input  = argv[1];
    const size_t  dim_x  = std::atol( argv[2] );
    const size_t  dim_y  = std::atol( argv[3] );
    const size_t  dim_z  = std::atol( argv[4] );
    const size_t  total_vals = dim_x * dim_y * dim_z;

    char output[256];
    std::strcpy( output, argv[0] );
    std::strcat( output, ".tmp" );

#ifdef QZ_TERM
    bool absolute_qz_level;
    if( std::strcmp( argv[5], "absolute" ) == 0 )
        absolute_qz_level = true;
    else if( std::strcmp( argv[5], "n_iterations" ) == 0 )
        absolute_qz_level = false;
    else {
        std::cerr << "EITHER *absolute* or *n_iterations* needs to be specified!" << std::endl;
        return 1;
    }
    const int qz_levels = std::atoi( argv[6] );
    if( absolute_qz_level == false && qz_levels <= 0 ) {
        std::cerr << "A positive value must be used for *n_iterations* quantization levels!" 
                  << std::endl;
        return 1;
    }
#else
    const float bpp = std::atof( argv[5] );
#endif

#ifdef NO_CPP14
    speck::buffer_type_f in_buf( new float[total_vals] );
#else
    speck::buffer_type_f in_buf = std::make_unique<float[]>( total_vals );
#endif

    // Let's read in binaries as 4-byte floats
    if( sam_read_n_bytes( input, sizeof(float) * total_vals, in_buf.get() ) )
    {
        std::cerr << "Input read error!" << std::endl;
        return 1;
    }

    // Take input to go through DWT.
    speck::CDF97 cdf;
    cdf.set_dims( dim_x, dim_y, dim_z );
    cdf.copy_data( in_buf, total_vals );
    const auto startT = std::chrono::high_resolution_clock::now();
    cdf.dwt3d();

    // Do a speck encoding
    speck::SPECK3D encoder;
    encoder.set_dims( dim_x, dim_y, dim_z );
    encoder.set_image_mean( cdf.get_mean() );
    encoder.take_coeffs( cdf.release_data(), total_vals );

#ifdef QZ_TERM
    if( absolute_qz_level )
        encoder.set_quantization_term_level( qz_levels );
    else
        encoder.set_quantization_iterations( qz_levels );
#else
    encoder.set_bit_budget( total_vals * bpp );
#endif

    encoder.encode();
    encoder.write_to_disk( output );

    // Do a speck decoding
    speck::SPECK3D  decoder;
    decoder.read_from_disk( output );
    decoder.set_bit_budget( 0 );    // decode all available bits
    decoder.decode();

    // Do an inverse wavelet transform
    speck::CDF97 idwt;
    size_t dim_x_r, dim_y_r, dim_z_r;
    decoder.get_dims( dim_x_r, dim_y_r, dim_z_r );
    idwt.set_dims( dim_x_r, dim_y_r, dim_z_r );
    idwt.set_mean( decoder.get_image_mean() );
    idwt.take_data( decoder.release_coeffs_double(), dim_x_r * dim_y_r * dim_z_r );
    idwt.idwt3d();

    // Finish timer and print timing
    const auto endT   = std::chrono::high_resolution_clock::now();
    const std::chrono::duration<double> diffT  = endT - startT;
    std::cout << "Time for SPECK in milliseconds: " << diffT.count() * 1000.0f << std::endl;

    // Compare the result with the original input in double precision
    speck::buffer_type_d in_bufd = std::make_unique<double[]>( total_vals );
    for( size_t i = 0; i < total_vals; i++ )
        in_bufd[i] = in_buf[i];

    double rmse, lmax, psnr, arr1min, arr1max;
    sam_get_statsd( in_bufd.get(), idwt.get_read_only_data().get(), 
                    total_vals, &rmse, &lmax, &psnr, &arr1min, &arr1max );
    printf("Sam stats: rmse = %f, lmax = %f, psnr = %fdB, orig_min = %f, orig_max = %f\n", 
            rmse, lmax, psnr, arr1min, arr1max );
    in_bufd.reset( nullptr );

#ifdef QZ_TERM
    float bpp = float(encoder.get_num_of_bits()) / float(total_vals);

    if( !absolute_qz_level )
        printf("With %d levels of quantization, average BPP = %f, and qz terminates at level %d\n",
                qz_levels, bpp, encoder.get_quantization_term_level() );
    else
        printf("Quantization terminates at level %d with average BPP = %f\n", qz_levels, bpp );
#endif

}
