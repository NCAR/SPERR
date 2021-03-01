#include "SPECK3D_Compressor.h"
#include "SPECK3D_Decompressor.h"

#include "CLI11.hpp"

#include <cstdlib>
#include <cassert>
#include <cstring>
#include <iostream>
#include <chrono>
#include <cctype>   // std::tolower
#include <numeric>  // std::inner_product
#include <functional>  // std::plus<>


#ifdef QZ_TERM
// This file should only be compiled in QZ_TERM mode.


auto test_configuration( const float* in_buf, std::array<size_t, 3> dims, 
                         int32_t qz_level, double tolerance ) -> int 
{
    // Setup
    const size_t total_vals = dims[0] * dims[1] * dims[2];
    SPECK3D_Compressor compressor( dims[0], dims[1], dims[2] );
    compressor.copy_data( in_buf, total_vals );
    compressor.set_qz_level( qz_level );
    compressor.set_tolerance( tolerance );

    // Perform compression
    auto start_time = std::chrono::steady_clock::now();
    auto rtn  = compressor.compress();
    if(  rtn != RTNType::Good )
        return 1;
    auto end_time = std::chrono::steady_clock::now();
    auto diff_time = std::chrono::duration_cast<std::chrono::milliseconds>(end_time-start_time).count();
    std::cout << " -> Compression takes time: " << diff_time << "ms\n";
    auto encoded_stream = compressor.get_encoded_bitstream();
    auto otl_count = compressor.get_outlier_stats();

    // Print some analysis
    if( speck::empty_buf( encoded_stream ) )
        return 1;
    printf("    Total compressed size in bytes = %ld, average bpp = %.2f\n", 
               encoded_stream.second, float(encoded_stream.second * 8) / float(total_vals) );
    if( otl_count.first == 0 ) {
        printf("    There are no outliers at this quantization level!\n");
    }
    else {
        printf("    Outliers: num = %ld, pct = %.2f%%, bpp ~ %.2f, "
                   "using total storage ~ %.2f%%\n",
                otl_count.first, float(otl_count.first * 100) / float(total_vals),
                float(otl_count.second * 8) / float(otl_count.first),
                float(otl_count.second * 100) / float(encoded_stream.second) );
    }

    // Perform decompression
    SPECK3D_Decompressor decompressor;
    if( decompressor.use_bitstream( encoded_stream.first.get(), encoded_stream.second ) !=
                                    RTNType::Good )
        return 1;
    start_time = std::chrono::steady_clock::now();
    rtn = decompressor.decompress();
    if( rtn != RTNType::Good )
        return 1;
    end_time = std::chrono::steady_clock::now();
    diff_time = std::chrono::duration_cast<std::chrono::milliseconds>(end_time-start_time).count();
    std::cout << " -> Decompression takes time: " << diff_time << "ms\n";
    auto decoded_volume = decompressor.get_decompressed_volume<float>();
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
    app.add_option("filename", input_file, "Input file to the probe")
            ->required()->check(CLI::ExistingFile);

    std::vector<size_t> dims_v;
    app.add_option("--dims", dims_v, "Dimensions of the input volume. "
            "E.g., `--dims 128 128 128`.\n")->required()->expected(3);

    bool rel_tol = false;
    app.add_flag("--rel_tol", rel_tol, "Use relative error tolerance (as a percentage).\n");

    double tolerance = 0.0;
    app.add_option("-t", tolerance, "Maximum point-wise error tolerance. E.g., `-t 0.001`.\n"
                   "By default, it takes the input as an absolute error tolerance.\n"
                   "If flag `--rel_tol` presents, the input is treated as a percentage\n"
                   "of input data range that point-wise errors could occur.\n"
                   "E.g., `-t 5` means that error tolerance is at 5% of the data range.\n")
                    ->required();

    int32_t qz_level;
    auto* qz_level_ptr = app.add_option("-q,--qz_level", qz_level, 
                         "Integer quantization level to test. E.g., `-q -10`. \n"
                         "If not specified, the probe will pick one for you.");

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
    // Let's do an initial analysis to pick an initial QZ level.
    // The strategy is to use a QZ level that about 1/1000 of the input data range.
    //
    const auto minmax = std::minmax_element( input_buf.get(), input_buf.get() + total_vals );
    const auto range  = *minmax.second - *minmax.first;
    if( !(*qz_level_ptr) ) {
        qz_level  = int32_t(std::floor(std::log2(range / 1000.0)));
    }
    if( rel_tol ) {
        if( tolerance <= 0.0 || tolerance >= 100.0 ) {
            std::cerr << "Relative error tolerance must be in the range of [0, 100]\n";
            return 1;
        }
        else {
            auto frc  = tolerance / 100.0;
            tolerance = range * frc;
        }
    }
    else {
        if( tolerance <= 0.0 ) {
            std::cerr << "Absolute error tolerance must be a positive value!\n";
            return 1;
        }
    }

    printf("Initial analysis: absolute error tolerance = %.2e, quantization level = %d ...  \n", 
            tolerance, qz_level);
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
        std::cout << "Please input a new qz level to test: ";
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
