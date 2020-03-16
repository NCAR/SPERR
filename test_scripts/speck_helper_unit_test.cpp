
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


TEST( speck_helper, input_output )
{
    size_t dims[2] = { 101, 999 };
    double mean    = 3.14159;
    uint16_t max_bit = 128;
    const char* filename = "temp.temp";
    std::vector<bool> a{ true, false, true, false, false, true, false, true, 
                         true, true, false, false, true, true, false, false };
    speck::output_speck2d( dims[0], dims[1], mean, max_bit, a, filename );

    size_t dim_x, dim_y;
    double mean_r;
    uint16_t max_bit_r;
    std::vector<bool> a_r;
    speck::input_speck2d( dim_x, dim_y, mean_r, max_bit_r, a_r, filename );

    EXPECT_EQ( dims[0], dim_x );
    EXPECT_EQ( dims[1], dim_y );
    EXPECT_EQ( mean,    mean_r );
    EXPECT_EQ( max_bit, max_bit_r );
    EXPECT_EQ( a.size(), a_r.size() );
    for( size_t i = 0; i < a.size(); i++ )
    {
        EXPECT_EQ( a[i], a_r[i] );
    }
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
