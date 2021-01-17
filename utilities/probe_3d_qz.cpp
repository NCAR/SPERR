#include "SPECK3D_Compressor.h"
#include "SPECK3D_Decompressor.h"

#include "CLI11.hpp"

#include <cstdlib>
#include <iostream>
#include <chrono>
#include <cstring>
#include <cctype> // std::tolower

// This file should only be compiled in QZ_TERM mode.
#ifdef QZ_TERM


auto use_compressor( const float* in_buf, std::array<size_t, 3> dims, int32_t qz_level )
     -> std::pair<speck::buffer_type_uint8, size_t>
{
    const size_t total_vals = dims[0] * dims[1] * dims[2];
    SPECK3D_Compressor compressor ( dims[0], dims[1], dims[2] );
    if( compressor.copy_data( in_buf, total_vals ) != speck::RTNType::Good ) {
        std::cerr << "  -- copying data failed!" << std::endl;
        return {nullptr, 0};
    }

    compressor.set_qz_level( qz_level );

    auto start = std::chrono::steady_clock::now();
    if( compressor.compress() != speck::RTNType::Good ) {
        std::cerr << "  -- compression failed!\n";
        return {nullptr, 0};
    }
    auto end = std::chrono::steady_clock::now();
    auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(end-start).count();
    std::cout << "    Compression takes time  : " << diff << "ms\n";

    auto stream = compressor.get_encoded_bitstream();
    if( stream.first == nullptr || stream.second == 0 ) {
        std::cerr << "  -- obtaining encoded bitstream failed!\n";
        return {nullptr, 0};
    }

    return stream;
}

auto use_decompressor( std::pair<speck::buffer_type_uint8, size_t> stream )
     -> std::pair<speck::buffer_type_f, size_t>
{
    SPECK3D_Decompressor decompressor;
    decompressor.take_bitstream( std::move(stream.first), stream.second );

    auto start = std::chrono::steady_clock::now();
    if( decompressor.decompress() != speck::RTNType::Good ) {
        std::cerr << "  -- decompression failed!\n";
        return {nullptr, 0};
    }
    auto end = std::chrono::steady_clock::now();
    auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(end-start).count();
    std::cout << "    Decompression takes time: " << diff << "ms\n";

    auto vol = decompressor.get_decompressed_volume_f();
    if( vol.first == nullptr ) {
        std::cerr << "  -- obtaining reconstructed volume failed!\n";
        return {nullptr, 0};
    }

    return vol;
}

auto test_configuration( const float* in_buf, std::array<size_t, 3> dims, int32_t qz_level,
                         float tolerance ) -> int 
{
    const size_t total_vals = dims[0] * dims[1] * dims[2];

    auto stream = use_compressor( in_buf, dims, qz_level );
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
    printf("    RMSE = %.2e, L-Infty = %.2e, PSNR = %.2f\n", rmse, lmax, psnr);

    size_t num_outlier = 0;
    for( size_t i = 0; i < total_vals; i++ ) {
        if( std::abs( in_buf[i] - reconstruct.first[i] ) > tolerance )
            num_outlier++;
    }
    printf("    With qz level = %d and tolerance = %.2e, "
           "num of outliers = %ld, pct = %.2f%%\n",
           qz_level, tolerance, num_outlier, float(num_outlier * 100) / float(total_vals) );

    // Here we approximate each encoded outlier taking 32 bits.
    printf("    Encoding these outliers will increase bpp to ~ %.2f\n",
                float(stream.second + num_outlier * 4) * 8.0f / float(total_vals) );
    
    return 0;
}


int main( int argc, char* argv[] )
{
    //
    // Parse command line options
    //
    CLI::App app("CLI options to probe_3d");

    std::string input_file;
    app.add_option("filename", input_file, "Input file to the probe")
            ->required()->check(CLI::ExistingFile);

    std::vector<size_t> dims_v;
    app.add_option("--dims", dims_v, "Dimensions of the input volume. \n"
            "For example, `--dims 128 128 128`.")->required()->expected(3);

    float tolerance = 0.0;
    app.add_option("-t", tolerance, "Maximum point-wise error tolerance.\n"
                   "For example, `-t 0.001`.")->required();

    int32_t qz_level;
    auto* qz_level_ptr = app.add_option("-q", qz_level, 
                         "Integer quantization level to test. \nFor example, `-q -10`.");

    CLI11_PARSE(app, argc, argv);
    if( tolerance <= 0.0 ) {
        std::cerr << "Error tolerance must be a positive value!\n";
        return 1;
    }
    const std::array<size_t, 3> dims = {dims_v[0], dims_v[1], dims_v[2]};

    // Read and keep a copy of input data (will be used for evaluation)
    const size_t total_vals = dims[0] * dims[1] * dims[2];
    auto input_buf = speck::unique_malloc<float>( total_vals );
    if( speck::read_n_bytes( input_file.c_str(), total_vals * sizeof(float), input_buf.get() )
        != speck::RTNType::Good ) {
        std::cerr << "  -- reading input file failed!" << std::endl;
        return 1;
    }

    //
    // Let's do an initial analysis to pick an initial QZ level.
    // The strategy is to use a QZ level that about 1/1000 of the input data range.
    //
    const auto minmax = std::minmax_element( input_buf.get(), input_buf.get() + total_vals );
    const auto range  = *minmax.second - *minmax.first;
    if( !(*qz_level_ptr) ) {
        qz_level  = int32_t(std::floor(std::log2(range / 1000.0)));
    }

    printf("Initial analysis: compression at quantization level %d ...  \n", qz_level);
    int rtn = test_configuration( input_buf.get(), dims, qz_level, tolerance );
    if( rtn != 0 )
        return rtn;

    //
    // Now it enters the interactive session
    //
    char answer;
    std::cout << "Do you want to explore other quantization levels? [y/n]:  ";
    std::cin >> answer;
    while ( std::tolower(answer) == 'y' ) {
        int32_t tmp;
        std::cout << "Please input a new qz level to test "
                     "(larger values mean more compression):  ";
        std::cin >> tmp;
        while (tmp < qz_level - 10 || tmp > qz_level + 10) {
            printf("Please input a qz level within (-10, 10) of %d:  ", qz_level);
            std::cin >> tmp;
        }
        qz_level = tmp;
        printf("\nNow testing qz level = %d ...\n", qz_level);
    
        rtn = test_configuration( input_buf.get(), dims, qz_level, tolerance );
        if ( rtn != 0 )
            return rtn;

        std::cout << "Do you want to try other qz level? [y/n]:  ";
        std::cin >> answer;
        answer = std::tolower(answer);
        while( answer != 'y' && answer != 'n' ) {
            std::cout << "Do you want to try other qz level? [y/n]:  ";
            std::cin >> answer;
            answer = std::tolower(answer);
        }
    }
    
    std::cout << "\nHave a good day! \n";
    
    return 0;
}


#endif
