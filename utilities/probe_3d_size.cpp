#include "SPECK3D_Compressor.h"
#include "SPECK3D_Decompressor.h"

#include "SPECK3D_OMP_C.h"
#include "SPECK3D_OMP_D.h"

#include "CLI11.hpp"

#include <cstdlib>
#include <iostream>
#include <chrono>
#include <cstring>
#include <cctype> // std::tolower()

// This file should only be compiled in non QZ_TERM mode.
#ifndef QZ_TERM

#if 0
auto use_compressor( const float* in_buf, std::array<size_t, 3> dims, float bpp )
                    -> speck::smart_buffer_uint8 
{
    const size_t total_vals = dims[0] * dims[1] * dims[2];
    SPECK3D_Compressor compressor;
    if( compressor.copy_data( in_buf, total_vals, dims[0], dims[1], dims[2] ) != speck::RTNType::Good ) {
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
    if( decompressor.use_bitstream( stream.first.get(), stream.second ) != RTNType::Good )
        return {nullptr, 0};

    auto start = std::chrono::steady_clock::now();
    if( decompressor.decompress() != speck::RTNType::Good ) {
        std::cerr << "  -- decompression failed!\n";
        return {nullptr, 0};
    }
    auto end = std::chrono::steady_clock::now();
    auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(end-start).count();
    std::cout << " -> Decompression takes time: " << diff << "ms\n";

    auto vol = decompressor.get_decompressed_volume<float>();
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
#endif


auto test_configuration_omp( const float* in_buf, std::array<size_t, 3> dims, 
                             std::array<size_t, 3> chunks,
                             float bpp, size_t omp_num_threads ) -> int
{
    // Setup
    const size_t total_vals = dims[0] * dims[1] * dims[2];
    SPECK3D_OMP_C compressor;
    compressor.set_dims(dims[0], dims[1], dims[2]);
    compressor.prefer_chunk_size( chunks[0], chunks[0], chunks[0] );
    compressor.set_num_threads( omp_num_threads );
    compressor.set_bpp( bpp );
    auto rtn = compressor.use_volume( in_buf, total_vals );
    if(  rtn != RTNType::Good )
        return 1;

    // Perform actual compression work
    auto start_time = std::chrono::steady_clock::now();
    rtn = compressor.compress();
    if(  rtn != RTNType::Good )
        return 1;
    auto end_time = std::chrono::steady_clock::now();
    auto diff_time = std::chrono::duration_cast<std::chrono::milliseconds>(end_time-start_time).count();
    std::cout << " -> Compression takes time: " << diff_time << "ms\n";

    auto encoded_stream = compressor.get_encoded_bitstream();
    if( speck::empty_buf( encoded_stream ) )
        return 1;
    else
        printf("    Total compressed size in bytes = %ld, average bpp = %.2f\n",
               encoded_stream.second, float(encoded_stream.second * 8) / float(total_vals) );


    // Perform decompression
    SPECK3D_OMP_D decompressor;
    decompressor.set_num_threads( omp_num_threads );
    rtn = decompressor.use_bitstream( encoded_stream.first.get(), encoded_stream.second );
    if( rtn != RTNType::Good )
        return 1;

    start_time = std::chrono::steady_clock::now();
    rtn = decompressor.decompress( encoded_stream.first.get() );
    if(  rtn != RTNType::Good )
        return 1;
    end_time = std::chrono::steady_clock::now();
    diff_time = std::chrono::duration_cast<std::chrono::milliseconds>(end_time-start_time).count();
    std::cout << " -> Decompression takes time: " << diff_time << "ms\n";

    auto decoded_volume = decompressor.get_data_volume<float>();
    if( !speck::size_is( decoded_volume, total_vals ) )
        return 1;


    // Collect statistics
    float rmse, lmax, psnr, arr1min, arr1max;
    speck::calc_stats( in_buf, decoded_volume.first.get(), total_vals,
                       &rmse, &lmax, &psnr, &arr1min, &arr1max);
    printf("    Original data range = (%.2e, %.2e)\n", arr1min, arr1max);
    printf("    Reconstructed data RMSE = %.2e, L-Infty = %.2e, PSNR = %.2fdB\n", rmse, lmax, psnr);

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
            "For example, `--dims 128 128 128`.\n")->required()->expected(3);

    std::vector<size_t> chunks_v{ 64, 64, 64 };
    app.add_option("--chunks", chunks_v, "Dimensions of the preferred chunk size. "
            "E.g., `--chunks 64 64 64`.\n"
            "If not specified, then 64^3 will be used\n")->expected(3);

    float bpp;
    auto* bpp_ptr = app.add_option("--bpp", bpp, "Target bit-per-pixel value.\n"
                    "For example, `--bpp 0.5`.\n")->check(CLI::Range(0.0f, 64.0f));

    size_t omp_num_threads = 4;
    auto* omp_ptr = app.add_option("--omp", omp_num_threads, "Number of OpenMP threads to use. "
                                   "Default: 4\n"); 

    CLI11_PARSE(app, argc, argv);

    const std::array<size_t, 3> dims = {dims_v[0], dims_v[1], dims_v[2]};

    // Read and keep a copy of input data (will be used for evaluation)
    const size_t total_vals = dims[0] * dims[1] * dims[2];
    auto [input_buf, buf_len] = speck::read_whole_file<float>( input_file.c_str() );
    if( input_buf == nullptr || buf_len != total_vals ) {
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
    int rtn = test_configuration_omp( input_buf.get(), dims, {chunks_v[0], chunks_v[1], chunks_v[2]},
                                      bpp, omp_num_threads );
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
        std::cout << "\nPlease input a new bpp value to test [0.0 - 64.0]:  ";
        std::cin >> bpp;
        while (bpp <= 0.0 || bpp >= 64.0 ) {
            std::cout << "Please input a bpp value inbetween [0.0 - 64.0]:  ";
            std::cin >> bpp;
        }
        printf("\nNow testing bpp = %.2f ...\n", bpp);
    
        rtn = test_configuration_omp( input_buf.get(), dims, {chunks_v[0], chunks_v[1], chunks_v[2]},
                                      bpp, omp_num_threads );
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
