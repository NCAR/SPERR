#include "SPECK3D_Err.h"
#include <iostream>

int main( int argc, char* argv[] )
{
    speck::SPECK3D_Err se;
    se.set_dims( 9, 4, 8 );
    se.set_tolerance( 0.1 );

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
