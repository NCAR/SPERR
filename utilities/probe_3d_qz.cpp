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
                         int32_t qz_level, float tolerance ) -> int 
{
    // Here we use individual CDF and SPECK encoders instead of 
    // `SPECK3D_Compressor` and `SPECK3D_Decompressor.`
    // That is because `SPECK3D_Compressor` does outlier correction already
    // in QZ_TERM mode!
    // A minor benefit is that we can reuse the same objects for encoding and decoding.

    // Step 1: Wavelet transform
    auto start_time = std::chrono::steady_clock::now();
    const size_t total_vals = dims[0] * dims[1] * dims[2];
    speck::CDF97 cdf;
    cdf.set_dims( dims[0], dims[1], dims[2] );   
    cdf.copy_data( in_buf, total_vals );
    cdf.dwt3d();
    auto cdf_out = cdf.release_data();
    
    // Step 2: SPECK3D encoding
    speck::SPECK3D speck;
    speck.set_dims( dims[0], dims[1], dims[2] );
    speck.set_image_mean( cdf.get_mean() );
    speck.set_quantization_term_level( qz_level );
    speck.take_data( std::move(cdf_out.first), cdf_out.second );
    auto rtn  = speck.encode();
    if(  rtn != RTNType::Good )
        return 1;
    auto end_time = std::chrono::steady_clock::now();
    auto diff_time = std::chrono::duration_cast<std::chrono::milliseconds>(end_time-start_time).count();
    std::cout << "    Compression takes time: " << diff_time << "ms\n";
    const auto num_bits = speck.get_bit_buffer_size();
    printf("    Total compressed size in bytes: ~%ld, average bpp: ~%.2f\n", 
                num_bits / 8, float(num_bits) / float(total_vals) );

    // Step 3: SPECK3D decoding
    start_time = std::chrono::steady_clock::now();
    rtn = speck.decode();
    if( rtn != RTNType::Good )
        return 1;
    auto speck_out = speck.release_data();

    // Step 4: Inverse wavelet transform
    cdf.take_data( std::move(speck_out.first), speck_out.second );
    cdf.idwt3d();
    auto vol_d = cdf.get_read_only_data();
    assert( vol_d.second == total_vals );
    auto reconstruct = std::make_unique<float[]>( total_vals );
    for( size_t i = 0; i < total_vals; i++ )
        reconstruct[i] = vol_d.first[i];
    end_time = std::chrono::steady_clock::now();
    diff_time = std::chrono::duration_cast<std::chrono::milliseconds>(end_time-start_time).count();
    std::cout << "    Decompression takes time: " << diff_time << "ms\n";

    // Step 5: Calculate statistics
    float rmse, lmax, psnr, arr1min, arr1max;
    speck::calc_stats( in_buf, reconstruct.get(), total_vals,
                       &rmse, &lmax, &psnr, &arr1min, &arr1max);
    printf("    Original data range = (%.2e, %.2e)\n", arr1min, arr1max);
    printf("    Reconstructed data RMSE = %.2e, L-Infty = %.2e, PSNR = %.2f\n", rmse, lmax, psnr);

    // Step 6: Count the number of outliers. 
    //         Estimate new bpp by approximating each encoded outlier taking 32 bits.
    auto num_outlier = std::inner_product( in_buf, in_buf + total_vals,
                                           reconstruct.get(), size_t{0},
                                           std::plus<>(),
                [tolerance](auto a, auto b){return std::abs(a-b) > tolerance ? 1 : 0;});
    printf("    With qz level = %d and tolerance = %.2e, "
           "num of outliers = %ld, pct = %.2f%%\n",
           qz_level, tolerance, num_outlier, float(num_outlier * 100) / float(total_vals) );
    printf("    Encoding these outliers will increase bpp to: ~%.2f\n",
                float(num_bits + num_outlier * 32) / float(total_vals) );

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
    auto input_buf = std::make_unique<float[]>( total_vals );
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
