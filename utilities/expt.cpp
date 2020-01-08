#include "speck_helper.h"
#include "SPECK.h"

#include <iostream>
#include <cstdlib>


void print_size( const speck::SPECKSet2D& set )
{
    std::cout << "    start position: " << set.start_x << ",  " << set.start_y << std::endl;
    std::cout << "    length        : " << set.length_x << ",  " << set.length_y << std::endl;
}

int main( int argc, char* argv[] )
{
    if( argc != 4 )
    {
        std::cout << "Usage: ./a.out dim_x, dim_y, num_of_levels" << std::endl;
        return 1;
    }

    const long dim_x   = std::stol( argv[1] );
    const long dim_y   = std::stol( argv[2] );
    const long num_lev = std::stol( argv[3] );

    speck::SPECK    spk;
    spk.assign_mean_dims( 0.0, dim_x, dim_y, 1 );

    speck::SPECKSet2D set( speck::SPECKSetType::TypeS );
    set.part_level = num_lev;
    for( long i = 0; i < 4; i++ )
    {
        spk.m_calc_set_size_2d( set, i );
        std::cout << "subband = " << i << std::endl;
        print_size( set );
    }
}
