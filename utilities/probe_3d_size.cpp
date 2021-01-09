#include "SPECK3D_Compressor.h"
#include "SPECK3D_Decompressor.h"

#include "CLI11.hpp"

#include <cstdlib>
#include <iostream>
#include <chrono>
#include <cstring>

// This file should only be compiled in non QZ_TERM mode.
#ifndef QZ_TERM

int main( int argc, char* argv[] )
{
    //
    // Parse command line options
    //
    CLI::App app("Parse CLI options to compressor_3d");

    std::string input_file;
    app.add_option("filename", input_file, "Input file to the compressor")
            ->required()->check(CLI::ExistingFile);

    std::vector<size_t> dims;
    app.add_option("--dims", dims, "Dimensions of the input volume. \n"
            "For example, `--dims 128 128 128`.")->expected(3)
            ->group("Compression Options");

    CLI11_PARSE(app, argc, argv);

    // Read and keep a copy of input data (will be used for evaluation)
    const size_t total_vals = dims[0] * dims[1] * dims[2];
    auto input_buf = speck::unique_malloc<float>( total_vals );
    if( speck::read_n_bytes( input_file.c_str(), total_vals * sizeof(float), input_buf.get() )
        != speck::RTNType::Good ) {
        std::cerr << "  -- reading input file failed!" << std::endl;
        return 1;
    }

    //
    // Let's use a compressor
    //
    float bpp = 4.0;    // We decide to use 4 bpp for initial analysis
    printf("Initial analysis: compression at %.2f bit-per-pixel...  ", bpp);
    SPECK3D_Compressor compressor ( dims[0], dims[1], dims[2] );
    if( compressor.copy_data( input_buf.get(), total_vals ) != speck::RTNType::Good ) {
        std::cerr << "  -- reading input file failed!" << std::endl;
        return 1;
    }

    if( compressor.set_bpp( bpp ) != speck::RTNType::Good ) {
        std::cerr << "  -- setting BPP failed!\n";
        return 1;
    }

    auto start = std::chrono::steady_clock::now();
    if( compressor.compress() != speck::RTNType::Good ) {
        std::cerr << "  -- compression failed!\n";
        return 1;
    }
    auto end = std::chrono::steady_clock::now();
    auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(end-start).count();
    std::cout << "(takes time: " << diff << "ms)\n";

    // Calculate the actual bpp
    auto stream = compressor.get_encoded_bitstream();
    if( stream.first == nullptr || stream.second == 0 ) {
        std::cerr << "  -- obtaining encoded bitstream failed!\n";
        return 1;
    }
    const auto actual_bpp = double(stream.second * 8) / double(total_vals);
    printf("  Actual bit-per-pixel = %f\n",  actual_bpp );


    //
    // Let's use a decompressor
    //
    printf("Initial analysis: decompression...  ");
    start = std::chrono::steady_clock::now();
    SPECK3D_Decompressor decompressor;
    decompressor.take_bitstream( std::move(stream.first), stream.second );
    if( decompressor.decompress() != speck::RTNType::Good ) {
        std::cerr << "  -- decompression failed!\n";
        return 1;
    }
    end = std::chrono::steady_clock::now();
    diff = std::chrono::duration_cast<std::chrono::milliseconds>(end-start).count();
    std::cout << "(takes time: " << diff << "ms)\n";

    auto vol = decompressor.get_decompressed_volume_f();
    if( vol.first == nullptr || vol.second != total_vals ) {
        std::cerr << "  -- obtaining reconstructed volume failed!\n";
        return 1;
    }

    float rmse, lmax, psnr, arr1min, arr1max;
    speck::calc_stats( input_buf.get(), vol.first.get(), total_vals,
                       &rmse, &lmax, &psnr, &arr1min, &arr1max);

    printf("==> Original data range = (%.2e, %.2e)\n", arr1min, arr1max);
    printf("==> RMSE = %.2e, L-Infty = %.2e, PSNR = %f\n", rmse, lmax, psnr);
    
    return 0;
}

#endif
