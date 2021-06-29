
#include "CDF97.h"
#include "Conditioner.h"
#include <cstdlib>
#include "gtest/gtest.h"

namespace
{

TEST( dwt1d, big_image_even )
{
    const char*   input = "../test_data/128x128.float";
    size_t  dim_x = 128;
    const size_t  total_vals = dim_x;

    // Let read in binaries as 4-byte floats
    auto in_buf = std::make_unique<float[]>( total_vals );
    if( speck::read_n_bytes( input, sizeof(float) * total_vals, in_buf.get() ) != speck::RTNType::Good )
        std::cerr << "Input read error!" << std::endl;

    // Make a copy and then use a conditioner
    auto in_copy = std::vector<double>( total_vals );
    std::copy( in_buf.get(), in_buf.get() + total_vals, in_copy.begin() );
    speck::Conditioner condi( true );
    auto [rtn, meta] = condi.condition( in_copy );

    // Use a speck::CDF97 to perform DWT and IDWT.
    speck::CDF97 cdf;
    cdf.take_data( std::move(in_copy), {dim_x, 1, 1} );
    cdf.dwt1d();
    cdf.idwt1d();

    // Claim that with single precision, the result is identical to the input
    auto result = cdf.release_data();
    EXPECT_EQ( result.size(), total_vals );

    // Apply the conditioner
    rtn = condi.inverse_condition( result, meta.data() );

    for( size_t i = 0; i < total_vals; i++ )
    {
        EXPECT_EQ( in_buf[i], float(result[i]) );
    }
}

TEST( dwt1d, big_image_odd )
{
    const char*   input = "../test_data/999x999.float";
    size_t  dim_x = 999;
    const size_t  total_vals = dim_x;

    // Let read in binaries as 4-byte floats
    auto in_buf = std::make_unique<float[]>( total_vals );
    if( speck::read_n_bytes( input, sizeof(float) * total_vals, in_buf.get() ) != speck::RTNType::Good )
        std::cerr << "Input read error!" << std::endl;

    // Make a copy and use a conditioner
    auto in_copy = std::vector<double>( total_vals );
    std::copy( in_buf.get(), in_buf.get() + total_vals, in_copy.begin() );
    speck::Conditioner condi( true );
    auto [rtn, meta] = condi.condition( in_copy );

    // Use a speck::CDF97 to perform DWT and IDWT.
    speck::CDF97 cdf;
    cdf.take_data( std::move(in_copy), {dim_x, 1, 1} );
    cdf.dwt1d();
    cdf.idwt1d();

    // Claim that with single precision, the result is identical to the input
    auto result = cdf.release_data();
    EXPECT_EQ( result.size(), total_vals );

    // Apply the conditioner again
    rtn = condi.inverse_condition( result, meta.data() );

    for( size_t i = 0; i < total_vals; i++ )
    {
        EXPECT_EQ( in_buf[i], float(result[i]) );
    }
}

TEST( dwt2d, small_image_even )
{
    const char*   input = "../test_data/16x16.float";
    size_t  dim_x = 16, dim_y = 16;
    const size_t  total_vals = dim_x * dim_y;

    // Let read in binaries as 4-byte floats
    auto in_buf = speck::read_whole_file<float>(input);
    if( in_buf.size() != total_vals )
        std::cerr << "Input read error!" << std::endl;

    // Make a copy and use a conditioner
    auto in_copy = std::vector<double>( total_vals );
    std::copy( in_buf.begin(), in_buf.end(), in_copy.begin() );
    speck::Conditioner condi( true );
    auto [rtn, meta] = condi.condition( in_copy );

    // Use a speck::CDF97 to perform DWT and IDWT.
    speck::CDF97 cdf;
    cdf.take_data( std::move(in_copy), {dim_x, dim_y, 1} );
    cdf.dwt2d();
    cdf.idwt2d();

    // Claim that with single precision, the result is identical to the input
    auto result = cdf.release_data( );
    EXPECT_EQ( result.size(), total_vals );

    // Apply the conditioner again
    rtn = condi.inverse_condition( result, meta.data() );

    for( size_t i = 0; i < total_vals; i++ )
    {
        EXPECT_EQ( in_buf[i], float(result[i]) );
    }
}

TEST( dwt2d, small_image_odd )
{
    const char*   input = "../test_data/15x15.float";
    size_t dim_x = 15, dim_y = 15;
    const size_t  total_vals = dim_x * dim_y;

    // Let read in binaries as 4-byte floats
    auto in_buf = speck::read_whole_file<float>(input);
    if( in_buf.size() != total_vals )
        std::cerr << "Input read error!" << std::endl;

    // Make a copy and use a conditioner
    auto in_copy = std::vector<double>( total_vals );
    std::copy( in_buf.begin(), in_buf.end(), in_copy.begin() );
    speck::Conditioner condi( true );
    auto [rtn, meta] = condi.condition( in_copy );

    // Use a speck::CDF97 to perform DWT and IDWT.
    speck::CDF97 cdf;
    cdf.take_data( std::move(in_copy), {dim_x, dim_y, 1} );
    cdf.dwt2d();
    cdf.idwt2d();

    // Claim that with single precision, the result is identical to the input
    auto result = cdf.release_data( );
    EXPECT_EQ( result.size(), total_vals );

    // Apply the conditioner
    rtn = condi.inverse_condition( result, meta.data() );

    for( size_t i = 0; i < total_vals; i++ )
    {
        EXPECT_EQ( in_buf[i], float(result[i]) );
    }
}

TEST( dwt2d, big_image_even )
{
    const char*   input = "../test_data/128x128.float";
    size_t  dim_x = 128, dim_y = 128;
    const size_t  total_vals = dim_x * dim_y;

    // Let read in binaries as 4-byte floats
    auto in_buf = speck::read_whole_file<float>(input);
    if( in_buf.size() != total_vals )
        std::cerr << "Input read error!" << std::endl;

    // Make a copy and use a conditioner
    auto in_copy = std::vector<double>( total_vals );
    std::copy( in_buf.begin(), in_buf.end(), in_copy.begin() );
    speck::Conditioner condi( true );
    auto [rtn, meta] = condi.condition( in_copy );

    // Use a speck::CDF97 to perform DWT and IDWT.
    speck::CDF97 cdf;
    cdf.take_data( std::move(in_copy), {dim_x, dim_y, 1} );
    cdf.dwt2d();
    cdf.idwt2d();

    // Claim that with single precision, the result is identical to the input
    auto result = cdf.release_data();
    EXPECT_EQ( result.size(), total_vals );

    // Apply the conditioner
    rtn = condi.inverse_condition( result, meta.data() );

    for( size_t i = 0; i < total_vals; i++ )
    {
        EXPECT_EQ( in_buf[i], float(result[i]) );
    }
}

TEST( dwt2d, big_image_odd )
{
    const char*   input = "../test_data/127x127.float";
    size_t  dim_x = 127, dim_y = 127;
    const size_t  total_vals = dim_x * dim_y;

    // Let read in binaries as 4-byte floats
    auto in_buf = speck::read_whole_file<float>(input);
    if( in_buf.size() != total_vals )
        std::cerr << "Input read error!" << std::endl;

    // Make a copy and use a conditioner
    auto in_copy = std::vector<double>( total_vals );
    std::copy( in_buf.begin(), in_buf.end(), in_copy.begin() );
    speck::Conditioner condi( true );
    auto [rtn, meta] = condi.condition( in_copy );

    // Use a speck::CDF97 to perform DWT and IDWT.
    speck::CDF97 cdf;
    cdf.take_data( std::move(in_copy), {dim_x, dim_y, 1} );
    cdf.dwt2d();
    cdf.idwt2d();

    // Claim that with single precision, the result is identical to the input
    auto result = cdf.release_data();
    EXPECT_EQ( result.size(), total_vals );

    // Apply the conditioner
    rtn = condi.inverse_condition( result, meta.data() );

    for( size_t i = 0; i < total_vals; i++ )
    {
        EXPECT_EQ( in_buf[i], float(result[i]) );
    }
}

TEST( dwt3d, small_even_cube )
{
    const char*   input = "../test_data/wmag16.float";
    size_t  dim_x = 16, dim_y = 16, dim_z = 16;
    const size_t  total_vals = dim_x * dim_y * dim_z;

    // Let read in binaries as 4-byte floats
    auto in_buf = speck::read_whole_file<float>(input);
    if( in_buf.size() != total_vals )
        std::cerr << "Input read error!" << std::endl;

    // Make a copy and use a conditioner
    auto in_copy = std::vector<double>( total_vals );
    std::copy( in_buf.begin(), in_buf.end(), in_copy.begin() );
    speck::Conditioner condi( true );
    auto [rtn, meta] = condi.condition( in_copy );

    // Use a speck::CDF97 to perform DWT and IDWT.
    speck::CDF97 cdf;
    cdf.take_data( std::move(in_copy), {dim_x, dim_y, dim_z} );
    cdf.dwt3d();
    cdf.idwt3d();

    // Claim that with single precision, the result is identical to the input
    auto result = cdf.release_data( );
    EXPECT_EQ( result.size(), total_vals );

    // Apply the conditioner
    rtn = condi.inverse_condition( result, meta.data() );

    for( size_t i = 0; i < total_vals; i++ )
    {
        EXPECT_EQ( in_buf[i], float(result[i]) );
    }
}

TEST( dwt3d, big_odd_cube )
{
    const char*   input = "../test_data/wmag91.float";
    size_t  dim_x = 91, dim_y = 91, dim_z = 91;
    const size_t  total_vals = dim_x * dim_y * dim_z;

    // Let read in binaries as 4-byte floats
    auto in_buf = speck::read_whole_file<float>(input);
    if( in_buf.size() != total_vals )
        std::cerr << "Input read error!" << std::endl;

    // Make a copy and use a conditioner
    auto in_copy = std::vector<double>( total_vals );
    std::copy( in_buf.begin(), in_buf.end(), in_copy.begin() );
    speck::Conditioner condi( true );
    auto [rtn, meta] = condi.condition( in_copy );

    // Use a speck::CDF97 to perform DWT and IDWT.
    speck::CDF97 cdf;
    cdf.take_data( std::move(in_copy), {dim_x, dim_y, dim_z} );
    cdf.dwt3d();
    cdf.idwt3d();

    // Claim that with single precision, the result is identical to the input
    auto result = cdf.release_data( );
    EXPECT_EQ( result.size(), total_vals );

    // Apply the conditioner
    rtn = condi.inverse_condition( result, meta.data() );

    for( size_t i = 0; i < total_vals; i++ )
    {
        EXPECT_EQ( in_buf[i], float(result[i]) );
    }
}

TEST( dwt3d, big_even_cube )
{
    const char*   input = "../test_data/wmag128.float";
    size_t  dim_x = 128, dim_y = 128, dim_z = 128;
    const size_t  total_vals = dim_x * dim_y * dim_z;

    // Let read in binaries as 4-byte floats
    auto in_buf = speck::read_whole_file<float>(input);
    if( in_buf.size() != total_vals )
        std::cerr << "Input read error!" << std::endl;

    // Make a copy and use a conditioner
    auto in_copy = std::vector<double>( total_vals );
    std::copy( in_buf.begin(), in_buf.end(), in_copy.begin() );
    speck::Conditioner condi( true );
    auto [rtn, meta] = condi.condition( in_copy );

    // Use a speck::CDF97 to perform DWT and IDWT.
    speck::CDF97 cdf;
    cdf.take_data( std::move(in_copy), {dim_x, dim_y, dim_z} );
    cdf.dwt3d();
    cdf.idwt3d();

    // Claim that with single precision, the result is identical to the input
    auto result = cdf.release_data( );
    EXPECT_EQ( result.size(), total_vals );

    // Apply the conditioner
    rtn = condi.inverse_condition( result, meta.data() );

    for( size_t i = 0; i < total_vals; i++ )
    {
        EXPECT_EQ( in_buf[i], float(result[i]) );
    }
}


}
