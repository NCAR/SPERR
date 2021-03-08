#include "SPECK3D_Decompressor.h"

#include "CLI11.hpp"

#include <cstdlib>
#include <iostream>
#include <cstring>


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
    app.add_option("-o", output_file, "Output filename")->required();

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
    if( speck::empty_buf(in_stream) )
        return 1;
    SPECK3D_Decompressor decompressor;
    if( decompressor.use_bitstream_header(in_stream.first.get(), in_stream.second) != 
                                          speck::RTNType::Good ) {
        std::cerr << "Read compressed file error: " << input_file << std::endl;
        return 1;
    }

#ifndef QZ_TERM
    decompressor.set_bpp( decomp_bpp );
#endif

    if( decompressor.decompress() != speck::RTNType::Good ) {
        std::cerr << "Decompression failed!" << std::endl;
        return 1;
    }

    auto vol = decompressor.get_decompressed_volume<float>();
    if( speck::empty_buf(vol) )
        return 1;
    if( speck::write_n_bytes( output_file.c_str(), vol.second * sizeof(float), 
                              vol.first.get() ) != speck::RTNType::Good ) {
        std::cerr << "Write to disk failed!" << std::endl;
        return 1;
    }

    // Compare with the original data if user specifies
    if( *compare_file_ptr ) {
        auto orig = speck::read_whole_file<float>( compare_file.c_str() );
        auto decomp = decompressor.get_decompressed_volume<float>();
        if( orig.second != decomp.second ) {
            std::cerr << "File to compare with has difference size "
                         "with the decompressed file!" << std::endl;
            return 1;
        }

        float rmse, lmax, psnr, arr1min, arr1max;
        speck::calc_stats( orig.first.get(), decomp.first.get(), orig.second,
                           &rmse, &lmax, &psnr, &arr1min, &arr1max);
        printf("Original data range = (%.2e, %.2e)\n", arr1min, arr1max);
        printf("Decompressed data RMSE = %.2e, L-Infty = %.2e, PSNR = %.2fdB\n", rmse, lmax, psnr);
    }

    return 0;
}
