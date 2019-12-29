#include "CDF97.h"

#include <cstdlib>
#include <iostream>

extern "C"  // C Function calls, and don't include the C header!
{
    int sam_read_n_bytes( const char*, long, void* );
    int sam_write_n_doubles( const char*, long, const double* );
}


int main( int argc, char* argv[] )
{
    if( argc != 5 )
    {
        std::cerr << "Usage: ./a.out input_filename dim_x dim_y output_filename" << std::endl;
        return 1;
    }

    const char* input = argv[1];
    const long  dim_x = std::atol( argv[2] );
    const long  dim_y = std::atol( argv[3] );
    const char* output = argv[4];
    const long  total_vals = dim_x * dim_y;

    // Let read in binaries as 4-byte floats
    float* in_buf = new float[ total_vals ];
    if( sam_read_n_bytes( input, sizeof(float) * total_vals, in_buf ) )
    {
        std::cerr << "Input read error!" << std::endl;
        return 1;
    }

    // Use a speck::CDF97 to perform DWT.
    speck::CDF97 cdf;
    cdf.assign_data( in_buf, dim_x, dim_y );
    cdf.dwt2d();

    // Write out the coefficients after DWT.
    const auto* coeffs = cdf.get_data();
    if( sam_write_n_doubles( output, total_vals, coeffs ) )
    {
        std::cerr << "Output write error!" << std::endl;
        return 1;
    }
    std::cout << "Mean is = " << cdf.get_mean() << std::endl;

    delete[] in_buf;
}
