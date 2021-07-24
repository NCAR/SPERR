#include "SPECK3D_OMP_D.h"

#include "CLI11.hpp"

#include <cstdlib>
#include <cstring>
#include <iostream>


int main( int argc, char* argv[] )
{
    // Parse command line options
    CLI::App app("");

    std::string input_file;
    app.add_option("input_filename", input_file, "Input compressed file to the decompressor")
            ->required()->check(CLI::ExistingFile);

#ifndef QZ_TERM
    // Partial bitstream decompression is only applicable to fixed-size mode.
    float decomp_bpp = 0.0;
    auto* decomp_bpp_ptr = app.add_option("--partial_bpp", decomp_bpp,
            "Partially decode the bitstream up to a certain bit-per-pixel. \n"
            "If not specified, the entire bitstream will be decoded.")
            ->check(CLI::Range(0.0f, 64.0f))
            ->group("Decompression Options");
#endif

    std::string output_file;
    app.add_option("-o", output_file, "Output filename\n")->required();

    size_t omp_num_threads = 4;
    app.add_option("--omp", omp_num_threads, "Number of OpenMP threads to use. Default: 4\n");

    std::string compare_file;
    auto* compare_file_ptr = app.add_option("--compare", compare_file,
            "Pass in the original data file so the decompressor could\n"
            "compare the decompressed data against (PSNR, L-Infty, etc.).")
            ->check(CLI::ExistingFile);

    CLI11_PARSE(app, argc, argv);


    //
    // Let's do the actual work
    //
    auto in_stream = speck::read_whole_file<uint8_t>( input_file.c_str() );
    if( in_stream.empty() )
        return 1;
    SPECK3D_OMP_D decompressor;
    decompressor.set_num_threads( omp_num_threads );
    if( decompressor.use_bitstream(in_stream.data(), in_stream.size()) != speck::RTNType::Good ) {
        std::cerr << "Read compressed file error: " << input_file << std::endl;
        return 1;
    }

#ifndef QZ_TERM
    decompressor.set_bpp( decomp_bpp );
#endif

    if( decompressor.decompress( in_stream.data() ) != speck::RTNType::Good ) {
        std::cerr << "Decompression failed!" << std::endl;
        return 1;
    }

    // Let's free up some memory here
    const auto in_stream_num_bytes = in_stream.size();
    in_stream.clear();
    in_stream.shrink_to_fit();

    auto vol = decompressor.get_data<float>();
    if( vol.empty() )
        return 1;
    if( speck::write_n_bytes( output_file.c_str(), vol.size() * sizeof(float), 
                              vol.data() ) != speck::RTNType::Good ) {
        std::cerr << "Write to disk failed!" << std::endl;
        return 1;
    }

    // Compare with the original data if user specifies
    if( *compare_file_ptr ) {
        auto orig = speck::read_whole_file<float>( compare_file.c_str() );
        if( orig.size() != vol.size() ) {
            std::cerr << "File to compare with has difference size "
                         "with the decompressed file!" << std::endl;
            return 1;
        }

        printf("Average bit-per-pixel = %.2f\n", in_stream_num_bytes * 8.0f / orig.size());

        float rmse, lmax, psnr, arr1min, arr1max;
        speck::calc_stats( orig.data(), vol.data(), orig.size(),
                           &rmse, &lmax, &psnr, &arr1min, &arr1max);
        printf("Original data range = (%.2e, %.2e)\n", arr1min, arr1max);
        printf("Decompressed data RMSE = %.2e, L-Infty = %.2e, PSNR = %.2fdB\n", rmse, lmax, psnr);
    }

    return 0;
}
