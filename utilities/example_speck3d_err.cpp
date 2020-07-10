#include "SPECK3D_Err.h"
#include <iostream>
#include <string>

int main( int argc, char* argv[] )
{
    if( argc != 6 ) {
        std::cout << "Usage: ./a.out dim_x dim_y dim_z tolerance outlier_percentage " 
                  << std::endl;
        return 1;
    }

    const size_t dim_x = std::stoi( argv[1] );
    const size_t dim_y = std::stoi( argv[2] );
    const size_t dim_z = std::stoi( argv[3] );
    const float  tol   = std::stof( argv[4] );
    const float  perc  = std::stof( argv[5] );

    if( perc <= 0.0f || perc >= 1.0f ) {
        std::cout << "outlier_percentage should be within range (0.0, 1.0)" << std::endl;
        return 1;
    }

    const size_t num_of_vals = dim_x * dim_y * dim_z;
    const size_t num_of_outs = size_t( num_of_vals * perc );

    speck::SPECK3D_Err se;
    se.set_dims( dim_x, dim_y, dim_z );
    se.set_tolerance( tol );

    std::vector<speck::Outlier> LOS;
    LOS.assign( num_of_outs, {} );  // default constructor of Outlier
    
    // Fill in random outliers
    

    se.add_outlier( 0, 0, 0, 0.11f );
    se.add_outlier( 3, 3, 3, 0.4f );
    se.add_outlier( 0, 0, 1, 0.5f );
    se.add_outlier( 1, 1, 1, 0.25f );
    se.add_outlier( 2, 2, 2, 0.7f );
    se.add_outlier( 0, 2, 2, -0.7f );
    
    se.add_outlier( 1, 1, 7, -0.25f );
    se.add_outlier( 8, 2, 2, 0.5f );
    se.add_outlier( 6, 2, 2, -0.125f );

    se.encode();

std::cout << "-- decoding -- " << std::endl;

    se.decode();

}
