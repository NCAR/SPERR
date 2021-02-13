#include "SPECK3D_Compressor.h"
#include "SPECK3D_Decompressor.h"

#include "CLI11.hpp"

#include <cstdlib>
#include <iostream>
#include <chrono>
#include <cstring>
#include <cctype> // std::tolower()

// This file should only be compiled in non QZ_TERM mode.
#ifndef QZ_TERM


auto use_compressor( const float* in_buf, std::array<size_t, 3> dims, float bpp )
                    -> speck::smart_buffer_uint8 
{
    const size_t total_vals = dims[0] * dims[1] * dims[2];
    SPECK3D_Compressor compressor ( dims[0], dims[1], dims[2] );
    if( compressor.copy_data( in_buf, total_vals ) != speck::RTNType::Good ) {
        std::cerr << "  -- copying data failed!" << std::endl;
        return {nullptr, 0};
    }

    if( compressor.set_bpp( bpp ) != speck::RTNType::Good ) {
        std::cerr << "  -- setting BPP failed!\n";
        return {nullptr, 0};
    }

    auto start = std::chrono::steady_clock::now();
    if( compressor.compress() != speck::RTNType::Good ) {
        std::cerr << "  -- compression failed!\n";
        return {nullptr, 0};
    }
    auto end = std::chrono::steady_clock::now();
    auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(end-start).count();
    std::cout << " -> Compression takes time  : " << diff << "ms\n";

    auto stream = compressor.get_encoded_bitstream();
    if( stream.first == nullptr || stream.second == 0 ) {
        std::cerr << "  -- obtaining encoded bitstream failed!\n";
        return {nullptr, 0};
    }

    return stream;
}

auto use_decompressor( speck::smart_buffer_uint8 stream ) -> speck::smart_buffer_f
{
    SPECK3D_Decompressor decompressor;
    decompressor.take_bitstream( std::move(stream) );

    auto start = std::chrono::steady_clock::now();
    if( decompressor.decompress() != speck::RTNType::Good ) {
        std::cerr << "  -- decompression failed!\n";
        return {nullptr, 0};
    }
    auto end = std::chrono::steady_clock::now();
    auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(end-start).count();
    std::cout << " -> Decompression takes time: " << diff << "ms\n";

    auto vol = decompressor.get_decompressed_volume_f();
    if( vol.first == nullptr ) {
        std::cerr << "  -- obtaining reconstructed volume failed!\n";
        return {nullptr, 0};
    }

    return vol;
}

auto test_configuration( const float* in_buf, std::array<size_t, 3> dims, float bpp ) -> int
{
    const size_t total_vals = dims[0] * dims[1] * dims[2];

    auto stream = use_compressor( in_buf, dims, bpp );
    if( stream.first == nullptr || stream.second == 0 )
        return 1;
    printf("    Total compressed size in bytes = %ld, average bpp = %.2f\n", 
                stream.second, float(stream.second) * 8.0f / float(total_vals) );

    auto reconstruct = use_decompressor( std::move(stream) );
    if( reconstruct.first == nullptr || reconstruct.second != total_vals )
        return 1;

    float rmse, lmax, psnr, arr1min, arr1max;
    speck::calc_stats( in_buf, reconstruct.first.get(), total_vals,
                       &rmse, &lmax, &psnr, &arr1min, &arr1max);

    printf("    Original data range = (%.2e, %.2e)\n", arr1min, arr1max);
    printf("    RMSE = %.2e, L-Infty = %.2e, PSNR = %.2fdB\n", rmse, lmax, psnr);
    
    return 0;
}


int main( int argc, char* argv[] )
{
    //
    // Parse command line options
    //
    CLI::App app("CLI options to probe_3d");

    std::string input_file;
    app.add_option("filename", input_file, "Input data file to probe")
            ->required()->check(CLI::ExistingFile);

    std::vector<size_t> dims_v;
    app.add_option("--dims", dims_v, "Dimensions of the input volume.\n"
            "For example, `--dims 128 128 128`.")->required()->expected(3);

    float bpp;
    auto* bpp_ptr = app.add_option("--bpp", bpp, "Target bit-per-pixel value.\n"
                    "For example, `--bpp 0.5`.")->check(CLI::Range(0.0f, 64.0f));

    CLI11_PARSE(app, argc, argv);
    const std::array<size_t, 3> dims = {dims_v[0], dims_v[1], dims_v[2]};

    // Read and keep a copy of input data (will be used for evaluation)
    const size_t total_vals = dims[0] * dims[1] * dims[2];
    auto input_buf = std::make_unique<float[]>( total_vals );
    if( speck::read_n_bytes( input_file.c_str(), total_vals * sizeof(float), input_buf.get() )
        != speck::RTNType::Good ) {
        std::cerr << "  -- reading input file failed!" << std::endl;
        return 1;
    }

    //
    // Let's do an initial analysis
    //
    if( !(*bpp_ptr) ) {
        bpp = 4.0; // We decide to use 4 bpp for initial analysis
    }
    printf("Initial analysis: compression at %.2f bit-per-pixel...  \n", bpp);
    int rtn = test_configuration( input_buf.get(), dims, bpp );
    if( rtn != 0 )
        return rtn;

    //
    // Now it enters the interactive session
    //
    char answer;
    std::cout << "Do you want to explore other bit-per-pixel values? [y/n]:  ";
    std::cin >> answer;
    while ( std::tolower(answer) == 'y' ) {
        bpp = -1.0;
        std::cout << "Please input a new bpp value to test [0.0 - 64.0]:  ";
        std::cin >> bpp;
        while (bpp <= 0.0 || bpp >= 64.0 ) {
            std::cout << "Please input a bpp value inbetween [0.0 - 64.0]:  ";
            std::cin >> bpp;
        }
        printf("\nNow testing bpp = %.2f ...\n", bpp);
    
        rtn = test_configuration( input_buf.get(), dims, bpp );
        if ( rtn != 0 )
            return rtn;

        std::cout << "Do you want to try other bpp value? [y/n]:  ";
        std::cin >> answer;
        answer = std::tolower(answer);
        while( answer != 'y' && answer != 'n' ) {
            std::cout << "Do you want to try other bpp value? [y/n]:  ";
            std::cin >> answer;
            answer = std::tolower(answer);
        }
    }
    
    std::cout << "\nHave a good day! \n";
    
    return 0;
}


#endif
