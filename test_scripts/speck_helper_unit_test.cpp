
#include "speck_helper.h"
#include "gtest/gtest.h"

namespace
{

TEST( speck_helper, num_of_xforms )
{
    EXPECT_EQ( speck::calc_num_of_xforms( 1 ), 0 );
    EXPECT_EQ( speck::calc_num_of_xforms( 2 ), 0 );
    EXPECT_EQ( speck::calc_num_of_xforms( 7 ), 0 );
    EXPECT_EQ( speck::calc_num_of_xforms( 8 ), 1 );
    EXPECT_EQ( speck::calc_num_of_xforms( 9 ), 1 );
    EXPECT_EQ( speck::calc_num_of_xforms( 15 ), 1 );
    EXPECT_EQ( speck::calc_num_of_xforms( 16 ), 2 );
    EXPECT_EQ( speck::calc_num_of_xforms( 17 ), 2 );
    EXPECT_EQ( speck::calc_num_of_xforms( 31 ), 2 );
    EXPECT_EQ( speck::calc_num_of_xforms( 32 ), 3 );
    EXPECT_EQ( speck::calc_num_of_xforms( 63 ), 3 );
    EXPECT_EQ( speck::calc_num_of_xforms( 64 ), 4 );
    EXPECT_EQ( speck::calc_num_of_xforms( 127 ), 4 );
    EXPECT_EQ( speck::calc_num_of_xforms( 128 ), 5 );
}



}
