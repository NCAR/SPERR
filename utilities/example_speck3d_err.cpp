#include "SPECK3D_Err.h"

int main( int argc, char* argv[] )
{
    speck::SPECK3D_Err se;
    se.set_dims( 4, 4, 4 );
    se.set_tolerance( 0.1 );

    se.add_outlier( 0, 0, 0, 0.3f );
    
    se.encode();
}
