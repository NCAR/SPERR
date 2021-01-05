#include "SPECK2D_Compressor.h"
#include "SPECK2D_Decompressor.h"

#include "CLI11.hpp"

#include <cstdlib>
#include <iostream>
#include <chrono>
#include <cstring>

extern "C"  // C Function calls, and don't include the C header!
{
    int sam_get_statsf( const float* arr1, const float* arr2, size_t len,
                        float* rmse,       float* lmax,       float* psnr,
                        float* arr1min,    float* arr1max            );
}

int main( int argc, char* argv[] )
{
    // Parse command line options
    CLI::App app("Parse CLI options to compressor_3d");

    std::string input_file;
    app.add_option("filename", input_file, "Input file to the compressor")
            ->required()->check(CLI::ExistingFile);

    bool decomp = false;
    auto* decomp_ptr = app.add_flag("-d", decomp, "Perform decompression. \n"
            "If not specified, the program performs compression.");

    std::vector<size_t> dims;
    app.add_option("--dims", dims, "Dimensions of the input slice. \n"
            "For example, `--dims 128 128`.")->expected(2)
            ->group("Compression Options");

#ifdef QZ_TERM
    //int32_t qz_level = 0;
    //auto* qz_level_ptr = app.add_option("--qz_level", qz_level, 
    //        "Quantization level to stop encoding. For example, \n"
    //        "if `--qz_level n`, then the last quantization level will be 2^n.")
    //        ->group("Compression Options");
#else
    float bpp = 0.0;
    auto* bpp_ptr = app.add_option("--bpp", bpp, "Average bit-per-pixel")
            ->check(CLI::Range(0.0f, 64.0f))
            ->group("Compression Options");
#endif

    std::string output_file;
    app.add_option("-o,--ofile", output_file, "Output filename.");

    bool print_stats = false;
    app.add_flag("-p,--print_stats", print_stats, "Print statistics (RMSE and L-Infinity)")
            ->group("Compression Options");

    float decomp_bpp = 0.0;
    auto* decomp_bpp_ptr = app.add_option("--partial_bpp", decomp_bpp,
            "Partially decode the bitstream up to a certain bit-per-pixel. \n"
            "If not specified, the entire bitstream will be decoded.")
            ->check(CLI::Range(0.0f, 64.0f))
            ->group("Decompression Options");
    
    CLI11_PARSE(app, argc, argv);

    speck::RTNType rtn;

    //
    // Compression mode
    //
    if( !decomp ) {

        if( dims.empty() ) {
            std::cerr << "Please specify the input dimensions" << std::endl;
            return 1;
        }

#ifdef QZ_TERM
        //if( !(*qz_level_ptr) ) {
        //    std::cerr << "Please specify the quantization level to encode" << std::endl;
        //    return 1;
        //}
#else
        if( !(*bpp_ptr) ) {
            std::cerr << "Please specify the target bit-per-pixel rate" << std::endl;
            return 1;
        }
#endif        

        const size_t total_vals = dims[0] * dims[1];
        SPECK2D_Compressor compressor ( dims[0], dims[1] );
        if( (rtn = compressor.read_floats( input_file.c_str())) != speck::RTNType::Good ) {
            std::cerr << "Read raw data failed!" << std::endl;
            return 1;
        }

#ifdef QZ_TERM
        //compressor.set_qz_level( qz_level );
#else
        if( (rtn = compressor.set_bpp( bpp )) != speck::RTNType::Good ) {
            std::cerr << "Bit-per-pixel value invalid!" << std::endl;
            return 1;
        }
#endif

        auto start = std::chrono::steady_clock::now();
        if( (rtn = compressor.compress()) != speck::RTNType::Good ) {
            std::cerr << "Compression Failed!" << std::endl;
            return 1;
        }
        auto end = std::chrono::steady_clock::now();

        if( print_stats ) {

            // Print compression time
            auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(end-start).count();
            std::cout << "Compression takes time: " << diff << "ms\n";

            // Need to do a decompression anyway
            auto stream = compressor.get_encoded_bitstream();
            if( stream.first == nullptr || stream.second == 0 ) {
                std::cerr << "get_encoded_bitstream() Failed!" << std::endl;
                return 1;
            }
            SPECK2D_Decompressor decompressor;
            decompressor.take_bitstream( std::move(stream.first), stream.second );
            if( decompressor.decompress() != speck::RTNType::Good ) {
                std::cerr << "decompress() Failed!" << std::endl;
                return 1;
            }
    
            auto slice = decompressor.get_decompressed_slice_f();
            if( slice.first == nullptr || slice.second != total_vals ) {
                std::cerr << "get_decompressed_slice_f() Failed!" << std::endl;
                return 1;
            }

            // Read the original input data again
            const size_t nbytes = sizeof(float) * total_vals;
            auto orig = speck::unique_malloc<float>(total_vals);
            if( speck::read_n_bytes( input_file.c_str(), nbytes, orig.get() ) != speck::RTNType::Good )
                return 1;
            
            int rtn;
            float rmse, lmax, psnr, arr1min, arr1max;
            if( sam_get_statsf( orig.get(), slice.first.get(), total_vals,
                                &rmse, &lmax, &psnr, &arr1min, &arr1max ) )
                return 1;

            printf("RMSE = %f, L-Infty = %f, PSNR = %f\n", rmse, lmax, psnr);
            printf("The original data range is: (%f, %f)\n", arr1min, arr1max);
        }
    
        if( !output_file.empty() ) {
            if( compressor.write_bitstream( output_file.c_str() ) != speck::RTNType::Good ) {
                std::cerr << "write to file Failed!" << std::endl;
                return 1;
            }
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
        SPECK2D_Decompressor decompressor;
        if( decompressor.read_bitstream( input_file.c_str() ) != speck::RTNType::Good ) {
            std::cerr << "Read file failed!" << std::endl;
            return 1;
        }
        decompressor.set_bpp( decomp_bpp );
        if( decompressor.decompress() != speck::RTNType::Good ) {
            std::cerr << "Decompression failed!" << std::endl;
            return 1;
        }
        if( decompressor.write_slice_f( output_file.c_str()) != speck::RTNType::Good ) {
            std::cerr << "Write to file failed!" << std::endl;
            return 1;
        }
    }

    return 0;
}
