
#include "speck_helper.h"
#include "gtest/gtest.h"

namespace
{

TEST( speck_helper, num_of_xforms )
{
    EXPECT_EQ( speck::num_of_xforms( 1 ), 0 );
    EXPECT_EQ( speck::num_of_xforms( 7 ), 0 );
    EXPECT_EQ( speck::num_of_xforms( 8 ), 1 );
    EXPECT_EQ( speck::num_of_xforms( 9 ), 1 );
    EXPECT_EQ( speck::num_of_xforms( 15 ), 1 );
    EXPECT_EQ( speck::num_of_xforms( 16 ), 2 );
    EXPECT_EQ( speck::num_of_xforms( 17 ), 2 );
    EXPECT_EQ( speck::num_of_xforms( 31 ), 2 );
    EXPECT_EQ( speck::num_of_xforms( 32 ), 3 );
    EXPECT_EQ( speck::num_of_xforms( 63 ), 3 );
    EXPECT_EQ( speck::num_of_xforms( 64 ), 4 );
    EXPECT_EQ( speck::num_of_xforms( 127 ), 4 );
    EXPECT_EQ( speck::num_of_xforms( 128 ), 5 );
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


TEST( speck_helper, bit_packing )
{
    const size_t num_of_bytes = 11;
    const size_t byte_offset  = 1;
    std::vector<bool>  input {true,  true,  true,  true,  true,  true,  true,  true,  // 1st byte
                              false, false, false, false, false, false, false, false, // 2nd byte
                              true,  false, true,  false, true,  false, true,  false, // 3rd byte
                              false, true,  false, true,  false, true,  false, true,  // 4th byte
                              true,  true,  false, false, true,  true,  false, false, // 5th byte
                              false, false, true,  true,  false, false, true,  true,  // 6th byte
                              false, false, true,  true,  false, false, true,  false, // 7th byte
                              true,  false, false, false, true,  true,  true,  false, // 8th byte
                              false, false, false, true,  false, false, false, true,  // 9th byte
                              true,  true,  true,  false, true,  true,  true,  false, // 10th byte
                              false, false, true,  true,  true,  false, false, true };// 11th byte

    auto bytes = std::make_unique<uint8_t[]>(num_of_bytes + byte_offset);

    // Pack booleans
    auto rtn = speck::pack_booleans( bytes, input, byte_offset );
    EXPECT_EQ( rtn, speck::RTNType::Good );
    // Unpack booleans
    auto output = std::vector<bool>( num_of_bytes * 8 );
    rtn = speck::unpack_booleans( output, bytes.get(), num_of_bytes + byte_offset, byte_offset );

    EXPECT_EQ( rtn, speck::RTNType::Good );
    EXPECT_EQ( input.size(), output.size() );

    for( size_t i = 0; i < input.size(); i++ )
        EXPECT_EQ( input[i], output[i] );
}


TEST( speck_helper, bit_packing_one_byte )
{
    unsigned char byte;
    bool output[8];

    // All true 
    bool input[8] {true,  true,  true,  true,  true,  true,  true,  true };
    // Pack booleans
    speck::pack_8_booleans( byte, input );
    // Unpack booleans
    speck::unpack_8_booleans( output, byte );
    for( size_t i = 0; i < 8; i++ )
        EXPECT_EQ( input[i], output[i] );

    // Odd locations false
    for( size_t i = 1; i < 8; i += 2 )
        input[i] = false;
    speck::pack_8_booleans( byte, input );
    speck::unpack_8_booleans( output, byte );
    for( size_t i = 0; i < 8; i++ )
        EXPECT_EQ( input[i], output[i] );

    // All false
    for( size_t i = 0; i < 8; i++ )
        input[i] = false;
    speck::pack_8_booleans( byte, input );
    speck::unpack_8_booleans( output, byte );
    for( size_t i = 0; i < 8; i++ )
        EXPECT_EQ( input[i], output[i] );

    // Odd locations true
    for( size_t i = 1; i < 8; i += 2 )
        input[i] = true;
    speck::pack_8_booleans( byte, input );
    speck::unpack_8_booleans( output, byte );
    for( size_t i = 0; i < 8; i++ )
        EXPECT_EQ( input[i], output[i] );
}


}
