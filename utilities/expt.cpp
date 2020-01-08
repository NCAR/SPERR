#include "speck_helper.h"

#include <iostream>

int main()
{
    for( long i = 1; i < 130; i++ )
    {
        long lev1 = speck::calc_num_of_xform_levels( i );
        long lev2 = speck::calc_num_of_xform_levels2( i );

        //if( lev1 != lev2 )
            std::cout << "i = " << i << ",  " << lev1 << ",  " << lev2 << std::endl;
    }
}
