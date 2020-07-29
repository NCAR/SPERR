#include "SPECK_Err.h"
#include <algorithm>
#include <cassert>
#include <random>
#include <iostream>
#include <fstream>
#include <string>


//
// Read the list of outliers from a file.
// This function will resize the list and fill it up with data read from the file.
//
void read_outliers( std::vector<speck::Outlier>&  list, 
                    const char*                   filename )
{
    std::ifstream file( filename, std::ios::binary | std::ios::ate );
    if( file.is_open() ) {
        auto len_char = file.tellg();
        assert( len_char % sizeof(speck::Outlier) == 0 );
        std::unique_ptr<char[]> buf = std::make_unique<char[]>( len_char );
        file.seekg(0);
        file.read( buf.get(), len_char );
        file.close();

        auto len_out = len_char / sizeof(speck::Outlier);
        list.clear();
        list.assign( len_out, speck::Outlier{} );
        size_t idx = 0;
        for( auto& e : list ) {
            e.location  = *(reinterpret_cast<size_t*>(buf.get() + idx));
            idx     +=   sizeof(size_t);
            e.error = *(reinterpret_cast<float*>(buf.get() + idx));
            idx     +=   sizeof(size_t);    // because of alignment requirements...
        }
    }
}

int main( int argc, char* argv[] )
{
    if( argc != 4 ) {
        std::cout << "Usage: ./a.out length tolerance outlier_number " 
                  << std::endl;
        return 1;
    }

    const size_t total_len = std::stoi( argv[1] );
    float        tol       = std::stof( argv[2] );
    size_t num_of_outs     = std::stol( argv[3] );

    // 
    // This chunk of code reads in outliers from a file.
    //
    std::vector<speck::Outlier> LOS;
    read_outliers( LOS, "top_outliers" );
    for( size_t i = 0; i < 10; i++ )
        printf("outliers: (%ld, %f)\n", LOS[i].location, LOS[i].error );

    assert( num_of_outs < LOS.size() );
    tol = std::abs( LOS[num_of_outs].error );
    std::cout << "tolerance = " << tol << std::endl;
    LOS.resize( num_of_outs );

    // Create an encoder
    speck::SPECK_Err se;
    se.set_length( total_len );
    se.set_tolerance( tol );
    se.add_outlier_list( LOS );

    std::cout << "encoding -- " << std::endl;
    se.encode();
    std::cout << "decoding -- " << std::endl;
    se.decode();
    std::cout << "num of outliers: " << LOS.size() << std::endl;
    std::cout << "average bpp:     " << float(se.num_of_bits()) / float(LOS.size()) << std::endl;

    // Now see if every element in LOS is successfully recovered
    std::cout << "checking correctness -- " << std::endl;
    auto recovered = se.release_outliers();

    for( const auto& orig : LOS ) {
        if( !std::any_of( recovered.cbegin(), recovered.cend(), 
            [&orig, tol](const auto& recov) { return (recov.location == orig.location &&
            std::abs( recov.error - orig.error ) < tol ); } ) ) {
            printf("failed to recover: (%ld, %f)\n", orig.location, orig.error );
        }
    }

}
