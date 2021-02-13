#include "SPECK3D_Compressor.h"

#include "CLI11.hpp"

#include <cstdlib>
#include <iostream>
#include <cstring>


int main( int argc, char* argv[] )
{
    // Parse command line options
    CLI::App app("");

    std::string input_file;
    app.add_option("filename", input_file, "Input data file to the compressor")
            ->required()->check(CLI::ExistingFile);

    std::vector<size_t> dims;
    app.add_option("--dims", dims, "Dimensions of the input volume\n"
                   "E.g., `--dims 128 128 128`")->expected(3)->required();

#ifdef QZ_TERM
    int32_t qz_level = 0;
    auto* qz_level_ptr = app.add_option("-q,--qz_level", qz_level, 
            "Quantization level to reach when encoding\n"
            "E.g., `-q n` means that the last quantization level is 2^n.\n"
            "Note 1: smaller n usually yields smaller compression errors.\n"
            "Note 2: n could be negative integers as well.")
            ->group("Compression Parameters")->required();
    double tolerance = 0.0;
    auto* tol_ptr = app.add_option("-t,--tolerance", tolerance, 
                                   "Maximum point-wise error tolerance\nI.e., `-t 1e-2`")
                                   ->group("Compression Parameters")->required();
#else
    float bpp;
    auto* bpp_ptr = app.add_option("-b,--bpp", bpp, 
            "Target bit-per-pixel. E.g., `-b 2.3`.")
             ->check(CLI::Range(0.0f, 64.0f))
             ->group("Compression Parameters")->required();
#endif

    std::string output_file;
    app.add_option("-o", output_file, "Output filename")->required();

    CLI11_PARSE(app, argc, argv);

#ifdef QZ_TERM
    if( tolerance <= 0.0 ) { 
        std::cerr << "Tolerance must be a positive value!\n";
        return 1;
    }
#endif


    //
    // Let's do the actual work
    //
    const size_t total_vals = dims[0] * dims[1] * dims[2];
    SPECK3D_Compressor compressor ( dims[0], dims[1], dims[2] );
    if( compressor.read_floats( input_file.c_str() ) != speck::RTNType::Good ) {
        std::cerr << "Read input file error: " << input_file << std::endl;
        return 1;
    }

#ifdef QZ_TERM
    compressor.set_qz_level( qz_level );
    compressor.set_tolerance( tolerance );
#else
    compressor.set_bpp( bpp );
#endif

    if( compressor.compress() != speck::RTNType::Good ) {
        std::cerr << "Compression failed!" << std::endl;
        return 1;
    }

    if( compressor.write_bitstream( output_file.c_str() ) != speck::RTNType::Good ) {
        std::cerr << "Write to disk failed!" << std::endl;
        return 1;
    }

    return 0;
}
