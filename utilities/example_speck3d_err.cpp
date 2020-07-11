#include "SPECK3D_Err.h"
#include <algorithm>
#include <cassert>
#include <random>
#include <iostream>
#include <string>

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

    // mean = 0.0, stddev = tolerance / 2
    std::normal_distribution<float> distrib_err (0.0f, tolerance * 0.5f);

    for( auto& e : list ) {
        e.x = distrib_x( gen );
        e.y = distrib_y( gen );
        e.z = distrib_z( gen );
        while( std::abs( e.err ) < tolerance )
            e.err = distrib_err( gen );
    }
}

int main( int argc, char* argv[] )
{
    if( argc != 6 ) {
        std::cout << "Usage: ./a.out dim_x dim_y dim_z tolerance outlier_number " 
                  << std::endl;
        return 1;
    }

    const size_t dim_x = std::stoi( argv[1] );
    const size_t dim_y = std::stoi( argv[2] );
    const size_t dim_z = std::stoi( argv[3] );
    const float  tol   = std::stof( argv[4] );

    const size_t num_of_vals = dim_x * dim_y * dim_z;
    const size_t num_of_outs = std::stol( argv[5] );

    // Create a list of outliers
    std::vector<speck::Outlier> LOS;
    LOS.assign( num_of_outs, speck::Outlier{} );  // default constructor of Outlier
    assign_outliers( LOS, dim_x, dim_y, dim_z, tol );

    // Sort these outliers
    std::sort( LOS.begin(), LOS.end(), [](speck::Outlier& a, speck::Outlier& b) {
                if( a.x < b.x )         return true;
                else if( a.x > b.x )    return false;
                else if( a.y < b.y )    return true;
                else if( a.y > b.y )    return false;
                else if( a.z < b.z )    return true;
                else if( a.z > b.z )    return false;
                else                    return false; } );

    std::cout << "Before cleaning num of outliers: " << LOS.size() << std::endl;

    // Remove duplicates
    auto it = std::unique( LOS.begin(), LOS.end(), 
                    [](speck::Outlier& a, speck::Outlier& b) {
                    return (a.x == b.x && a.y == b.y && a.z == b.z );
                    } );
    LOS.erase( it, LOS.end() );

    std::cout << "After cleaning num of outliers: " << LOS.size() << std::endl;
    //for( auto& e : LOS )
    //    printf("(%d, %d, %d, %f)\n", e.x, e.y, e.z, e.err );   // Print this outlier
    

    // Create an encoder
    speck::SPECK3D_Err se;
    se.set_dims( dim_x, dim_y, dim_z );
    se.set_tolerance( tol );

    se.add_outlier_list( LOS );
    //se.add_outlier( 4, 2, 9, -0.158822 );

    se.encode();

std::cout << "-- decoding -- " << std::endl;

    se.decode();

    auto recovered = se.release_outliers();

    // Now see if every element in LOS is successfully recovered
    for( auto& orig : LOS ) {
        if( !std::any_of( recovered.cbegin(), recovered.cend(), 
            [&orig, tol](auto recov) { return (recov.x == orig.x && recov.y == orig.y && 
            recov.z == orig.z && std::abs( recov.err - orig.err ) < tol ); } ) )
            printf("failed to recover: (%d, %d, %d, %f)\n", orig.x, orig.y, orig.z, orig.err );
    }
}
