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
        ->required();

    bool decomp = false;
    auto* decomp_ptr = app.add_flag("-d", decomp, "Perform decompression (compression by default)");

    std::vector<size_t> dims;
    app.add_option("--dims", dims, "Dimensions of the volume")
        ->expected(3)->group("Compression Options")->excludes(decomp_ptr);

    float bpp = 0.0;
    app.add_option("--bpp", bpp, "Average bit-per-pixel")
        ->group("Compression Options")->excludes(decomp_ptr);

    std::string output_file;
    app.add_option("-o,--ofile", output_file, "Output filename.");

    bool print_stats = false;
    app.add_flag("--stats", print_stats, "Print stats (RMSE and L-Infinity)")
        ->group("Compression Options")->excludes(decomp_ptr);
    
    CLI11_PARSE(app, argc, argv);


    //
    // Compression mode
    //
    if( !decomp ) {

        if( dims.empty() )
            return 1;

        const size_t total_vals = dims[0] * dims[1] * dims[2];
        int rtn;

        SPECK3D_Compressor compressor ( dims[0], dims[1], dims[2] );
        if( (rtn = compressor.read_floats( input_file.c_str() ) ) )
            return rtn;
        if( (rtn = compressor.set_bpp( bpp ) ) )
            return rtn;
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


    return 0;
}
