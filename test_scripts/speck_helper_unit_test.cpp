
#include "speck_helper.h"
#include "gtest/gtest.h"

namespace
{

TEST( speck_helper, num_of_xforms )
{
    EXPECT_EQ( speck::calc_num_of_xforms( 1 ), 0 );
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


TEST( speck_helper, approx_detail_len )
{
    std::array<size_t, 2> len;
    speck::calc_approx_detail_len( 7, 0, len );
    EXPECT_EQ( len[0], 7 );
    EXPECT_EQ( len[1], 0 );
    speck::calc_approx_detail_len( 7, 1, len );
    EXPECT_EQ( len[0], 4 );
    EXPECT_EQ( len[1], 3 );
    speck::calc_approx_detail_len( 8, 1, len );
    EXPECT_EQ( len[0], 4 );
    EXPECT_EQ( len[1], 4 );
    speck::calc_approx_detail_len( 8, 2, len );
    EXPECT_EQ( len[0], 2 );
    EXPECT_EQ( len[1], 2 );
    speck::calc_approx_detail_len( 16, 2, len );
    EXPECT_EQ( len[0], 4 );
    EXPECT_EQ( len[1], 4 );
}


}
