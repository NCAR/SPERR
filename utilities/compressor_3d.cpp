#include "SPECK3D_Compressor.h"
#include "SPECK3D_Decompressor.h"

#include "CLI11.hpp"

#include <cstdlib>
#include <iostream>
#include <chrono>
#include <cstring>


int main( int argc, char* argv[] )
{
    // Parse command line options
    CLI::App app("Parse CLI options to compressor_3d");

    std::string input_file;
    app.add_option("filename", input_file, "Input file to the compressor")
            ->required()->check(CLI::ExistingFile);

    bool decomp = false;
    auto* decomp_ptr = app.add_flag("-d,--decompress", decomp, "Perform decompression. \n"
            "If not specified, the program performs compression.");

    std::vector<size_t> dims;
    app.add_option("--dims", dims, "Dimensions of the input volume. \n"
            "For example, `--dims 128 128 128`.")->expected(3)
            ->group("Compression Options");

#ifdef QZ_TERM
    int32_t qz_level = 0;
    auto* qz_level_ptr = app.add_option("-q,--quantization_level", qz_level, 
            "Target quantization level to stop encoding. For example, \n"
            "if `-q n`, then the last quantization level will be 2^n.")
            ->group("Compression Options")->required();
#else
    float bpp = 0.0;
    auto* bpp_ptr = app.add_option("-b,--bit-per-pixel", bpp, 
            "Target bit-per-pixel on average. \nFor example, `-b 2.3`.")
             ->check(CLI::Range(0.0f, 64.0f))->group("Compression Options")->required();
#endif

    std::string output_file;
    app.add_option("-o,--output_filename", output_file, "Output filename.");

    bool print_stats = false;
    app.add_flag("-p,--print_stats", print_stats, "Print statistics (RMSE, L-Infinity, PSNR).")
            ->group("Compression Options");

    float decomp_bpp = 0.0;
    auto* decomp_bpp_ptr = app.add_option("--partial_bpp", decomp_bpp,
            "Partially decode the bitstream up to a certain bit-per-pixel. \n"
            "If not specified, the entire bitstream will be decoded.")
            ->check(CLI::Range(0.0f, 64.0f))
            ->group("Decompression Options");
    
    CLI11_PARSE(app, argc, argv);

    //
    // Compression mode
    //
    if( !decomp ) {

        if( dims.empty() ) {
            std::cerr << "Please specify the input dimensions" << std::endl;
            return 1;
        }

        const size_t total_vals = dims[0] * dims[1] * dims[2];
        SPECK3D_Compressor compressor ( dims[0], dims[1], dims[2] );
        if( compressor.read_floats( input_file.c_str() ) != speck::RTNType::Good )
            return 1;

#ifdef QZ_TERM
        compressor.set_qz_level( qz_level );
#else
        if( compressor.set_bpp( bpp ) != speck::RTNType::Good )
            return 1;
#endif

        auto start = std::chrono::steady_clock::now();
        if( compressor.compress() != speck::RTNType::Good )
            return 1;
        auto end = std::chrono::steady_clock::now();

        // Print compression time
        auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(end-start).count();
        std::cout << "Compression takes time: " << diff << "ms\n";

        // Let's obtain an encoded bitstream
        auto stream = compressor.get_encoded_bitstream();
        if( stream.first == nullptr || stream.second == 0 )
            return 1;

        // Print average bit-per-pixel
        const auto bpp = double(stream.second * 8) / double(total_vals);
        std::cout << "Average bit-per-pixel: " << bpp << "\n";

        if( print_stats ) {

            SPECK3D_Decompressor decompressor;
            decompressor.take_bitstream( std::move(stream) );
            if( decompressor.decompress() != speck::RTNType::Good )
                return 1;
    
            auto vol = decompressor.get_decompressed_volume_f();
            if( vol.first == nullptr || vol.second != total_vals )
                return 1;

            // Read the original input data again
            const size_t nbytes = sizeof(float) * total_vals;
            auto orig = std::make_unique<float[]>(total_vals);
            if( speck::read_n_bytes( input_file.c_str(), nbytes, orig.get() ) != speck::RTNType::Good )
                return 1;
            
            float rmse, lmax, psnr, arr1min, arr1max;
            speck::calc_stats( orig.get(), vol.first.get(), total_vals,
                               &rmse, &lmax, &psnr, &arr1min, &arr1max);

            printf("RMSE = %f, L-Infty = %f, PSNR = %f\n", rmse, lmax, psnr);
            printf("The original data range is: (%f, %f)\n", arr1min, arr1max);
        }
    
        if( !output_file.empty() ) {
            if( compressor.write_bitstream( output_file.c_str() ) != speck::RTNType::Good )
                return 1;
        }
    }

    //
    // Decompression mode
    //
    else {
        if( output_file.empty() ) {
            std::cerr << "Please specify output filename!" << std::endl;
            return 1;
        }
        SPECK3D_Decompressor decompressor;
        if( decompressor.read_bitstream( input_file.c_str() ) != speck::RTNType::Good )
            return 1;
        if( decompressor.set_bpp( decomp_bpp ) != speck::RTNType::Good )
            return 1;
        if( decompressor.decompress() != speck::RTNType::Good )
            return 1;
        if( decompressor.write_volume_f( output_file.c_str() ) != speck::RTNType::Good )
            return 1;
    }

    return 0;
}
