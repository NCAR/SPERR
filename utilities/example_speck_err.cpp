#include "SPECK_Err.h"
#include <algorithm>
#include <cassert>
#include <random>
#include <iostream>
#include <fstream>
#include <string>

#if 0
//
// The list is already filled with Outliers with all zero's.
// This function assigns x, y, z coordinates using an uniform distribution,
// and assigns err values using a normal distribution that's greater than a tolerance
// 
void assign_outliers( std::vector<speck::Outlier>&  list, 
                      size_t                        dim_x, 
                      size_t                        dim_y,
                      size_t                        dim_z,
                      float                         tolerance )
{
    assert( tolerance > 0.0f );

    std::random_device rd;  //Will be used to obtain a seed for the random number engine
    std::mt19937 gen(rd()); //Standard mersenne_twister_engine seeded with rd()

    std::uniform_int_distribution<size_t> distrib_x (0, dim_x-1);
    std::uniform_int_distribution<size_t> distrib_y (0, dim_y-1);
    std::uniform_int_distribution<size_t> distrib_z (0, dim_z-1);

    // mean = 0.0, stddev = tolerance
    std::normal_distribution<float> distrib_err (0.0f, tolerance);

    for( auto& e : list ) {
        e.x = distrib_x( gen );
        e.y = distrib_y( gen );
        e.z = distrib_z( gen );
        while( std::abs( e.err ) < tolerance )
            e.err = distrib_err( gen );
    }
}
#endif

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
#if 0
            e.x    = *(reinterpret_cast<uint32_t*>(buf.get() + idx));
            idx   +=   sizeof(uint32_t);
            e.y    = *(reinterpret_cast<uint32_t*>(buf.get() + idx));
            idx   +=   sizeof(uint32_t);
            e.z    = *(reinterpret_cast<uint32_t*>(buf.get() + idx));
            idx   +=   sizeof(uint32_t);
            e.err  = *(reinterpret_cast<float*>(buf.get() + idx));
            idx   +=   sizeof(float);
#endif
            e.first  = *(reinterpret_cast<uint32_t*>(buf.get() + idx));
            idx     +=   sizeof(uint32_t);
            e.second = *(reinterpret_cast<float*>(buf.get() + idx));
            idx     +=   sizeof(float);
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

#if 0
    // Create an encoder
    speck::SPECK_Err se;
    se.set_length( total_len );
    se.set_tolerance( tol );
    
    // add outliers
    std::vector<speck::Outlier> LOS;
    LOS.emplace_back( 0, 0.2 );
    LOS.emplace_back( 1, 0.2 );
    LOS.emplace_back( 4, 0.4 );
    LOS.emplace_back( 8, 0.5 );
    LOS.emplace_back( 80, -0.3 );
    LOS.emplace_back( 60, -0.125 );
    LOS.emplace_back( 50, -0.25 );
    se.add_outlier_list( LOS );
#endif

    // 
    // This chunk of code reads in outliers from a file.
    //
    std::vector<speck::Outlier> LOS;
    read_outliers( LOS, "top_outliers" );
    for( size_t i = 0; i < 10; i++ )
        printf("outliers: (%d, %f)\n", LOS[i].first, LOS[i].second );

    assert( num_of_outs < LOS.size() );
    tol = std::abs( LOS[num_of_outs].second );
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

    //for( auto& recv : recovered )
    //        printf("recovered: (%d, %f)\n", recv.first, recv.second );

    for( const auto& orig : LOS ) {
        if( !std::any_of( recovered.cbegin(), recovered.cend(), 
            [&orig, tol](const auto& recov) { return (recov.first == orig.first &&
            std::abs( recov.second - orig.second ) < tol ); } ) ) {
            printf("failed to recover: (%d, %f)\n", orig.first, orig.second );
        }
    }

}
