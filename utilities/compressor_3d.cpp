#include "SPECK3D_Compressor.h"
#include "SPECK3D_Decompressor.h"

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
    int sam_read_n_bytes( const char* filename, size_t n_bytes,
                          void*       buffer               );
}

int main( int argc, char* argv[] )
{
    // Parse command line options
    CLI::App app("Parse CLI options to compressor_3d");

    std::string input_file;
    app.add_option("filename", input_file, "Input file to the compressor")
        ->required()->check(CLI::ExistingFile);

    bool decomp = false;
    auto* decomp_ptr = app.add_flag("-d", decomp, "Perform decompression (compression by default)");

    std::vector<size_t> dims;
    app.add_option("--dims", dims, "Dimensions of the volume")->expected(3)
            ->group("Compression Options");

#ifdef QZ_TERM
    int32_t qz_level = 0;
    auto* qz_level_ptr = app.add_option("--qz_level", qz_level, "Quantization level to encode")
            ->group("Compression Options");
#else
    float bpp = 0.0;
    auto* bpp_ptr = app.add_option("--bpp", bpp, "Average bit-per-pixel")
            ->group("Compression Options");
#endif

    std::string output_file;
    app.add_option("-o,--ofile", output_file, "Output filename.");

    bool print_stats = false;
    app.add_flag("--print_stats", print_stats, "Print stats (RMSE and L-Infinity)")
        ->group("Compression Options");

    float decomp_bpp = 0.0;
    auto* decomp_bpp_ptr = app.add_option("--partial_bpp", decomp_bpp,
            "Partially decode the bitstream up to a certain bit-per-pixel budget")
            ->group("Decompression Options");
    
    CLI11_PARSE(app, argc, argv);

    int rtn;

    //
    // Compression mode
    //
    if( !decomp ) {

        if( dims.empty() ) {
            std::cerr << "Please specify the input dimensions" << std::endl;
            return 1;
        }

#ifdef QZ_TERM
        if( !(*qz_level_ptr) ) {
            std::cerr << "Please specify the quantization level to encode" << std::endl;
            return 1;
        }
#else
        if( !(*bpp_ptr) ) {
            std::cerr << "Please specify the target bit-per-pixel rate" << std::endl;
            return 1;
        }
#endif        

        const size_t total_vals = dims[0] * dims[1] * dims[2];
        SPECK3D_Compressor compressor ( dims[0], dims[1], dims[2] );
        if( (rtn = compressor.read_floats( input_file.c_str() ) ) )
            return rtn;

#ifdef QZ_TERM
        compressor.set_qz_level( qz_level );
#else
        if( (rtn = compressor.set_bpp( bpp ) ) )
            return rtn;
#endif

        if( (rtn = compressor.compress() ) )
            return rtn;

        if( print_stats ) {
            // Need to do a decompression anyway
            speck::buffer_type_raw   comp_buf;
            size_t comp_buf_size;
            if( (rtn = compressor.get_compressed_buffer( comp_buf, comp_buf_size )) )
                return rtn;
            SPECK3D_Decompressor decompressor;
            decompressor.take_bitstream( std::move(comp_buf), comp_buf_size );
            decompressor.decompress();
            decompressor.write_volume_f( "reconstructed.vol" );
    
            speck::buffer_type_f decomp_buf;
            size_t decomp_buf_size;
            if( (rtn = decompressor.get_decompressed_volume_f( decomp_buf, decomp_buf_size )) )
                return rtn;

            if( decomp_buf_size != total_vals )
                return rtn;

            // Read the original input data again
            const size_t nbytes = sizeof(float) * total_vals;
            auto orig = speck::unique_malloc<float>(total_vals);
            if( (rtn = sam_read_n_bytes( input_file.c_str(), nbytes, orig.get() )) )
                return rtn;
            
            float rmse, lmax, psnr, arr1min, arr1max;
            if( (rtn = sam_get_statsf( orig.get(), decomp_buf.get(), total_vals,
                       &rmse, &lmax, &psnr, &arr1min, &arr1max )) )
                return rtn;

            printf("RMSE = %f, L-Infty = %f, PSNR = %f\n", rmse, lmax, psnr);
            printf("The original data range is: (%f, %f)\n", arr1min, arr1max);
        }
    
        if( !output_file.empty() ) {
            if( (rtn = compressor.write_bitstream( output_file.c_str() ) ) )
                return rtn;
        }
    }
    else {  // Decompression mode
        SPECK3D_Decompressor decompressor;
        decompressor.read_bitstream( input_file.c_str() );
        decompressor.set_bpp( decomp_bpp );
        decompressor.decompress();
        if( !output_file.empty() ) {
            if( (rtn = decompressor.write_volume_f( output_file.c_str() )) )
                return rtn;
        }
    }

    return 0;
}
