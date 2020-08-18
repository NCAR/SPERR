
#include "CDF97.h"
#include <cstdlib>
#include "gtest/gtest.h"

namespace
{

extern "C"  // C Function calls, and don't include the C header!
{
    int sam_read_n_bytes( const char*, size_t, void* );
}

TEST( dwt1d, big_image_even )
{
    const char*   input = "../test_data/128x128.float";
    size_t  dim_x = 128;
    const size_t  total_vals = dim_x;

    // Let read in binaries as 4-byte floats
    auto in_buf = speck::unique_malloc<float>( total_vals );
    if( sam_read_n_bytes( input, sizeof(float) * total_vals, in_buf.get() ) )
        std::cerr << "Input read error!" << std::endl;

    // Use a speck::CDF97 to perform DWT and IDWT.
    speck::CDF97 cdf;
    cdf.set_dims( dim_x );
    cdf.copy_data( in_buf.get(), total_vals );
    cdf.dwt1d();
    cdf.idwt1d();

    // Claim that with single precision, the result is identical to the input
    const auto& result = cdf.get_read_only_data();
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
    auto in_buf = speck::unique_malloc<float>( total_vals );
    if( sam_read_n_bytes( input, sizeof(float) * total_vals, in_buf.get() ) )
        std::cerr << "Input read error!" << std::endl;

    // Use a speck::CDF97 to perform DWT and IDWT.
    speck::CDF97 cdf;
    cdf.set_dims( dim_x );
    cdf.copy_data( in_buf.get(), total_vals );
    cdf.dwt1d();
    cdf.idwt1d();

    // Claim that with single precision, the result is identical to the input
    const auto& result = cdf.get_read_only_data();
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
    auto in_buf = speck::unique_malloc<float>( total_vals );
    if( sam_read_n_bytes( input, sizeof(float) * total_vals, in_buf.get() ) )
        std::cerr << "Input read error!" << std::endl;

    // Use a speck::CDF97 to perform DWT and IDWT.
    speck::CDF97 cdf;
    cdf.set_dims( dim_x, dim_y );
    cdf.copy_data( in_buf.get(), dim_x * dim_y );
    cdf.dwt2d();
    cdf.idwt2d();

    // Claim that with single precision, the result is identical to the input
    const auto& result = cdf.get_read_only_data();
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
    auto in_buf = speck::unique_malloc<float>( total_vals );
    if( sam_read_n_bytes( input, sizeof(float) * total_vals, in_buf.get() ) )
        std::cerr << "Input read error!" << std::endl;

    // Use a speck::CDF97 to perform DWT and IDWT.
    speck::CDF97 cdf;
    cdf.set_dims( dim_x, dim_y );
    cdf.copy_data( in_buf.get(), dim_x * dim_y );
    cdf.dwt2d();
    cdf.idwt2d();

    // Claim that with single precision, the result is identical to the input
    const auto& result = cdf.get_read_only_data();
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
    auto in_buf = speck::unique_malloc<float>( total_vals );
    if( sam_read_n_bytes( input, sizeof(float) * total_vals, in_buf.get() ) )
        std::cerr << "Input read error!" << std::endl;

    // Use a speck::CDF97 to perform DWT and IDWT.
    speck::CDF97 cdf;
    cdf.set_dims( dim_x, dim_y );
    cdf.copy_data( in_buf.get(), dim_x * dim_y );
    cdf.dwt2d();
    cdf.idwt2d();

    // Claim that with single precision, the result is identical to the input
    const auto& result = cdf.get_read_only_data();
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
    auto in_buf = speck::unique_malloc<float>( total_vals );
    if( sam_read_n_bytes( input, sizeof(float) * total_vals, in_buf.get() ) )
        std::cerr << "Input read error!" << std::endl;

    // Use a speck::CDF97 to perform DWT and IDWT.
    speck::CDF97 cdf;
    cdf.set_dims( dim_x, dim_y );
    cdf.copy_data( in_buf.get(), dim_x * dim_y );
    cdf.dwt2d();
    cdf.idwt2d();

    // Claim that with single precision, the result is identical to the input
    const auto& result = cdf.get_read_only_data();
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
    auto in_buf = speck::unique_malloc<float>( total_vals );
    if( sam_read_n_bytes( input, sizeof(float) * total_vals, in_buf.get() ) )
        std::cerr << "Input read error!" << std::endl;

    // Use a speck::CDF97 to perform DWT and IDWT.
    speck::CDF97 cdf;
    cdf.set_dims( dim_x, dim_y, dim_z );
    cdf.copy_data( in_buf.get(), total_vals );
    cdf.dwt3d();
    cdf.idwt3d();

    // Claim that with single precision, the result is identical to the input
    const auto& result = cdf.get_read_only_data();
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
    auto in_buf = speck::unique_malloc<float>( total_vals );
    if( sam_read_n_bytes( input, sizeof(float) * total_vals, in_buf.get() ) )
        std::cerr << "Input read error!" << std::endl;

    // Use a speck::CDF97 to perform DWT and IDWT.
    speck::CDF97 cdf;
    cdf.set_dims( dim_x, dim_y, dim_z );
    cdf.copy_data( in_buf.get(), total_vals );
    cdf.dwt3d();
    cdf.idwt3d();

    // Claim that with single precision, the result is identical to the input
    const auto& result = cdf.get_read_only_data();
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
    auto in_buf = speck::unique_malloc<float>( total_vals );
    if( sam_read_n_bytes( input, sizeof(float) * total_vals, in_buf.get() ) )
        std::cerr << "Input read error!" << std::endl;

    // Use a speck::CDF97 to perform DWT and IDWT.
    speck::CDF97 cdf;
    cdf.set_dims( dim_x, dim_y, dim_z );
    cdf.copy_data( in_buf.get(), total_vals );
    cdf.dwt3d();
    cdf.idwt3d();

    // Claim that with single precision, the result is identical to the input
    const auto& result = cdf.get_read_only_data();
    for( size_t i = 0; i < total_vals; i++ )
    {
        EXPECT_EQ( in_buf[i], float(result[i]) );
    }
}


}
