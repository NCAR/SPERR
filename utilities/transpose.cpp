#include <algorithm>
#include <cstdio>
#include <cassert>
#include <cstdlib>
#include <array>
#include <memory>
#include <iostream>

using dims_type = std::array<size_t, 3>;
using FL        = float;

size_t translate_index( dims_type idx,    // logical index of a point (x, y, z)
                        dims_type vol )   // volume dimension
{
    for( auto i = 0; i < 3; i++ )
        assert( idx[i] < vol[i] );

    return (idx[2] * vol[0] * vol[1]) + (idx[1] * vol[0]) + idx[0];
}

dims_type transpose_matthias( dims_type in )
{
    dims_type out = {in[1], in[2], in[0]};
    return out;
}

dims_type inverse_transpose_matthias( dims_type in )
{
    dims_type out = {in[2], in[0], in[1]};
    return out;
}

int main( int argc, char* argv[] )
{
    if( argc != 6 ) {
        std::cout << "Usage: ./a.out in_name, in_dimx, in_dimy, in_dimz, out_name" << std::endl;
        return -1;
    }

    const char*  in_name   = argv[1];
    const char*  out_name  = argv[5];
    const size_t in_dimx   = std::atol(argv[2]);
    const size_t in_dimy   = std::atol(argv[3]);
    const size_t in_dimz   = std::atol(argv[4]);
    const auto   total_len = in_dimx * in_dimy * in_dimz;

    std::FILE*  f = std::fopen( in_name, "rb" );
    if( f == nullptr ) {
        std::cerr << "file open error: " << in_name << std::endl;
        return -1;
    }

    // Read in the input file
    auto in_buf = std::make_unique<FL[]>( total_len );
    auto nread  = std::fread( in_buf.get(), sizeof(FL), total_len, f );
    std::fclose(f);
    if( nread != total_len ) {
        std::printf("file read error: expected to read %u vals, actually read %u vals.\n",
                     total_len, nread );
        return -1;
    }
    else {
        std::printf("read input file %s at (%u X %u X %u)\n", in_name, in_dimx, in_dimy, in_dimz);
    }

    // What output ordering is needed? 
    const auto in_dims  = std::array<size_t, 3>{in_dimx, in_dimy, in_dimz};
    const auto out_dims = transpose_matthias( in_dims );
    
    const auto special = float{3.14};
    auto out_buf = std::make_unique<FL[]>( total_len );
    std::fill( out_buf.get(), out_buf.get() + total_len, special );

    for( size_t z = 0; z < in_dimz; z ++ ) {
      for( size_t y = 0; y < in_dimy; y++ ) {
        for( size_t x = 0; x < in_dimx; x++ ) {

            // Again, what output ordering?
            auto out_logical = transpose_matthias( {x, y, z} );
            auto in_i        = translate_index(    {x, y, z}, in_dims );
            auto out_i       = translate_index( out_logical, out_dims );

            out_buf[out_i]   = in_buf[in_i];
        }
      }
    }

    auto missed = std::count( out_buf.get(), out_buf.get() + total_len, special );
    std::cout << "num of data points missed: " << missed << std::endl;

    // Write out the output file
    f = std::fopen( out_name, "wb" );
    if( f == nullptr ) {
        std::cerr << "file open error: " << out_name << std::endl;
        return -1;
    }
    auto nwrite = std::fwrite( out_buf.get(), sizeof(FL), total_len, f );
    std::fclose( f );
    if( nwrite != total_len ) {
        std::printf("file write error: expected to write %u vals, actually write %u vals.\n",
                     total_len, nwrite );
        return -1;
    }
    else {
        std::printf("written output file %s at (%u X %u X %u)\n", out_name,
                     out_dims[0], out_dims[1], out_dims[2] );
        return 0;
    }
}
