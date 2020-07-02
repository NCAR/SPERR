#include "SPECK3D_Err.h"
#include <iostream>

int main( int argc, char* argv[] )
{
    speck::SPECK3D_Err se;
    se.set_dims( 4, 4, 4 );
    se.set_tolerance( 0.1 );

    //se.add_outlier( 0, 0, 1, 0.11f );
    //se.add_outlier( 3, 3, 3, 0.4f );
    se.add_outlier( 0, 0, 0, 0.5f );
    se.add_outlier( 1, 1, 1, 0.25f );
    se.add_outlier( 2, 2, 2, 0.7f );
    
    se.encode();

std::cout << "-- decoding -- " << std::endl;

    se.decode();

    for( size_t i = 0; i < se.get_num_of_outliers(); i++ ) {
        auto out = se.get_ith_outlier( i );
        printf("outlier: (%d, %d, %d, %f)\n", out.x, out.y, out.z, out.err );
    }
}
