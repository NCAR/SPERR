#include "SPECK3D_OMP_C.h"

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
                   "E.g., `--dims 128 128 128`\n")->expected(3)->required();

    std::vector<size_t> chunks{ 64, 64, 64 };
    app.add_option("--chunks", chunks, "Dimensions of the preferred chunk size.\n"
                   "If not specified, then 64^3 will be used\n"
                   "E.g., `--chunks 64 64 64`.\n")->expected(3);

    size_t omp_num_threads = 4;
    app.add_option("--omp", omp_num_threads, "Number of OpenMP threads to use. Default: 4\n");

#ifdef QZ_TERM
    int32_t qz_level = 0;
    app.add_option("-q,--qz_level", qz_level, "Quantization level to reach when encoding\n"
                   "E.g., `-q n` means that the last quantization level is 2^n.\n"
                   "Note 1: smaller n usually yields smaller compression errors.\n"
                   "Note 2: n could be negative integers as well.")
                   ->group("Compression Parameters")->required();
    double tolerance = 0.0;
    app.add_option("-t,--tolerance", tolerance, "Maximum point-wise error tolerance\n"
                   "E.g., `-t 1e-2`")->group("Compression Parameters")->required();
#else
    float bpp;
    auto* bpp_ptr = app.add_option("--bpp", bpp, 
            "Target bit-per-pixel. E.g., `-bpp 2.3`.")
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
#else
    if( bpp < 0.0 || bpp > 32.0 ) {
        std::cerr << "Bit-per-pixel must be in the range (0.0, 32.0)!\n";
        return 1;
    }
#endif


    //
    // Let's do the actual work
    //
    const size_t total_vals = dims[0] * dims[1] * dims[2];
    SPECK3D_OMP_C compressor;
    compressor.set_dims( {dims[0], dims[1], dims[2]} );
    compressor.prefer_chunk_dims( {chunks[0], chunks[1], chunks[2]} );
    compressor.set_num_threads( omp_num_threads );

#ifdef QZ_TERM
    compressor.set_qz_level( qz_level );
    compressor.set_tolerance( tolerance );
#else
    compressor.set_bpp( bpp );
#endif

    auto orig = speck::read_whole_file<float>( input_file.c_str() );
    if( orig.size() != total_vals ) {
        std::cerr << "Read input file error: " << input_file << std::endl;
        return 1;
    }
    if( compressor.use_volume( orig.data(), orig.size()) != speck::RTNType::Good ) {
        std::cerr << "Copy data failed!" << std::endl;
        return 1;
    }

    // Free memory taken by `orig` since the volume has been put into chunks.
    orig.clear();
    orig.shrink_to_fit();

    if( compressor.compress() != speck::RTNType::Good ) {
        std::cerr << "Compression failed!" << std::endl;
        return 1;
    }

    auto stream = compressor.get_encoded_bitstream();
    if( stream.empty() )
        return 1;

    if( speck::write_n_bytes( output_file.c_str(), stream.size(), stream.data() ) != 
                              speck::RTNType::Good ) {
        std::cerr << "Write to disk failed!" << std::endl;
        return 1;
    }

    return 0;
}
